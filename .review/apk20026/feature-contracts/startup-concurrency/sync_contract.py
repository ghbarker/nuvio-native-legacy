"""Run the unchanged actual-sync latch assertions with recorded offline boundaries."""
from pathlib import Path
import hashlib
import json
import os
import subprocess

ROOT = Path(__file__).resolve().parent
SCENARIOS = {
    'concurrent': [
        'PASS: account order and collections applied while progress remains blocked',
        'PASS: progress applied once, joined before pushes, structure consumed once',
    ],
    'stale': ['PASS: A -> B -> A rejects stale progress and structural readiness'],
    'thread-failure': ['PASS: progress-thread creation failure preserves serial correctness'],
}


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def run(source, output, compiler, private, sanitize):
    output.mkdir()
    env = dict(os.environ, CC=str(compiler), NUVIO_SYNC_STARTUP_RESULTS=str(output),
               SANITIZE=str(int(sanitize)), EXPECT_BASELINE='0',
               NV_REVIEW_PRIVATE=str(int(private)),
               ASAN_OPTIONS='detect_leaks=0:halt_on_error=1',
               UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
    command = ['bash', '-x', str(ROOT / 'sync/run.sh'), str(source), str(ROOT / 'sdk')]
    result = subprocess.run(command, env=env, text=True, capture_output=True, timeout=180)
    log = output / 'producer.log'
    log.write_text(result.stdout + result.stderr)
    assert result.returncode == 0, (str(log), result.stdout[-2000:], result.stderr[-2000:])
    trace = log.read_text()
    assert not any(marker in trace for marker in ['ERROR: AddressSanitizer', 'runtime error:', 'UNEXPECTED dependency:'])
    assert ('-DNV_TEST_LOCAL_LOGS=1' in trace) is private
    assert ('-fsanitize=address,undefined' in trace) is sanitize
    assert str(compiler) in trace and str(source / 'src/sync.c') in trace
    receipt_path = output / 'receipt.json'
    raw = json.loads(receipt_path.read_text())
    assert raw['passed'] is True and raw['baseline'] is False
    assert raw['sanitize'] is sanitize and raw['source_sha256'] == sha(source / 'src/sync.c')
    # The wrapper's producer.log is written after the shell's raw receipt; only
    # the three assertion scenario logs belong to that receipt's scenario map.
    assert set(raw['scenarios']) == set(SCENARIOS)
    logs = {}
    for name, markers in SCENARIOS.items():
        path = output / (name + '.log')
        assert sha(path) == raw['scenarios'][name]
        text = path.read_text()
        assert all(marker in text for marker in markers), name
        assert 'REPRODUCED:' not in text and 'FALHOU' not in text
        logs[name + '.log'] = sha(path)
    return {
        'kind': 'sync', 'passed': True, 'returncode': result.returncode,
        'command': command, 'runner_relative': 'sync/run.sh',
        'runner_sha256': sha(ROOT / 'sync/run.sh'),
        'private': private, 'sanitize': sanitize,
        'scenarios': list(SCENARIOS), 'scenario_logs_sha256': logs,
        'raw_receipt_relative': 'receipt.json', 'raw_receipt_sha256': sha(receipt_path),
        'producer_log_relative': 'producer.log', 'producer_log_sha256': sha(log),
        'binary_relative': 'latch', 'binary_sha256': sha(output / 'latch'),
        'object_relative': 'sync.o', 'object_sha256': sha(output / 'sync.o'),
        'source_inputs_sha256': {str(path.relative_to(source)): sha(path)
                                for path in [source / 'src' / (name + '.c')
                                             for name in ['sync', 'catordem', 'catordemcache',
                                                          'contacache', 'js', 'jsw']]},
        'test_inputs_sha256': {relative: sha(source / relative)
                              for relative in ['tests/stub_contapend.c', 'tests/syncordem.c']},
    }
