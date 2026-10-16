"""Run the committed Android JVM policy tests after the genuine Gradle build."""
from pathlib import Path
import argparse, json, os, subprocess, sys
from common import HERE, sha, source_binding, record

p = argparse.ArgumentParser()
for name in ['source', 'proof', 'output']:
    p.add_argument('--' + name, type=Path, required=True)
a = p.parse_args()
s = a.source.resolve()
o = a.output.resolve()
o.mkdir(parents=True, exist_ok=False)
plan, binding, manifest = source_binding(s, a.proof)
java = Path(os.environ['JAVA_HOME']) / 'bin/java'
assert java.is_file()
version = subprocess.check_output([str(java), '-version'], stderr=subprocess.STDOUT, text=True)
assert 'version "17' in version
rows = []
for suite in manifest['android_public_suites']:
    assert suite['script'].startswith('tests/') and sha(s / suite['script']) == suite['script_sha256']
    for name, digest in suite['inputs'].items():
        assert sha(s / name) == digest
    row = record(o, suite['name'], [sys.executable, suite['script']], cwd=s, timeout=600)
    output = (o / row['log']).read_text()
    for marker in suite['required_markers']:
        assert output.splitlines().count(marker) == 1, marker
    row.update(source_binding=binding, test_script_sha256=sha(s / suite['script']),
               input_sha256=suite['inputs'], required_markers=suite['required_markers'])
    rows.append(row)
source_binding(s, a.proof)
(o / 'receipt.json').write_text(json.dumps({'passed': True, 'source_binding': binding,
    'plan_sha256': sha(HERE / 'plan.json'), 'java_version': version,
    'java_sha256': sha(java), 'groups': rows,
    'scope': 'Exact committed public JVM test script, its embedded assertions and production Kotlin inputs. Actual Java17 and the real cached Gradle Kotlin compiler; no source extraction or test rewrite.'}, indent=2) + '\n')
print('PASS committed Android JVM policy tests:', len(rows), flush=True)
