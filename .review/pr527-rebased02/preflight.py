"""Reject drift before either isolated PR527 SDK producer starts."""
from pathlib import Path
import ast,hashlib,json,os
from common import HERE,approved,sha
p=approved();w=json.loads((Path(os.environ['GITHUB_WORKSPACE'])/'.github/workflows/pr527-rebased02-review.yml').read_text());m=json.loads((HERE/'source-candidates.json').read_text())
assert list(m)==['527'] and m['527']['base']==p['base'] and m['527']['head']==p['candidate_head'] and m['527']['tree']==p['candidate_tree']
assert w['on']['push']['branches']==[p['review_branch']] and set(w['jobs'])=={'native','android'} and p['required_jobs']==['native','android']
assert w['env']['REBASE_PR']=='527' and w['env']['REBASE_PLAN_SHA256']==sha(HERE/'plan.json')
n=json.loads(json.dumps(w));n['env']['REBASE_PLAN_SHA256']='<PLAN_SHA256>'
for name,j in w['jobs'].items():
 assert j['if'] is True and 'strategy'not in j and j['env']['REBASE_PR']=='527';n['jobs'][name]['if']='<ROOT_GATE>'
 uploads=[s for s in j['steps']if s.get('uses','').startswith('actions/upload-artifact@')]
 assert len(uploads)==2
 assert any('-proof-'in s['with']['name'] and '!results/**/*.apk'in s['with']['path']for s in uploads)
assert hashlib.sha256(json.dumps(n,sort_keys=True,separators=(',',':')).encode()).hexdigest()==p['workflow_normalized_sha256']
a=w['jobs']['android']['steps'];before=next(s for s in a if s.get('name')=='Capture strictly clean Android workspace before build');after=next(s for s in a if s.get('name')=='Preserve and verify post-build Android workspace')
assert before['id']=='workspace_before' and after['if']=="${{ always() && steps.workspace_before.conclusion == 'success' }}"
assert not any('kotlin.py'in s.get('run','')for s in a),'No unrelated PR431 Kotlin suite'
counts=[]
for j in w['jobs'].values():
 for s in j['steps']:
  if s.get('uses','').startswith('actions/cache@'):
   keys=[s['with']['key']]+[x for x in s['with'].get('restore-keys','').splitlines()if x.strip()]
   assert len(keys)<=10 and len(keys)==len(set(keys));counts.append(len(keys))
assert counts==[10,10]
for relative in p['recipe_inputs']:
 if (HERE/relative).suffix=='.py':ast.parse((HERE/relative).read_text(),filename=relative)
assert p['fresh_focused_groups']==['settings-normal','settings-asan','collection-state-normal','collection-state-asan']
print(json.dumps({'closed_recipe':True,'source':p['candidate_head'],'base':p['base'],'sdk_producers':2,'cache_keys':counts,'focused_host':'Linux Android lane','previous_passes_reused':False}))
