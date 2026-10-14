#!/usr/bin/env python3
import hashlib
import json
from pathlib import Path
import xml.etree.ElementTree as ET
from media3_contracts import load_contracts, ORIGINAL_TESTS, STARTUP_FILE

probe = Path('.review/source-preload-v2/android')
manifest, required, startup, classes, manifest_sha = load_contracts(probe / 'fixture-manifest.json')
reports = list((probe / 'probe/build/outputs/androidTest-results').rglob('*.xml'))
assert reports, 'No instrumentation XML results were produced'
passed, passed_classes = set(), {}
for report in reports:
    root = ET.parse(report).getroot()
    for test in root.iter('testcase'):
        assert all(test.find(tag) is None for tag in ('failure', 'error', 'skipped')), test.attrib
        name = test.attrib['name'].split('[')[0]
        passed.add(name)
        passed_classes.setdefault(test.attrib.get('classname', ''), set()).add(name)
assert required <= passed, (required, passed)
for klass, names in classes.items():
    assert set(names) <= passed_classes.get(klass, set()), (klass, names, passed_classes)
hashes = []
for entry in manifest['staged_player_helpers']:
    relative = entry['path']
    staged = (probe / 'probe/src/player' / relative).read_bytes()
    actual = (Path('source-app/android/app/src/main/java') / relative).read_bytes()
    assert staged == actual and hashlib.sha256(actual).hexdigest() == entry['sha256'], relative
    hashes.append({'path': relative, 'sha256': entry['sha256']})
for entry in manifest['helper_files']:
    assert hashlib.sha256((probe / entry['path']).read_bytes()).hexdigest() == entry['sha256'], entry['path']
startup_entry = next(entry for entry in manifest['helper_files'] if entry['path'].endswith('/StartupContractTest.kt'))
assert hashlib.sha256((probe / STARTUP_FILE).read_bytes()).hexdigest() == startup_entry['sha256'], 'Staged startup class differs from approved helper'
source = json.loads(Path('build/review/source-verified.json').read_text())
assert source['verified'] and source['phase'] == 'final' and source['case'] == 'combined-delivery'
proof = {'outcome': 'passed', 'source_commit': source['source_commit'], 'review_commit': source['build_commit'],
         'case': source['case'], 'source_scope': source['scope'],
         'real_media3_version': '1.8.0', 'emulator': 'API35 x86_64', 'required_tests': sorted(required),
         'original_v1_contracts': sorted(ORIGINAL_TESTS), 'startup_tests': sorted(startup),
         'required_test_classes': classes, 'test_classes_seen': {k: sorted(v) for k, v in passed_classes.items()},
         'fixture_manifest_sha256': manifest_sha, 'tests_seen': sorted(passed),
         'exact_staged_product_sources': hashes,
         'probe_scope': 'Real NvPlayer and Media3 with tiny Activity and JNI doubles; production FFmpeg decoder extension absent',
         'physical_device_startup_validation': 'pending'}
Path('build/review/media3-verified.json').write_text(json.dumps(proof, indent=2) + '\n')
print('All frozen Media3 contracts passed:', len(required), 'including', len(startup), 'startup checks; source', source['source_commit'])
