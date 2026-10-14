#!/usr/bin/env python3
"""Check frozen test-only overlay boundaries against actual source bytes."""
import hashlib
import json
from pathlib import Path

MARKERS = (b'NV_TEST_LOCAL_LOGS', b'TestLocalLogs', b'nuvio.testLocalLogs', b'[test-next]')

def source_files(root):
    root = Path(root)
    files = {}
    candidates = [f for prefix in ('src', 'android', 'tools', 'docs')
                  for f in (root / prefix).rglob('*') if f.is_file()]
    if (root / 'README.md').is_file():
        candidates.append(root / 'README.md')
    for path in candidates:
        relative = path.relative_to(root)
        if any(part in {'.git', 'build', '__pycache__', '.gradle', '.cxx'} for part in relative.parts):
            continue
        files[str(relative)] = {'sha256': hashlib.sha256(path.read_bytes()).hexdigest(),
                                'mode': '100755' if path.stat().st_mode & 0o111 else '100644'}
    return files

def assert_clean(root):
    for relative in source_files(root):
        data = (Path(root) / relative).read_bytes()
        assert not any(marker in data for marker in MARKERS), ('Test-only code entered a clean product tree', relative)

def load_isolation(path):
    path = Path(path)
    proof = json.loads(path.read_text())
    assert proof['schema'] == 1 and proof['test_only'] and proof['never_include_in_pr']
    assert proof['clean_products_checked'] and proof['isolated_delivery_copy']
    assert proof['overlay_changes'] and proof['standalone_files'] and proof['clean_combined_files']
    assert len({entry['path'] for entry in proof['overlay_changes']}) == len(proof['overlay_changes'])
    return proof, hashlib.sha256(path.read_bytes()).hexdigest()

def verify_tree(path, case, target):
    proof, digest = load_isolation(path)
    current = source_files(target)
    if case == 'standalone-preload':
        assert_clean(target)
        expected = proof['standalone_files']
    else:
        assert case == 'combined-delivery'
        expected = dict(proof['clean_combined_files'])
        for entry in proof['overlay_changes']:
            assert expected.get(entry['path']) == entry['before'], entry['path']
            expected[entry['path']] = entry['after']
        for relative in ('src/app.c', 'src/posplay.c', 'android/app/src/main/java/space/nuvio/nativelegacy/TestLocalLogs.kt'):
            assert relative in {entry['path'] for entry in proof['overlay_changes']}, relative
    assert current == expected, ('Frozen clean/overlay source boundary changed',
        sorted(set(current) ^ set(expected)),
        [relative for relative in current.keys() & expected.keys() if current[relative] != expected[relative]])
    return {'outcome': 'passed', 'test_only': True, 'case': case,
            'isolation_manifest_sha256': digest,
            'clean_pr_candidate': case == 'standalone-preload',
            'test_overlay_present': case == 'combined-delivery',
            'verified_source_files': len(current),
            'overlay_paths': [entry['path'] for entry in proof['overlay_changes']] if case == 'combined-delivery' else []}
