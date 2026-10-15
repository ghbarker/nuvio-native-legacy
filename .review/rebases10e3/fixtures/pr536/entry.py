"""Fresh minimal #536 Settings-menu checks on an exact materialized rebase."""
from pathlib import Path
import argparse, hashlib, json, subprocess, sys

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parents[1]))
from common import source_binding

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--proof', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
p.add_argument('--cc', default='/usr/bin/cc')
a = p.parse_args()
s = a.source.resolve()
o = a.output.resolve()
o.mkdir(parents=True, exist_ok=False)
sha = lambda p: hashlib.sha256(Path(p).read_bytes()).hexdigest()
plan, proof, manifest = source_binding(s, a.proof)
origin = json.loads((HERE / 'provenance.json').read_text())
assert proof['source_commit'] == origin['candidate']
for path, digest in origin['assertion_fixtures_unchanged'].items():
    assert sha(HERE / path) == digest, path
schema = json.loads((HERE / 'schema-preservation.origin.json').read_text())
assert sha(HERE / 'schema-preservation.origin.json') == origin['schema_origin_sha256']
assert schema['passed'] and schema['candidate'] == proof['source_commit'] and schema['tree'] == proof['tree']
groups = []
for mode in ['normal', 'asan']:
    output = o / mode
    command = [sys.executable, str(HERE / 'run.py'), '--source', str(s), '--output', str(output),
               '--cc', a.cc, '--only', 'menu', *(['--sanitize'] if mode == 'asan' else [])]
    with (o / (mode + '.log')).open('w') as log:
        result = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, timeout=1200)
    if result.returncode:
        print('\n'.join((o / (mode + '.log')).read_text().splitlines()[-70:]), flush=True)
    assert result.returncode == 0, (mode, result.returncode)
    root = json.loads((output / 'receipt.json').read_text())
    raw = output / 'menu/receipt.json'
    report = json.loads(raw.read_text())
    assert root['passed'] and len(root['records']) == 1
    assert sha(raw) == root['records'][0]['sha256']
    assert report['passed'] and report['outcome'] == 'passed' and report['kind'] == 'menu'
    assert report['source_commit'] == proof['source_commit'] and report['source_tree'] == proof['tree']
    assert not report['private'] and not report['abort_traps_fired']
    assert report['sanitizers'] == (['address', 'undefined'] if mode == 'asan' else [])
    assert len(report['cases']) == 6 and all(c['returncode'] == c['actual_returncode'] == 0 for c in report['cases'])
    for path, digest in report['dependency_sha256'].items():
        assert sha(path) == digest, path
    for path, digest in root['root_logs_sha256'].items():
        assert sha(output / path) == digest, path
    groups.append({'name': 'settings-menu-' + mode, 'source_commit': report['source_commit'],
                   'source_tree': report['source_tree'], 'cases': len(report['cases']),
                   'receipt': str(raw.relative_to(o)), 'receipt_sha256': sha(raw),
                   'root_receipt': str((output / 'receipt.json').relative_to(o)),
                   'root_receipt_sha256': sha(output / 'receipt.json'),
                   'binary': str((output / 'menu/probe').relative_to(o)),
                   'binary_sha256': sha(output / 'menu/probe'),
                   'command': command, 'execution_log': mode + '.log',
                   'execution_log_sha256': sha(o / (mode + '.log'))})
source_binding(s, a.proof)
raw_files = {str(p.relative_to(o)): sha(p) for p in sorted(o.rglob('*'))
             if p.is_file() and (p.suffix in ['.json', '.log', '.d'] or p.name == 'probe')}
receipt = {'passed': True, 'outcome': 'passed', 'pr': 536, 'source_binding': proof,
           'source_commit': proof['source_commit'], 'source_tree': proof['tree'],
           'proof_sha256': sha(a.proof), 'groups': groups, 'group_count': 2,
           'fixture_provenance_sha256': sha(HERE / 'provenance.json'),
           'schema_origin_sha256': origin['schema_origin_sha256'],
           'raw_files_sha256': raw_files,
           'scope': 'Fresh actual Settings renderer, main/secondary persistence and pointer checks: six TV/mobile geometry/UI-scale cases per normal/ASan mode. Accepted assertions and typed offline companions retained byte-for-byte. Rebase schema proof is an explicitly local origin. Unchanged secondary/outline render suites are retained at their prior run; no replay or device claim.'}
(o / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
print('PASS #536 actual Settings menu: normal and ASan, six cases each', flush=True)
