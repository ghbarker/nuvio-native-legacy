"""Private emulator diagnostics; never copies session/token file contents to proof.

Import StartupDiagnostics beside the smoke driver. Call checked_seed for every
seed, snapshot('before-launch'), then snapshot('after-launch') and in finally.
This module does not launch the APK, change firewall rules, or invent UI state.
"""
from pathlib import Path
import hashlib
import base64
import json
import re
import subprocess


class StartupDiagnostics:
    def __init__(self, package, output):
        if not re.fullmatch(r'[A-Za-z][A-Za-z0-9_.]+', package):
            raise ValueError('Unsafe package')
        self.package = package
        self.output = Path(output)
        self.output.mkdir(parents=True, exist_ok=True)
        self.expected = {}
        self.seed_checks = []

    @staticmethod
    def _adb(*args, data=None):
        return subprocess.run(['adb', *args], input=data, stdout=subprocess.PIPE,
                              stderr=subprocess.PIPE, timeout=30)

    def _read(self, relative):
        return self._adb('exec-out', 'run-as', self.package, 'cat', relative)

    @staticmethod
    def _identity(data):
        return {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}

    def checked_seed(self, name, text):
        if not re.fullmatch(r'[A-Za-z0-9_.-]+', name):
            raise ValueError('Unsafe seed filename')
        relative = 'files/dados/' + name
        data = text.encode('utf-8')
        command = "run-as " + self.package + " sh -c 'cat > " + relative + "'"
        written = self._adb('shell', '-T', command, data=data)
        if written.returncode:
            raise RuntimeError('Seed write failed: ' + name)
        read = self._read(relative)
        first_identity = {'read_status': read.returncode, **self._identity(read.stdout)}
        fallback_used = read.returncode != 0 or read.stdout != data
        if fallback_used:
            encoded = base64.b64encode(data).decode('ascii')
            repair = ("run-as " + self.package + " sh -c 'printf %s " +
                      encoded + " | base64 -d > " + relative + "'")
            repaired = self._adb('shell', '-T', repair)
            if repaired.returncode:
                raise RuntimeError('Identical-byte seed transport repair failed: ' + name)
            read = self._read(relative)
        entry = {'file': relative, 'first_readback': first_identity,
                 'identical_byte_transport_fallback': fallback_used,
                 'expected': self._identity(data),
                 'actual': self._identity(read.stdout),
                 'read_status': read.returncode,
                 'byte_equal': read.returncode == 0 and read.stdout == data}
        self.seed_checks.append(entry)
        (self.output / 'seed-write-readback.json').write_text(
            json.dumps(self.seed_checks, indent=2) + '\n')
        if not entry['byte_equal']:
            raise RuntimeError('Seed byte readback mismatch: ' + name)
        self.expected[relative] = data

    @staticmethod
    def _redact(text):
        text = re.sub(r'https?://\S+', '<url>', text)
        text = re.sub(r'(?i)(bearer\s+)\S+', r'\1<redacted>', text)
        text = re.sub(r'\b(?:eyJ[A-Za-z0-9_-]*|e30)\.[A-Za-z0-9_-]+\.[A-Za-z0-9_-]+\b',
                      '<jwt>', text)
        return text.replace('offline-refresh', '<synthetic-refresh>')

    def snapshot(self, phase):
        if not re.fullmatch(r'[A-Za-z0-9_-]+', phase):
            raise ValueError('Unsafe diagnostic phase')
        result = {'phase': phase, 'seed_readbacks': [], 'commands': {},
                  'native_logs': [], 'errors': []}
        for relative, expected in self.expected.items():
            try:
                read = self._read(relative)
                result['seed_readbacks'].append({
                    'file': relative, 'read_status': read.returncode,
                    'actual': self._identity(read.stdout),
                    'expected': self._identity(expected),
                    'byte_equal': read.returncode == 0 and read.stdout == expected})
            except Exception as exc:
                result['errors'].append({'file': relative, 'error': str(exc)})
        commands = {
            'files-tree': "run-as " + self.package +
                " find files -maxdepth 6 -type f -exec ls -ln '{}' ';'",
            'window': 'dumpsys window',
            'activity': 'dumpsys activity activities',
        }
        for label, command in commands.items():
            try:
                got = self._adb('shell', command)
                filename = phase + '-' + label + '.txt'
                # File listings and Android dumps contain no seed-file bodies.
                (self.output / filename).write_text(
                    self._redact(got.stdout.decode(errors='replace')))
                result['commands'][label] = {'file': filename,
                    'status': got.returncode, **self._identity(got.stdout)}
            except Exception as exc:
                result['errors'].append({'command': label, 'error': str(exc)})
        try:
            pid_reply = self._adb('shell', 'pidof ' + self.package)
            pids = pid_reply.stdout.decode(errors='replace').split()
            result['pids'] = pids
            if pids and all(re.fullmatch(r'[0-9]+', pid) for pid in pids):
                # Filtering happens inside the emulator: no complete process
                # environment is returned to or retained on the CI host.
                command = ('cat /proc/' + pids[0] + "/environ | tr '\\000' '\\n' | "
                           "grep -E '^(NUVIO_DADOS|NUVIO_ARTE|HOME)='")
                env = self._adb('shell', command)
                values = env.stdout.decode(errors='replace').splitlines()
                if any(not re.match(r'^(NUVIO_DADOS|NUVIO_ARTE|HOME)=', line)
                       for line in values):
                    raise RuntimeError('Unexpected environment output')
                result['approved_process_environment'] = {
                    'status': env.returncode, 'values': values}
        except Exception as exc:
            result['errors'].append({'command': 'selected-environment', 'error': str(exc)})
        try:
            paths = self._adb('shell', 'run-as ' + self.package +
                              " find files -maxdepth 6 -type f -name 'nuvio*.log'")
            for relative in paths.stdout.decode(errors='replace').splitlines():
                if not re.fullmatch(r'files/[A-Za-z0-9_./-]+', relative):
                    continue
                got = self._read(relative)
                text = got.stdout.decode(errors='replace')
                filename = phase + '-native-' + relative.replace('/', '_') + '.txt'
                (self.output / filename).write_text(self._redact(text))
                result['native_logs'].append({
                    'source_file': relative, 'file': filename,
                    'status': got.returncode, **self._identity(got.stdout),
                    'session_restored_marker': '[sessao] sessao restaurada' in text,
                    'invalid_session_marker': 'sessao gravada invalida' in text})
        except Exception as exc:
            result['errors'].append({'command': 'native-logs', 'error': str(exc)})
        (self.output / (phase + '-startup-diagnostics.json')).write_text(
            json.dumps(result, indent=2) + '\n')
        return result
