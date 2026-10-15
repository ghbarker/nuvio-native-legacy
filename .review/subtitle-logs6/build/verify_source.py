#!/usr/bin/env python3
"""Verify a pinned product tree independently of review/build tooling."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
from verify_test_isolation import verify_tree

ROOT = Path(__file__).resolve().parent

def load_plan():
    plan = json.loads((ROOT / 'plans.json').read_text())
    assert plan['schema'] == 2 and plan['validation_phase'] in ('compile', 'final')
    cases = plan['cases']
    assert len(cases) >= 2
    assert len({case['id'] for case in cases}) == len(cases)
    assert sum(bool(case['delivery']) for case in cases) <= 1
    for case in cases:
        assert re.fullmatch(r'[a-z0-9-]+', case['id'])
        assert re.fullmatch(r'[0-9a-f]{40}', case['base'])
        assert re.fullmatch(r'[0-9a-f]{64}', case['patch_sha256']) and case['patch']
        assert case['manifest'] and case['label']
    assert not plan['publish_release'] or (plan['validation_phase'] == 'final' and sum(bool(c['delivery']) for c in cases) == 1)
    if plan.get('media3_case'):
        assert plan['media3_case'] in {c['id'] for c in cases}
    if plan['publish_release']:
        delivery_case = next(c for c in cases if c['delivery'])
        assert plan.get('media3_case', delivery_case['id']) == delivery_case['id'], 'Final release must repeat Media3 tests on the full delivery source'
    return plan

def blob(data):
    return hashlib.sha1(b'blob ' + str(len(data)).encode() + b'\0' + data).hexdigest()

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--matrix', action='store_true')
    parser.add_argument('--case')
    parser.add_argument('--source-commit')
    parser.add_argument('--target', type=Path)
    parser.add_argument('--proof', type=Path)
    parser.add_argument('--reverify', action='store_true')
    args = parser.parse_args()
    plan = load_plan()
    if args.matrix:
        result = {'matrix': {'include': plan['cases']}, 'phase': plan['validation_phase'],
                  'publish': plan['publish_release'], 'delivery': next((c for c in plan['cases'] if c['delivery']), None)}
        print(json.dumps(result, separators=(',', ':')))
        return
    case = next(c for c in plan['cases'] if c['id'] == args.case)
    source_commit = args.source_commit
    assert isinstance(source_commit, str) and re.fullmatch(r'[0-9a-f]{40}', source_commit)
    manifest = json.loads((ROOT / case['manifest']).read_text())
    base = manifest.get('base', manifest.get('upstream_commit'))
    assert base == case['base']
    assert len({e['path'] for e in manifest['files']}) == len(manifest['files']) > 0
    target = args.target.resolve()
    if not args.reverify:
        assert not target.exists()
        if subprocess.run(['git', 'cat-file', '-e', source_commit], capture_output=True).returncode:
            subprocess.run(['git', 'fetch', 'origin', source_commit], check=True)
        subprocess.run(['git', 'worktree', 'add', '--detach', str(target), source_commit], check=True)
    assert subprocess.check_output(['git', '-C', str(target), 'rev-parse', 'HEAD'], text=True).strip() == source_commit
    subprocess.run(['git', '-C', str(target), 'diff', '--exit-code', source_commit, '--'], check=True)
    parents = subprocess.check_output(['git', '-C', str(target), 'rev-list', '--parents', '-n', '1', source_commit], text=True).split()
    assert parents == [source_commit, base], parents
    changed = subprocess.check_output(['git', '-C', str(target), 'diff', '--name-only', '-z', base, source_commit]).decode().split('\0')
    changed = sorted(path for path in changed if path)
    assert changed == sorted(entry['path'] for entry in manifest['files']), (changed, [e['path'] for e in manifest['files']])
    verified = []
    for entry in manifest['files']:
        path = entry['path']
        assert '..' not in Path(path).parts and not Path(path).is_absolute()
        assert not path.startswith(('.review/', '.github/')), path
        file = target / path
        actual = blob(file.read_bytes())
        expected = subprocess.check_output(['git', '-C', str(target), 'rev-parse', source_commit + ':' + path], text=True).strip()
        assert actual == expected == entry['sha'], (path, actual, expected, entry['sha'])
        assert bool(file.stat().st_mode & 0o111) == (entry['mode'] == '100755'), path
        verified.append({'path': path, 'sha': actual, 'mode': entry['mode']})
    proof = {'verified': True, 'base': base, 'source_commit': source_commit,
             'build_commit': os.environ.get('GITHUB_SHA'), 'case': case['id'],
             'phase': plan['validation_phase'], 'label': case['label'],
             'scope': case.get('scope', 'product candidate'),
             'feature_changed_paths': changed, 'files': verified}
    args.proof.parent.mkdir(parents=True, exist_ok=True)
    args.proof.write_text(json.dumps(proof, indent=2) + '\n')
    isolation = verify_tree(ROOT / 'test-log-isolation.json', case['id'], target)
    isolation.update(source_commit=source_commit, review_commit=os.environ.get('GITHUB_SHA'))
    args.proof.with_name('test-log-isolation-verified.json').write_text(json.dumps(isolation, indent=2) + '\n')
    print('Pinned product tree verified:', case['id'], source_commit, len(verified), 'files')

if __name__ == '__main__':
    main()
