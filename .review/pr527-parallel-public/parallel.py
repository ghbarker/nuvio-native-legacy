"""Run actual public collection reader, UI scheduler and retained URL contracts."""
from pathlib import Path
import argparse
import json
import os
import platform
import subprocess
import sys
from common import HERE, sha, source_binding

p = argparse.ArgumentParser()
for name in ['source', 'proof', 'output']:
    p.add_argument('--' + name, type=Path, required=True)
a = p.parse_args()
s, out = a.source.resolve(), a.output.resolve()
assert platform.system() == 'Linux' and not out.exists()
plan, binding, manifest = source_binding(s, a.proof)
fixtures = HERE / 'fixtures'
root = fixtures / 'parallel'
contract = json.loads((root / 'contract.json').read_text())
assert len(contract['reader_cases']) == 45 and len(contract['ui_markers']) == 10
cc = str(Path('/usr/bin/cc').resolve())
assert Path(cc).is_file()
env = dict(os.environ, CC=cc)
out.mkdir(parents=True)
records = []
source_inputs = {str(path.relative_to(s)): sha(path) for path in (s / 'src').glob('*.h')}
source_inputs.update({name: sha(s / name) for name in [
    'src/descoberta.c', 'src/vertudo.c', 'src/perfis.c', 'src/js.c', 'src/redeurl.c',
    'src/metaprov.c', 'src/cwordem.c', 'src/cotacat.c', 'src/colecoes.c', 'src/catordem.c']})
fixture_inputs = {}
for mode in ['normal', 'asan']:
    san = ['--sanitize'] if mode == 'asan' else []
    for name in ['reader', 'ui', 'owned']:
        directory = out / (name + '-' + mode)
        if name == 'reader':
            command = [sys.executable, str(root / 'reader/run.py'), '--source', str(s),
                       '--out', str(directory), '--sdk', str(root / 'owned-support/sdk'), *san]
        elif name == 'ui':
            command = [sys.executable, str(fixtures / 'async/run_parallel.py'), '--source', str(s),
                       '--output', str(directory), '--cc', cc, *san]
        else:
            command = [sys.executable, str(root / 'owned-url/run.py'), '--source', str(s),
                       '--support', str(root / 'owned-support'), '--out', str(directory), *san]
        producer_command = list(command)
        result = subprocess.run(command, env=env, text=True, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, timeout=900)
        logfile = out / (name + '-' + mode + '-producer.log')
        logfile.write_text(result.stdout)
        assert result.returncode == 0, result.stdout[-8000:]
        rp = directory / 'receipt.json'
        raw = json.loads(rp.read_text())
        assert raw['source'] == str(s)
        if name == 'reader':
            assert raw['all_passed'] is True and raw['expected_cases'] == 45
            assert raw['source_unchanged'] is True and raw['source_sha256_before'] == raw['source_sha256']
            assert raw['sanitized'] is (mode == 'asan')
            assert [record['name'] for record in raw['records']] == ['compile', *contract['reader_cases']]
            for record in raw['records']:
                assert record['exit_code'] == 0
                log = directory / record['log']
                assert sha(log) == record['log_sha256']
                if record['name'] != 'compile':
                    text = log.read_text()
                    assert 'PASS ' + record['name'] + ' ' in text
                    assert ('actual-reader-to-legacy-continuation' if record['name'].startswith('manual-')
                            else 'singleton=unchanged') in text
            commands = [raw['records'][0]['command']]
            assert any(flag.startswith('-DDISCOVERY_SOURCE=') and str(s / 'src/descoberta.c') in flag
                       for flag in commands[0])
            for relative, expected in raw['source_sha256'].items():
                assert sha(s / relative) == expected
                source_inputs[relative] = expected
            for relative, expected in raw['fixture_sha256'].items():
                assert sha(root / 'reader' / relative) == expected
            assert sha(directory / 'backend-reader') == raw['binary_sha256']
        elif name == 'ui':
            assert raw['source_commit'] == manifest['head'] and raw['source_status'] == ''
            assert raw['private'] is False and raw['sanitized'] is (mode == 'asan')
            assert raw['exit_code'] == 0 and raw['abort_traps_fired'] is False
            assert len(raw['runs']) == 10
            assert {record['case'].removeprefix('v7-o0-z0-r0-') for record in raw['runs']} == set(contract['ui_markers'])
            for record in raw['runs']:
                assert record['exit_code'] == 0 and record['abort_traps_fired'] is False
                text = (directory / ('run-' + record['case'] + '.log')).read_text()
                case = record['case'].removeprefix('v7-o0-z0-r0-')
                assert contract['ui_markers'][case] in text
                if not case.startswith('parallel-manual-'):
                    assert 'PARALLEL_FIRST_THREE_STARTED_WITH_FIRST_BLOCKED calls=3 active=3 max=3' in text
            commands = [command for command in raw['commands'] if '-c' in command]
            assert commands and all('-DNV_ANDROID=1' in command and '-DNV_TOUCH_UI=1' in command
                                    for command in commands)
            for path, expected in raw['dependency_sha256'].items():
                path = Path(path).resolve()
                assert sha(path) == expected
                if path.is_relative_to(s):
                    source_inputs[str(path.relative_to(s))] = expected
                elif path.is_relative_to(fixtures):
                    fixture_inputs[str(path.relative_to(fixtures))] = expected
                else:
                    assert path.is_relative_to(out), 'Unbound fixture input: ' + str(path)
            for relative, expected in raw['tooling_sha256'].items():
                assert sha(fixtures / 'async' / relative) == expected
            assert sha(directory / 'probe') == raw['binary_sha256']
        else:
            assert raw['passed'] is True and raw['expected_bug'] is False and raw['run_exit'] == 0
            assert raw['head'] == manifest['head'] and raw['sanitize'] is (mode == 'asan')
            assert raw['source_unchanged'] is True
            assert raw['source_sha256_before'] == raw['source_sha256'] == sha(s / 'src/descoberta.c')
            assert sha(root / 'owned-url/retained.c') == raw['fixture_sha256']
            for relative, expected in raw['support_sha256'].items():
                assert sha(root / 'owned-support' / relative) == expected
            log = directory / 'run.log'
            assert sha(log) == raw['log_sha256'] and contract['owned_marker'] in log.read_text()
            assert sha(directory / 'retained') == raw['binary_sha256']
            commands = [raw['compile']]
            assert ('-fsanitize-address-use-after-scope' in commands[0]) is (mode == 'asan')
        for command in commands:
            assert command[0] == cc
            assert ('-fsanitize=address,undefined' in command) is (mode == 'asan')
            assert not any('NV_TEST_LOCAL_LOGS' in flag for flag in command)
        records.append({'component': name, 'mode': mode, 'command': producer_command,
                        'raw_receipt_relative': str(rp.relative_to(out)), 'raw_receipt_sha256': sha(rp),
                        'producer_log_relative': logfile.name, 'producer_log_sha256': sha(logfile),
                        'cases': 45 if name == 'reader' else 10 if name == 'ui' else 1})
for relative, expected in source_inputs.items():
    assert sha(s / relative) == expected
source_binding(s, a.proof)
value = {'outcome': 'passed', 'source_binding': binding, 'plan_sha256': sha(HERE / 'plan.json'),
         'records': records, 'source_inputs_sha256': source_inputs,
         'fixture_inputs_sha256': fixture_inputs, 'compiler': cc,
         'contract_sha256': sha(root / 'contract.json'),
         'artifacts_sha256': {str(path.relative_to(out)): sha(path) for path in out.rglob('*')
                              if path.is_file() and path.suffix != '.o'},
         'scope': 'Fresh exact public source: actual reader45/manual continuation, scheduler/UI10, '
                  'and retained URL lifetime, normal plus ASan/UBSan; no private overlay.'}
(out / 'parallel-verified.json').write_text(json.dumps(value, indent=2) + '\n')
print('PASS public reader45 + UI10 + retained URL in both modes')
