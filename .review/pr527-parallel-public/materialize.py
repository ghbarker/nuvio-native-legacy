"""Materialize the exact reviewed public source objects, never mutable PR refs."""
from pathlib import Path
import argparse,base64,hashlib,json,os,subprocess,tempfile
from common import HERE,approved,sha,git
p=argparse.ArgumentParser()
p.add_argument('--target',type=Path,required=True);p.add_argument('--proof',type=Path,required=True)
p.add_argument('--reverify',action='store_true');p.add_argument('--preflight',action='store_true');a=p.parse_args()
if not a.preflight:approved()
assert not(a.preflight and os.environ.get('GITHUB_ACTIONS')=='true')
m=json.loads((HERE/'source-manifest.json').read_text());target=a.target.resolve()
bundle=base64.b64decode((HERE/'source.bundle.base64').read_bytes(),validate=False)
assert hashlib.sha256(bundle).hexdigest()==m['bundle_sha256']and len(bundle)==m['bundle_bytes']
if not a.reverify:
 assert not target.exists()
 subprocess.run(['git','cat-file','-e',m['base']+'^{commit}'],check=True)
 with tempfile.TemporaryDirectory(prefix='pr527-public-source-')as directory:
  path=Path(directory)/'source.bundle';path.write_bytes(bundle)
  subprocess.run(['git','bundle','verify',str(path)],check=True)
  subprocess.run(['git','fetch','--no-write-fetch-head','--no-tags',str(path),'refs/review/pr527-public-exact'],check=True)
 subprocess.run(['git','worktree','add','--detach',str(target),m['head']],check=True)
assert git(target,'rev-parse','HEAD')==m['head']and git(target,'rev-parse','HEAD^{tree}')==m['tree']
assert git(target,'rev-list','--parents','-n','1','HEAD').split()==[m['head'],*m['parents']]
subprocess.run(['git','-C',str(target),'merge-base','--is-ancestor',m['base'],m['head']],check=True)
assert not git(target,'status','--porcelain')
assert git(target,'diff','--name-only',m['base'],m['head']).splitlines()==sorted(e['path']for e in m['paths']+m['reviewed_test_compatibility'])
assert [e['path']for e in m['reviewed_test_compatibility']]==['tests/colecao_exibicao.c']
assert all(e['path'].startswith('src/')for e in m['paths'])
for e in m['paths']+m['reviewed_test_compatibility']:assert sha(target/e['path'])==e['sha256']and git(target,'rev-parse','HEAD:'+e['path'])==e['blob']
for entry in m['commits_after_base']:
 assert git(target,'rev-parse',entry['commit']+'^{tree}')==entry['tree']
 assert git(target,'rev-list','--parents','-n','1',entry['commit']).split()==[entry['commit'],*entry['parents']]
for name in ['.github','android','tools','deploy']:assert git(target,'rev-parse','HEAD:'+name)==git(target,'rev-parse',m['base']+':'+name)
v={'verified':True,'source_commit':m['head'],'tree':m['tree'],'source_parent':m['parent'],'source_parent_tree':m['parent_tree'],'source_parents':m['parents'],'base':m['base'],'review_commit':os.environ.get('GITHUB_SHA','local-preflight'),'manifest_sha256':sha(HERE/'source-manifest.json'),'bundle_sha256':m['bundle_sha256'],'product_paths':len(m['paths']),'product_only':True,'exact_ancestry_bound':True,'base_is_ancestor':True,'non_test_non_source_trees_unchanged':True,'tests_unchanged_except_reviewed_paths':True,'reviewed_test_compatibility':m['reviewed_test_compatibility']}
a.proof.parent.mkdir(parents=True,exist_ok=True);a.proof.write_text(json.dumps(v,indent=2)+'\n');print('Verified exact current-upstream public source',m['head'],flush=True)
