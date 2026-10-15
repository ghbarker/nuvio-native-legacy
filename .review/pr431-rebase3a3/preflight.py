"""Reject drift in the isolated two-producer PR431 recipe before SDK work."""
from pathlib import Path
import ast,hashlib,json,os
from common import HERE,approved,sha
p=approved();w=json.loads((Path(os.environ['GITHUB_WORKSPACE'])/'.github/workflows/pr431-rebase3a3-review.yml').read_text());m=json.loads((HERE/'source-candidates.json').read_text())
assert list(m)==['431'] and m['431']['base']==p['base'] and m['431']['head']==p['candidate_head'] and m['431']['tree']==p['candidate_tree']
assert w['on']['push']['branches']==[p['review_branch']] and set(w['jobs'])=={'native','android'}
assert w['env']['REBASE_PR']=='431' and w['env']['REBASE_PLAN_SHA256']==sha(HERE/'plan.json')
n=json.loads(json.dumps(w));n['env']['REBASE_PLAN_SHA256']='<PLAN_SHA256>'
for name,j in w['jobs'].items():
 assert j['if'] is True and 'strategy'not in j and j['env']['REBASE_PR']=='431';n['jobs'][name]['if']='<ROOT_GATE>'
 assert len([s for s in j['steps']if s.get('uses','').startswith('actions/upload-artifact@')])==2
 assert any('-proof-'in s.get('with',{}).get('name','') and '!results/**/*.apk'in s['with']['path']for s in j['steps'])
assert hashlib.sha256(json.dumps(n,sort_keys=True,separators=(',',':')).encode()).hexdigest()==p['workflow_normalized_sha256']
counts=[]
for j in w['jobs'].values():
 for s in j['steps']:
  if s.get('uses','').startswith('actions/cache@'):
   keys=[s['with']['key']]+[x for x in s['with'].get('restore-keys','').splitlines()if x.strip()]
   assert len(keys)<=10 and len(keys)==len(set(keys));counts.append(len(keys))
assert counts==[10,10]
for relative in p['recipe_inputs']:
 if (HERE/relative).suffix=='.py':ast.parse((HERE/relative).read_text(),filename=relative)
assert p['fresh_runtime']=={'compiled_fixtures':22,'executed_cases':70,'settings_cases':48,'storage_error_geometry_cases':12,'tv_notice_observations':12,'normal_and_sanitizer_modes':['normal','address+undefined']}
print(json.dumps({'closed_recipe':True,'source':p['candidate_head'],'base':p['base'],'sdk_producers':2,'cache_keys':counts,'runtime_host':'Linux Android lane','previous_passes_reused':False}))
