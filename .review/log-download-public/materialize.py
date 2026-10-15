#!/usr/bin/env python3
"""Reconstruct and verify the exact reviewed product commit without ref writes."""
import argparse, hashlib, json, os, subprocess
from pathlib import Path

HERE = Path(__file__).resolve().parent
M = json.loads((HERE / 'source-manifest.json').read_text())
p = argparse.ArgumentParser()
p.add_argument('--target', type=Path, required=True)
p.add_argument('--proof', type=Path, required=True)
p.add_argument('--reverify', action='store_true')
a = p.parse_args()
target = a.target.resolve()
def git(*args, cwd=None, data=None):
    return subprocess.check_output(['git', *args], cwd=cwd, input=data)
def text(*args, cwd=None): return git(*args, cwd=cwd).decode().strip()
def sha(data): return hashlib.sha256(data).hexdigest()
assert M['base'] == '3c28c0d73664fe82fabced499bd7171810e0c91b'
assert M['head'] == '1cd707586a5bc94579904e5601de996e62326067'
patch = (HERE / 'product-delta.patch').read_bytes()
commit = (HERE / 'product-commit.txt').read_bytes()
assert sha(patch) == M['patch_sha256']
assert hashlib.sha1(b'commit ' + str(len(commit)).encode() + b'\0' + commit).hexdigest() == M['head']
if not a.reverify:
    assert not target.exists()
    if subprocess.run(['git', 'cat-file', '-e', M['base']], capture_output=True).returncode:
        subprocess.run(['git', 'fetch', 'https://github.com/iqui27/nuvio-native-legacy.git', M['base']], check=True)
    subprocess.run(['git', 'worktree', 'add', '--detach', str(target), M['base']], check=True)
    subprocess.run(['git', 'apply', '--check', '--index', str(HERE / 'product-delta.patch')], cwd=target, check=True)
    subprocess.run(['git', 'apply', '--index', str(HERE / 'product-delta.patch')], cwd=target, check=True)
    assert text('write-tree', cwd=target) == M['tree']
    assert git('hash-object', '-w', '-t', 'commit', '--stdin', cwd=target, data=commit).decode().strip() == M['head']
    subprocess.run(['git', 'reset', '--hard', M['head']], cwd=target, check=True)
assert text('rev-parse', 'HEAD', cwd=target) == M['head']
assert text('rev-parse', 'HEAD^{tree}', cwd=target) == M['tree']
assert text('rev-list', '--parents', '-n', '1', 'HEAD', cwd=target).split() == [M['head'], M['base']]
assert text('diff', '--name-only', M['base'], M['head'], cwd=target).splitlines() == [e['path'] for e in M['paths']]
subprocess.run(['git', 'diff', '--exit-code', 'HEAD', '--'], cwd=target, check=True)
for e in M['paths']:
    data = (target / e['path']).read_bytes()
    assert sha(data) == e['sha256']
    assert text('rev-parse', 'HEAD:' + e['path'], cwd=target) == e['blob']
unchanged = ['src/avisos.c', 'src/avisos.h', 'src/registro.c', 'src/registro.h',
             'src/queda.c', 'src/recomenda.c', 'src/recomenda.h', 'tools/env.sh',
             'android/app/build.gradle.kts', 'android/app/src/main/AndroidManifest.xml',
             'android/app/src/main/java/space/nuvio/nativelegacy/NvPlayer.kt']
for path in unchanged:
    assert (target / path).read_bytes() == git('show', M['base'] + ':' + path, cwd=target)
for tree in ('tests', '.github'):
    assert text('rev-parse', M['base'] + ':' + tree, cwd=target) == text('rev-parse', 'HEAD:' + tree, cwd=target)
proof = {'verified': True, 'source_commit': M['head'], 'base': M['base'], 'tree': M['tree'],
         'review_commit': os.environ.get('GITHUB_SHA', 'local-materialization-preflight'),
         'product_paths': len(M['paths']), 'source_manifest_sha256': sha((HERE / 'source-manifest.json').read_bytes()),
         'original_reporting_viewer_crash_backend_build_identity_unchanged': unchanged,
         'tests_and_workflows_unchanged': True, 'product_commit_recreated_exactly': True}
a.proof.parent.mkdir(parents=True, exist_ok=True)
a.proof.write_text(json.dumps(proof, indent=2) + '\n')
print('Verified exact public log-download source:', M['head'], len(M['paths']), 'product paths; no ref writes')
