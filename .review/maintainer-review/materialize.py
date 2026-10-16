"""Materialize only the immutable candidate selected by the current matrix row."""
from pathlib import Path
import argparse,json,os,subprocess
from common import HERE,approved,sha,git
p=argparse.ArgumentParser();p.add_argument('--candidate',required=True);p.add_argument('--target',type=Path,required=True);p.add_argument('--proof',type=Path,required=True);p.add_argument('--reverify',action='store_true');a=p.parse_args()
plan=approved();m=json.loads((HERE/'source-candidates.json').read_text())[a.candidate];target=a.target.resolve()
assert a.candidate==os.environ['REBASE_PR']and a.candidate in plan['selected_candidates']
if not a.reverify:
 assert not target.exists()
 subprocess.run(['git','cat-file','-e',m['head']+'^{commit}'],check=True)
 subprocess.run(['git','worktree','add','--detach',str(target),m['head']],check=True)
assert git(target,'rev-parse','HEAD')==m['head']and git(target,'rev-parse','HEAD^{tree}')==m['tree']
assert git(target,'rev-list','--parents','-n','1','HEAD').split()==[m['head'],*m['parents']]
assert git(target,'rev-parse',m['base']+'^{tree}')==m['base_tree']
subprocess.run(['git','-C',str(target),'merge-base','--is-ancestor',m['base'],m['head']],check=True)
assert not git(target,'status','--porcelain')
assert git(target,'diff','--name-only',m['base'],m['head']).splitlines()==m['exact_diff_paths']
for e in m['paths']+m['reviewed_test_paths']:
 assert git(target,'rev-parse','HEAD:'+e['path'])==e['blob']
 if 'sha256'in e:assert sha(target/e['path'])==e['sha256']
v={'verified':True,'source_clean':True,'candidate_pr':int(a.candidate),'source_commit':m['head'],'tree':m['tree'],'source_parents':m['parents'],'base':m['base'],'review_commit':os.environ['GITHUB_SHA'],'workflow_run_id':int(os.environ['GITHUB_RUN_ID']),'workflow_run_attempt':int(os.environ['GITHUB_RUN_ATTEMPT']),'manifest_sha256':sha(HERE/'source-candidates.json'),'reviewed_test_paths':[e['path']for e in m['reviewed_test_paths']]}
a.proof.parent.mkdir(parents=True,exist_ok=True);a.proof.write_text(json.dumps(v,indent=2)+'\n')
print('Verified exact rebased candidate',a.candidate,m['head'],flush=True)
