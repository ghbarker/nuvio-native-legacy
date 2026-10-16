"""Fail closed before exact failover-attempt public-source SDK producers."""
from pathlib import Path
import ast,hashlib,json,os,subprocess
from common import HERE,approved,sha
p=approved();sources=json.loads((HERE/'source-candidates.json').read_text());w=json.loads((Path(os.environ['GITHUB_WORKSPACE'])/'.github/workflows/failover-attempt-review.yml').read_text())
assert list(sources)==p['selected_candidates']==['0']
assert p['internal_issue_candidate']==0 and p['public_pr_number'] is None
assert p['required_jobs']==['native'] and set(w['jobs'])=={'native'}
assert w['on']['push']['branches']==[p['review_branch']] and w['env']['REBASE_PLAN_SHA256']==sha(HERE/'plan.json')
n=json.loads(json.dumps(w));n['env']['REBASE_PLAN_SHA256']='<PLAN_SHA256>';counts=[]
for lane,job in w['jobs'].items():
 assert job['if'] is True and 'strategy' not in job and job['env']['REBASE_PR']=='0'
 assert job['name'].startswith('Failover ');n['jobs'][lane]['if']='<ROOT_GATE>'
 uploads=[s for s in job['steps']if s.get('uses','').startswith('actions/upload-artifact@')]
 assert len(uploads)==2 and any('-proof-'in s['with']['name']and '!results/**/*.apk'in s['with']['path']for s in uploads)
 for step in job['steps']:
  if step.get('uses','').startswith(('actions/cache@','actions/cache/restore@')):
   keys=[step['with']['key']]+[q.strip()for q in step['with'].get('restore-keys','').splitlines()if q.strip()]
   assert len(keys)<=10 and len(keys)==len(set(keys));counts.append(len(keys))
assert counts==[]
assert hashlib.sha256(json.dumps(n,sort_keys=True,separators=(',',':')).encode()).hexdigest()==p['workflow_normalized_sha256']
m=sources['0'];assert m['head']==m['public_test_commit'] and m['parents']==[m['product_head']] and m['base']==p['current_master'] and m['fresh_sdk_required']
paths=subprocess.check_output(['git','diff','--name-only',m['product_head'],m['head']],text=True).splitlines();assert paths==m['public_test_commit_paths'] and paths and all(q.startswith('tests/')for q in paths)
assert m['public_suites'] and m['android_public_suites']==[]
for q in m['public_suites']:
 assert q['script'].startswith('tests/') and q['script'].endswith('.sh')and q['modes']==['normal','asan']and q['inputs']
 for variant in q.get('variants',[{}]):
  rules={**q,**variant};assert rules.get('required_markers')or rules.get('marker_counts')
 for path in [q['script'],*q['inputs']]:assert path.startswith(('src/','tests/'))
for relative in p['recipe_inputs']:
 if relative.endswith('.py'):ast.parse((HERE/relative).read_text(),filename=relative)
print(json.dumps({'source_bound':True,'internal_issue_candidate':0,'public_pr_number':None,'sdk_producers':1,'cache_keys':counts,'public_tests_are_separate_commit':True,'prior_sdk_passes_reused':False,'private_overlay_reused':False}))

assert m['candidate_label']=='failover-attempt-ownership'
assert [q['script']for q in m['public_suites']]==['tests/fontefalha_prebusca.sh','tests/fontefalha_playback.sh','tests/fontesmortas.sh']
assert sum(len(q['modes'])*len(q.get('variants',[{}]))for q in m['public_suites'])==6

for job in w['jobs'].values():
 for step in job['steps']:
  if 'materialize.py'in step.get('run',''):assert '--candidate "0"'in step['run']

b=p['retained_android_input_identity']
def non_test_rows(commit):
 return [r for r in subprocess.check_output(['git','ls-tree','-r','--full-tree',commit],text=True).splitlines()if not r.split('\t',1)[1].startswith('tests/')]
old_rows=non_test_rows(b['retained_android_source']);new_rows=non_test_rows(m['head'])
assert old_rows==new_rows and len(new_rows)==b['non_test_paths']
assert hashlib.sha256(('\n'.join(new_rows)+'\n').encode()).hexdigest()==b['non_test_tree_listing_sha256']
assert subprocess.check_output(['git','diff','--name-only',b['retained_android_source'],m['head']],text=True).splitlines()==['tests/fontesmortas.c']
