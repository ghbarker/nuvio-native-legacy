#!/usr/bin/env python3
"""Materialize the original Git object from a base-relative reviewed bundle."""
from pathlib import Path
import argparse, base64, hashlib, json, os, subprocess, tempfile
from common import HERE, git, plan, sha, verify

p = argparse.ArgumentParser(); p.add_argument('--output', type=Path, required=True)
p.add_argument('--proof', type=Path, required=True); p.add_argument('--base-repository', type=Path)
a = p.parse_args(); config = plan(); source = a.output.resolve()
assert not source.exists(), 'Refusing to replace an existing source'
entry = config['source']; manifest_path = HERE/entry['manifest']
assert sha(manifest_path) == entry['manifest_sha256']
m = json.loads(manifest_path.read_text())
assert m['commit'] == entry['commit'] and m['tree'] == entry['tree']
data = base64.b64decode((HERE/config['bundle']['path']).read_bytes())
assert len(data) == config['bundle']['bytes'] and hashlib.sha256(data).hexdigest() == config['bundle']['sha256']
def run(args, cwd=None):
    subprocess.run(args, cwd=cwd, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
if a.base_repository:
    run(['git','clone','--quiet','--shared','--no-checkout',str(a.base_repository.resolve()),str(source)])
else:
    source.mkdir(parents=True); run(['git','init','--quiet'], source)
try:
    run(['git','cat-file','-e',config['upstream']['commit']+'^{commit}'], source)
except subprocess.CalledProcessError:
    run(['git','fetch','--quiet','--depth=1',config['upstream']['url'],config['upstream']['commit']], source)
with tempfile.NamedTemporaryFile(prefix='pr431-exact-', suffix='.bundle') as f:
    f.write(data); f.flush(); run(['git','bundle','verify',f.name], source)
    run(['git','bundle','unbundle',f.name], source)
run(['git','checkout','--quiet','--detach',entry['commit']], source)
proof = {'verified':True,'layer':'product','source_commit':entry['commit'],'tree':entry['tree'],
 'base':config['upstream']['commit'],'review_commit':os.environ.get('GITHUB_SHA','local-review'),
 'manifest_sha256':entry['manifest_sha256'],'bundle_sha256':config['bundle']['sha256'],
 'source_clean':True,'original_tests_tree_unchanged':True,'review_only':False}
a.proof.parent.mkdir(parents=True, exist_ok=True)
a.proof.write_text(json.dumps(proof, indent=2)+'\n'); verify(source, a.proof)
print('Verified exact standalone PR431 source', entry['commit'], entry['tree'])
