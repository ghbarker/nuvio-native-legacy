#!/usr/bin/env python3
"""Materialize and reverify the exact seven-file subtitle candidate."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess

HERE = Path(__file__).resolve().parent


def plan():
    return json.loads((HERE / 'plan.json').read_text())


def verify(root):
    p = plan()
    root = Path(root).resolve()
    def git(*args):
        return subprocess.check_output(['git', '-C', str(root), *args], text=True).strip()
    assert git('rev-parse', 'HEAD') == p['source_commit']
    assert git('show', '-s', '--format=%P', 'HEAD') == p['base']
    assert git('rev-parse', 'HEAD^{tree}') == p['expected_tree']
    subprocess.run(['git', '-C', str(root), 'diff', '--exit-code', 'HEAD', '--'], check=True)
    subprocess.run(['git', '-C', str(root), 'diff', '--check', p['base'], 'HEAD'], check=True)
    manifest = json.loads((HERE / p['manifest']).read_text())
    changed = git('diff', '--name-only', p['base'], 'HEAD').splitlines()
    assert sorted(changed) == sorted(e['path'] for e in manifest['files']) and len(changed) == 7
    for entry in manifest['files']:
        path = entry['path']
        assert path.startswith('src/') and '..' not in Path(path).parts
        assert git('rev-parse', 'HEAD:' + path) == entry['sha']
        assert hashlib.sha256((root / path).read_bytes()).hexdigest() == entry['sha256']
        assert git('ls-tree', 'HEAD', '--', path).split()[0] == entry['mode']
    for path in ['tests', '.github']:
        assert git('rev-parse', 'HEAD:' + path) == git('rev-parse', p['base'] + ':' + path)
    return {'verified': True, 'case': p['case'], 'source_commit': p['source_commit'],
            'base': p['base'], 'tree': p['expected_tree'], 'review_commit': os.environ.get('GITHUB_SHA'),
            'feature_changed_paths': changed, 'all_original_tests_unchanged': True,
            'workflows_and_test_logging_excluded': True}


def main():
    a = argparse.ArgumentParser()
    a.add_argument('--case')
    a.add_argument('--target', type=Path, required=True)
    a.add_argument('--proof', type=Path, required=True)
    a.add_argument('--reverify', action='store_true')
    args = a.parse_args()
    if not args.reverify:
        assert not args.target.exists()
        source = plan()['source_commit']
        if subprocess.run(['git', 'cat-file', '-e', source], capture_output=True).returncode:
            subprocess.run(['git', 'fetch', 'origin', source], check=True)
        subprocess.run(['git', 'worktree', 'add', '--detach', str(args.target.resolve()), source], check=True)
    proof = verify(args.target)
    args.proof.parent.mkdir(parents=True, exist_ok=True)
    args.proof.write_text(json.dumps(proof, indent=2) + '\n')
    print('Exact clean subtitle source and unchanged tests verified:', proof['source_commit'])


if __name__ == '__main__':
    main()
