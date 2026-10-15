"""Reject recovery drift; prove the retained Android build has identical inputs."""
from pathlib import Path
import ast, hashlib, json, os, subprocess
from common import HERE, approved, sha

plan=approved();root=Path(os.environ['GITHUB_WORKSPACE'])
workflow=json.loads((root/'.github/workflows/issue528-review.yml').read_text())
sources=json.loads((HERE/'source-candidates.json').read_text());m=sources['528']
assert workflow['on']['push']['branches']==[plan['review_branch']]
assert set(workflow['jobs'])=={'native'} and workflow['jobs']['native']['if'] is True
assert 'strategy' not in workflow['jobs']['native']
assert workflow['env']['ISSUE528_PLAN_SHA256']==sha(HERE/'plan.json') and workflow['env']['ISSUE528_ID']=='528'
assert list(sources)==['528'] and m['base']==plan['base'] and m['head']==plan['candidate_head'] and m['tree']==plan['candidate_tree']
normalized=json.loads(json.dumps(workflow));normalized['env']['ISSUE528_PLAN_SHA256']='<PLAN_SHA256>';normalized['jobs']['native']['if']='<ROOT_GATE>'
assert hashlib.sha256(json.dumps(normalized,sort_keys=True,separators=(',',':')).encode()).hexdigest()==plan['workflow_normalized_sha256']
counts=[]
for job in workflow['jobs'].values():
 for step in job['steps']:
  if step.get('uses','').startswith('actions/cache@'):
   keys=[step['with']['key']]+[x for x in step['with'].get('restore-keys','').splitlines() if x.strip()]
   assert len(keys)<=10 and len(keys)==len(set(keys));counts.append(len(keys))
assert counts==[], 'Native-only recovery must not add an Android/cache lane'
for relative in plan['recipe_inputs']:
 path=HERE/relative
 if path.suffix=='.py':ast.parse(path.read_text(),filename=str(path))
retained=json.loads((HERE/'retained-android.json').read_text())
assert retained['require_actual_original_job_success_and_raw_proof_before_ready'] and not retained['previous_pass_relabelled_as_new_execution']
assert retained['original_source_commit']==plan['retained_android_source_commit']
assert m['parents']==[retained['original_source_commit']], 'Recovery must be the reviewed test-only child'
def git(*args):return subprocess.check_output(['git','-C',str(root),*args],text=True).strip()
old=retained['original_source_commit'];new=m['head']
assert git('rev-parse',old+'^{tree}')==retained['original_source_tree']
assert git('rev-parse',new+'^{tree}')==m['tree']
changed=git('diff','--name-only',old,new).splitlines()
assert changed==retained['required_source_delta']==['tests/cateps.c']
subprocess.run(['git','-C',str(root),'diff','--quiet',old,new,'--','.',':(exclude)tests/cateps.c'],check=True)
proof={'verified':True,'review_commit':os.environ['GITHUB_SHA'],'workflow_run_id':int(os.environ['GITHUB_RUN_ID']),'workflow_run_attempt':int(os.environ['GITHUB_RUN_ATTEMPT']),'plan_sha256':sha(HERE/'plan.json'),'source_manifest_sha256':plan['source_manifest_sha256'],'retained_origin_sha256':sha(HERE/'retained-android.json'),'old_source_commit':old,'old_source_tree':retained['original_source_tree'],'new_source_commit':new,'new_source_tree':m['tree'],'exact_changed_paths':changed,'every_product_build_input_and_other_path_unchanged':True,'old_android_not_reexecuted':True,'original_android_success_claimed_here':False,'required_original_android_run_id':retained['original_run_id'],'required_original_android_job_id':retained['original_job_id'],'scope':'Git proves the only change is the native test fixture; root must separately verify the original Android job and raw proof before readiness.'}
out=root/'results';out.mkdir(exist_ok=True);(out/'retained-android-equivalence.json').write_text(json.dumps(proof,indent=2)+'\n')
print(json.dumps({'closed_recipe':True,'candidate':new,'sdk_producers':1,'cache_keys':counts,'retained_android_source_equivalence':True,'android_rebuilt':False}))
