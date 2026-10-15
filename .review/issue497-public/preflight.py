"""Reject stale pins, malformed recipes and cache-key overflow before toolchain work."""
import ast,json,os,subprocess
from pathlib import Path
from common import HERE,approved,sha
p=approved()
workflow=json.loads((Path(os.environ['GITHUB_WORKSPACE'])/'.github/workflows/issue497-public-review.yml').read_text())
assert workflow['on']['push']['branches']==[p['review_branch']]
assert set(workflow['jobs'])==set(p['required_jobs'])|{'all_gates'}
assert workflow['jobs']['all_gates']['needs']==p['required_jobs']
assert workflow['env']['STARTUP497_PLAN_SHA256']==sha(HERE/'plan.json')
caches=[]
for job_id,job in workflow['jobs'].items():
 for step in job.get('steps',[]):
  if step.get('uses','').startswith('actions/cache@'):
   keys=[step['with']['key']]+[line for line in step['with'].get('restore-keys','').splitlines()if line]
   assert len(keys)<=10 and len(keys)==len(set(keys)),(job_id,keys)
   caches.append({'job':job_id,'keys':len(keys)})
for relative in p['recipe_inputs']:
 path=HERE/relative
 if path.suffix=='.py':ast.parse(path.read_text(),filename=str(path))
 if path.suffix=='.sh':subprocess.run(['/bin/bash','-n',str(path)],check=True)
assert len(caches)==2 and all(c['job']=='android'for c in caches)
assert p['source_commit']=='e54672fdfe0a2df5843c7fee685e1743e15d16fd'
assert p['source_tree']=='6e017811272138b061f38e98ddad72d132ea1275'
print(json.dumps({'closed_kit':True,'plan_sha256':sha(HERE/'plan.json'),'caches':caches,'source':p['source_commit']}))
