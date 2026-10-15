from pathlib import Path
import argparse,hashlib,json,os,subprocess
from common import HERE,approved,sha,git
p=argparse.ArgumentParser();p.add_argument('--target',type=Path,required=True);p.add_argument('--proof',type=Path,required=True);p.add_argument('--reverify',action='store_true');p.add_argument('--preflight',action='store_true');a=p.parse_args()
if not a.preflight:approved()
assert not(a.preflight and os.environ.get('GITHUB_ACTIONS')=='true')
m=json.loads((HERE/'source-manifest.json').read_text());target=a.target.resolve();assert len(m['chain'])==2
if not a.reverify:
 assert not target.exists();subprocess.run(['git','cat-file','-e',m['base']],check=True)
 subprocess.run(['git','worktree','add','--detach',str(target),m['base']],check=True)
 for step in m['chain']:
  patch=HERE/step['patch'];commit=HERE/step['commit'];data=commit.read_bytes()
  assert sha(patch)==step['patch_sha256'] and sha(commit)==step['commit_sha256']
  assert hashlib.sha1(b'commit '+str(len(data)).encode()+b'\0'+data).hexdigest()==step['head']
  assert git(target,'rev-parse','HEAD')==step['parent']
  subprocess.run(['git','apply','--check','--index',str(patch)],cwd=target,check=True)
  subprocess.run(['git','apply','--index',str(patch)],cwd=target,check=True)
  assert git(target,'write-tree')==step['tree']
  assert subprocess.check_output(['git','hash-object','-w','-t','commit','--stdin'],cwd=target,input=data).decode().strip()==step['head']
  subprocess.run(['git','reset','--hard',step['head']],cwd=target,check=True)
assert git(target,'rev-parse','HEAD')==m['head'] and git(target,'rev-parse','HEAD^{tree}')==m['tree']
assert git(target,'rev-list','--reverse',m['base']+'..HEAD').splitlines()==[x['head']for x in m['chain']]
assert git(target,'merge-base',m['base'],'HEAD')==m['base']
for step in m['chain']:assert git(target,'rev-list','--parents','-n','1',step['head']).split()==[step['head'],step['parent']]
assert not git(target,'status','--porcelain')
assert git(target,'diff','--name-only',m['base'],m['head']).splitlines()==[e['path']for e in m['paths']]
assert len(m['paths'])==52 and all(e['path'].startswith('src/')for e in m['paths'])
for e in m['paths']:assert sha(target/e['path'])==e['sha256'] and git(target,'rev-parse','HEAD:'+e['path'])==e['blob']
for name in ['tests','.github','android','tools','deploy']:assert git(target,'rev-parse','HEAD:'+name)==git(target,'rev-parse',m['base']+':'+name)
scope=json.loads((HERE/'cumulative-pr-scope.json').read_text());assert scope['source_commit']==m['head'] and scope['tree']==m['tree'] and scope['base']==m['base']
assert scope['paths']==[e['path']for e in m['paths']]and scope['changed_paths']==52
v={'verified':True,'source_commit':m['head'],'tree':m['tree'],'base':m['base'],'review_commit':os.environ.get('GITHUB_SHA','local-preflight'),'manifest_sha256':sha(HERE/'source-manifest.json'),'product_paths':len(m['paths']),'product_only':True,'tests_workflows_android_tools_deploy_unchanged':True,'source_commit_chain':[x['head']for x in m['chain']],'cumulative_base':scope['base'],'cumulative_product_paths':52,'cumulative_scope_sha256':sha(HERE/'cumulative-pr-scope.json')}
a.proof.parent.mkdir(parents=True,exist_ok=True);a.proof.write_text(json.dumps(v,indent=2)+'\n');print('Verified exact two-commit synopsis source',m['head'],flush=True)
