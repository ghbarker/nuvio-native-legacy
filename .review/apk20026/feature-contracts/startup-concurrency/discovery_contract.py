"""Run and bind the actual discovery/CW/social latch producer."""
from pathlib import Path
import hashlib
import json
import os
import subprocess
import sys

ROOT = Path(__file__).resolve().parent
SCENARIOS = ['fresh', 'cached', 'clear', 'stale']


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def run(source, output, compiler, private, sanitize):
    output.mkdir()
    env = dict(os.environ, CC=str(compiler), NV_TEST_LOCAL_LOGS=str(int(private)))
    command = [sys.executable, str(ROOT / 'discovery/run.py'), '--source', str(source),
               '--out', str(output)] + (['--sanitize'] if sanitize else [])
    result = subprocess.run(command, env=env, text=True, capture_output=True, timeout=300)
    producer_log = output / 'producer.log'
    producer_log.write_text(result.stdout + result.stderr)
    assert result.returncode == 0, (str(producer_log), result.stdout[-3000:], result.stderr[-3000:])
    raw_path = output / 'receipt.json'
    raw = json.loads(raw_path.read_text())
    assert raw['all_passed'] is True and raw['fixed'] is True
    assert raw['private'] is private and raw['sanitized'] is sanitize
    assert [record['name'] for record in raw['records']] == ['compile', *SCENARIOS]
    assert raw['source_sha256'] == {relative: sha(source / relative)
                                    for relative in ['src/descoberta.c', 'src/catalogo.c', 'src/perfis.c']}
    compilation = raw['records'][0]['command']
    assert compilation[0] == str(compiler)
    assert ('-DNV_TEST_LOCAL_LOGS=1' in compilation) is private
    assert ('-fsanitize=address,undefined' in compilation) is sanitize
    assert '-DDISCOVERY_SOURCE="' + str(source / 'src/descoberta.c') + '"' in compilation
    assert 'src/perfis.c' in compilation
    logs = {}
    for record in raw['records']:
        assert record['exit_code'] == 0
        log = output / record['log']
        assert log.parent == output and sha(log) == record['log_sha256']
        text = log.read_text()
        assert not any(marker in text for marker in ['ERROR: AddressSanitizer', 'runtime error:', 'UNEXPECTED dependency:'])
        if record['name'] != 'compile':
            assert record['command'] == [str(output / 'discovery-latch'), '1', record['name']]
            assert 'CATALOG_AVAILABLE_WHILE_CW_AND_SOCIAL_BLOCKED' in text
            if record['name'] == 'stale':
                assert 'STALE_PROFILE_CW_AND_SOCIAL_REJECTED' in text
            else:
                assert 'LATCH PASS mode=' + record['name'] + ' fixed=1' in text
        logs[record['log']] = sha(log)
    assert sha(output / 'discovery-latch') == raw['binary_sha256']
    for name, expected in raw['fixture_sha256'].items():
        assert '/' not in name and sha(ROOT / 'discovery' / name) == expected
    return {
        'kind': 'discovery', 'passed': True, 'returncode': result.returncode,
        'command': command, 'runner_relative': 'discovery/run.py',
        'runner_sha256': sha(ROOT / 'discovery/run.py'),
        'private': private, 'sanitize': sanitize, 'scenarios': SCENARIOS,
        'scenario_logs_sha256': logs,
        'raw_receipt_relative': 'receipt.json', 'raw_receipt_sha256': sha(raw_path),
        'producer_log_relative': 'producer.log', 'producer_log_sha256': sha(producer_log),
        'binary_relative': 'discovery-latch', 'binary_sha256': sha(output / 'discovery-latch'),
        'source_inputs_sha256': {str(path.relative_to(source)): sha(path)
                                for path in [source / 'src' / (name + '.c')
                                             for name in ['descoberta', 'catalogo', 'perfis', 'cwordem',
                                                          'cotacat', 'js', 'metaprov', 'colecoes',
                                                          'redeurl', 'catordem']]},
    }
