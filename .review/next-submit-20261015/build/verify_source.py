#!/usr/bin/env python3
"""Pin the independent three-file Next candidate; never patch product files."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parent

def git(*arguments, cwd=None):
    return subprocess.check_output(['git', *arguments], cwd=cwd)

def plan():
    value = json.loads((ROOT / 'plan.json').read_text())
    assert value['schema'] == 1
    assert value['base_commit'] == '17b7769dac4273557e1b07f5fb8734df91dd718f'
    assert len(value['source_commit']) == 40 and all(c in '0123456789abcdef' for c in value['source_commit'])
    assert sorted(value['files']) == ['src/player.c', 'src/posplay.c', 'src/posplay.h']
    return value

def original_tests(target, value):
    verified = {}
    paths = ['tests/' + name + suffix for name in
             ('player_recuo', 'posplay_recuo', 'creditosauto', 'fontepref', 'fontepref_file', 'player_retido')
             for suffix in ('.c', '.sh')]
    paths.append('tests/posplay.c')
    for relative in paths:
        data = (target / relative).read_bytes()
        assert data == git('show', value['base_commit'] + ':' + relative, cwd=target), relative
        verified[relative] = hashlib.sha256(data).hexdigest()
    return verified

def verify(target, value):
    source = value['source_commit']
    assert git('rev-parse', 'HEAD', cwd=target).decode().strip() == source
    assert git('rev-list', '--parents', '-n', '1', source, cwd=target).decode().split() == [source, value['base_commit']]
    changed = sorted(path.decode() for path in git('diff', '--name-only', '-z', value['base_commit'], source, cwd=target).split(b'\0') if path)
    assert changed == sorted(value['files']), changed
    subprocess.run(['git', 'diff', '--exit-code', source, '--'], cwd=target, check=True)
    for relative, expected in value['files'].items():
        data = (target / relative).read_bytes()
        assert hashlib.sha256(data).hexdigest() == expected['sha256'], relative
        actual = hashlib.sha1(b'blob ' + str(len(data)).encode() + b'\0' + data).hexdigest()
        assert actual == expected['git_blob_sha1'] == git('rev-parse', source + ':' + relative, cwd=target).decode().strip(), relative
        assert not any(marker in data for marker in (b'NV_TEST_LOCAL_LOGS', b'[test-next]', b'proximopreparo_')), relative
    originals = original_tests(target, value)
    return {'verified': True, 'source_commit': source, 'base_commit': value['base_commit'],
            'review_commit': os.environ.get('GITHUB_SHA'), 'changed_paths': changed,
            'original_test_sha256': originals, 'source_scope': 'independent generic three-file Next fix'}

if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument('--target', type=Path, required=True)
    p.add_argument('--proof', type=Path, required=True)
    p.add_argument('--reverify', action='store_true')
    args = p.parse_args()
    value = plan()
    target = args.target.resolve()
    if not args.reverify:
        assert not target.exists(), target
        git('cat-file', '-e', value['source_commit'])
        subprocess.run(['git', 'worktree', 'add', '--detach', str(target), value['source_commit']], check=True)
    proof = verify(target, value)
    args.proof.parent.mkdir(parents=True, exist_ok=True)
    args.proof.write_text(json.dumps(proof, indent=2) + '\n')
    print('Verified exact clean Next source:', proof['source_commit'], 'and', len(proof['original_test_sha256']), 'unchanged original test files')
