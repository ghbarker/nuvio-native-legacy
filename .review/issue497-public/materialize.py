"""Create a clean detached worktree from the exact approved source commit."""
from pathlib import Path
import argparse,json,os,subprocess
from common import HERE,approved,sha,git
p=argparse.ArgumentParser()
p.add_argument('--target',type=Path,required=True);p.add_argument('--proof',type=Path,required=True)
p.add_argument('--reverify',action='store_true');a=p.parse_args()
plan=approved();m=json.loads((HERE/'source-manifest.json').read_text());target=a.target.resolve()
assert m['head']==plan['source_commit'] and m['tree']==plan['source_tree']
assert m['base']==plan['base'] and m['parents']==[m['base']]
if not a.reverify:
 assert not target.exists()
 subprocess.run(['git','cat-file','-e',m['head']+'^{commit}'],check=True)
 subprocess.run(['git','worktree','add','--detach',str(target),m['head']],check=True)
assert git(target,'rev-parse','HEAD')==m['head'] and git(target,'rev-parse','HEAD^{tree}')==m['tree']
assert git(target,'rev-list','--parents','-n','1','HEAD').split()==[m['head'],m['base']]
assert git(target,'rev-parse',m['base']+'^{tree}')==m['base_tree']
assert not git(target,'status','--porcelain')
assert git(target,'diff','--name-only',m['base'],m['head']).splitlines()==sorted(e['path']for e in m['paths'])
assert len(m['paths'])==4 and all(e['path'].startswith('src/')for e in m['paths'])
for e in m['paths']:assert sha(target/e['path'])==e['sha256']
v={'verified':True,'source_clean':True,'product_only':True,'source_commit':m['head'],'tree':m['tree'],'source_parents':m['parents'],'base':m['base'],'review_commit':os.environ['GITHUB_SHA'],'workflow_run_id':int(os.environ['GITHUB_RUN_ID']),'workflow_run_attempt':int(os.environ['GITHUB_RUN_ATTEMPT']),'manifest_sha256':sha(HERE/'source-manifest.json'),'product_paths':4,'tests_unchanged':True,'all_non_source_paths_unchanged':True}
a.proof.parent.mkdir(parents=True,exist_ok=True);a.proof.write_text(json.dumps(v,indent=2)+'\n')
print('Verified exact #497 four-source-only candidate',m['head'],flush=True)
