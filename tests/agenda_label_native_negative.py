#!/usr/bin/env python3
"""In an isolated test checkout, prove the real-font label assertion detects
the old exact-fit truncation. Keep the failing capture evidence, restore the
Agenda source, and require independent positive captures as well.
"""
from pathlib import Path
import hashlib
import json
import os
import re
import subprocess
import sys


ALLOWANCE = b'if (telefone) w[k] += escDe(corpo, fonte);'
DISABLED = b'/* phone label allowance disabled for native negative control */'
CASE = '1080x2340@1'
CAPTURE_LOG = 'agenda/1080x2340-ui1/capture.log'


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def validate_failure(output, returncode):
    require(returncode == 1, f'Expected capture driver status 1, got {returncode}')
    results = json.loads((output / 'results.json').read_text(encoding='utf-8'))
    require(isinstance(results, list) and len(results) == 1,
            'Expected exactly one Agenda capture result')
    result = results[0]
    require(result.get('fixture') == 'agenda' and result.get('case') == CASE
            and result.get('status') == 'capture failed'
            and result.get('log') == CAPTURE_LOG,
            'Expected the Agenda portrait100 native capture failure, not a build or other failure')
    error = result.get('error', '')
    require(isinstance(error, str) and error
            and 'timeout' not in error.lower() and 'timed out' not in error.lower(),
            'A missing error or timeout is not the required assertion failure')
    require('SIGABRT' in error, 'Expected the native assertion to terminate the capture with SIGABRT')
    log = (output / CAPTURE_LOG).read_text(encoding='utf-8', errors='replace')
    assertion = r'agenda_shot\.c:\d+: captura: Assertion [\x60\x27\x22]vistos == 3 && inteiros == 3[\x60\x27\x22] failed'
    require(re.search(assertion, log) is not None,
            'Native capture did not fail on the exact uncut List/Month label assertion')
    return result


def run_negative(root):
    root = Path(root).resolve()
    source = root / 'src/agendaui.c'
    original = source.read_bytes()
    require(original.count(ALLOWANCE) == 1, 'Expected exactly one phone label allowance statement')
    output = root / 'build/phone-label-negative'
    # Refuse stale results; the preserved evidence must come from this run.
    output.mkdir(parents=True, exist_ok=False)
    mutant = original.replace(ALLOWANCE, DISABLED, 1)
    receipt = {
        'source': 'src/agendaui.c',
        'original_sha256': hashlib.sha256(original).hexdigest(),
        'mutation_count': 1,
        'expected_assertion': 'vistos == 3 && inteiros == 3',
        'case': CASE,
        'font': 'shipped Inter (0)',
        'language': 'Portuguese',
    }
    command = [sys.executable, 'tests/phone_shots.py', '--fixtures', 'agenda',
               '--cases', CASE, '--output', 'build/phone-label-negative']
    try:
        source.write_bytes(mutant)
        with (output / 'driver.log').open('w', encoding='utf-8') as stream:
            print('Command:', repr(command), file=stream, flush=True)
            environment = dict(os.environ, NUVIO_SHOT_FONTE='0', NUVIO_SHOT_EN='0')
            process = subprocess.run(command, cwd=root, env=environment,
                                     stdout=stream, stderr=subprocess.STDOUT)
            receipt['capture_driver_returncode'] = process.returncode
        receipt['capture_result'] = validate_failure(output, process.returncode)
        receipt['expected_native_failure_verified'] = True
    finally:
        source.write_bytes(original)
        restored = source.read_bytes()
        receipt['restored_sha256'] = hashlib.sha256(restored).hexdigest()
        receipt['source_restored_byte_for_byte'] = restored == original
        (output / 'negative-control.json').write_text(
            json.dumps(receipt, indent=2) + '\n', encoding='utf-8')
        require(restored == original, 'Agenda source did not restore byte-for-byte')
    print('PASS: zero-allowance native Agenda capture failed on the exact label '
          'assertion; source restored byte-for-byte. Independent positive captures are also required.')


if __name__ == '__main__':
    try:
        run_negative(Path(__file__).resolve().parents[1])
    except Exception as error:
        print(f'FAIL: native Agenda label negative control: {error}', file=sys.stderr)
        sys.exit(1)
