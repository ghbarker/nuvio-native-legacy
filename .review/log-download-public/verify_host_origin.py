#!/usr/bin/env python3
"""Bind completed local proofs honestly; never call them a fresh CI runtime run."""
import argparse, hashlib, json
from pathlib import Path
p = argparse.ArgumentParser(); p.add_argument('--source', type=Path, required=True)
p.add_argument('--output', type=Path, required=True); a = p.parse_args()
here = Path(__file__).resolve().parent
origin = json.loads((here / 'host-proof/origin.json').read_text())
for record in origin['files']:
    data = (here / record['path']).read_bytes()
    assert len(data) == record['bytes'] and hashlib.sha256(data).hexdigest() == record['sha256']
k = json.loads((here / 'host-proof/kotlin-result.json').read_text())
assert k['status'] == 'passed' and k['passed_contracts'] == 26 and k['actual_product_unchanged_during_run']
assert all(r['exit_code'] == 0 for r in k['results'])
assert k['actual_product_sha256'] == hashlib.sha256((a.source / 'android/app/src/main/java/space/nuvio/nativelegacy/RegistroDownload.kt').read_bytes()).hexdigest()
for path, expected in k['fixture_sha256'].items():
    assert hashlib.sha256((here / 'host-proof' / path).read_bytes()).hexdigest() == expected
assert 'REGISTRO_DOWNLOAD_PREFLIGHT_OK 26' in (here / 'host-proof/kotlin-execute.log').read_text()
c = json.loads((here / 'host-proof/c-result.json').read_text())
assert c['outcome'] == 'passed' and c['baseline'] == origin['baseline']
assert c['original_reporting_viewer_crash_backend_and_identity_unchanged']
assert all(r['exit_code'] == 0 for r in c['records'])
for name, key in [('src/android.c', 'android_source_sha256'), ('src/ajustes.c', 'settings_source_sha256')]:
    assert hashlib.sha256((a.source / name).read_bytes()).hexdigest() == c[key]
origin['verified'] = True
origin['reexecuted_by_ci'] = False
a.output.parent.mkdir(parents=True, exist_ok=True)
a.output.write_text(json.dumps(origin, indent=2) + '\n')
print('Bound existing local preflight:26 JVM contracts,290 old Settings slots in4 modes,9 JNI cases normal/ASan; not rerun')
