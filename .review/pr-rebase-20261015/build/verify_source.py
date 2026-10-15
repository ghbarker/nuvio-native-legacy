#!/usr/bin/env python3
"""Materialize one reviewed rebased PR and verify its exact source and original tests."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess

REVIEW = Path(__file__).resolve().parents[1]

def git(*args, cwd=None):
    return subprocess.check_output(['git', *args], cwd=cwd, text=True).strip()

def plan(case):
    value = json.loads((REVIEW / 'cases.json').read_text())
    return value, next(c for c in value['cases'] if c['id'] == case)

def verify(root, value, case):
    root = root.resolve()
    assert git('rev-parse', 'HEAD', cwd=root) == case['head']
    assert git('rev-parse', 'HEAD^{tree}', cwd=root) == case['tree']
    assert git('rev-parse', 'HEAD^', cwd=root) == value['base']
    subprocess.run(['git', 'diff', '--no-ext-diff', '--exit-code', 'HEAD', '--'], cwd=root, check=True)
    changed = git('diff', '--name-only', '-z', value['base'], case['head'], cwd=root).split('\0')
    changed = sorted(p for p in changed if p)
    assert changed == sorted(f['path'] for f in case['files']), (changed, case['files'])
    assert not any(p.startswith(('tests/', '.review/', '.github/')) for p in changed)
    for entry in case['files']:
        p = root / entry['path']
        data = p.read_bytes()
        blob = hashlib.sha1(b'blob ' + str(len(data)).encode() + b'\0' + data).hexdigest()
        assert blob == entry['gitBlob']
        assert hashlib.sha256(data).hexdigest() == entry['sha256']
        assert bool(p.stat().st_mode & 0o111) == (entry['mode'] == '100755')
    original = {}
    for p, expected in case['original_tests'].items():
        actual = hashlib.sha256((root / p).read_bytes()).hexdigest()
        assert actual == expected, (p, expected, actual)
        original[p] = actual
    return {'case': case['id'], 'pr': case['pr'], 'base': value['base'], 'source_commit': case['head'],
            'source_tree': case['tree'], 'review_commit': os.environ.get('GITHUB_SHA'),
            'source_verified': True, 'product_changed_paths': changed, 'original_test_sha256': original}

def main():
    p = argparse.ArgumentParser()
    p.add_argument('--case', required=True)
    p.add_argument('--target', type=Path, required=True)
    p.add_argument('--proof', type=Path, required=True)
    p.add_argument('--reverify', action='store_true')
    a = p.parse_args()
    value, case = plan(a.case)
    if not a.reverify:
        assert not a.target.exists(), a.target
        subprocess.run(['git', 'worktree', 'add', '--detach', str(a.target.resolve()), case['head']], check=True)
    proof = verify(a.target, value, case)
    a.proof.parent.mkdir(parents=True, exist_ok=True)
    a.proof.write_text(json.dumps(proof, indent=2) + '\n')
    print('Pinned source and unchanged original tests verified:', a.case, case['head'])

if __name__ == '__main__':
    main()
