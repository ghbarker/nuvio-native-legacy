"""Fail closed on matrix/source/cache/recipe drift before any SDK build."""
from pathlib import Path
import ast,json,os,subprocess
from common import HERE,approved,sha
p=approved();root=Path(os.environ['GITHUB_WORKSPACE']);w=json.loads((root/'.github/workflows/rebases10e3-review.yml').read_text());candidates=json.loads((HERE/'source-candidates.json').read_text())
assert w['on']['push']['branches']==[p['review_branch']]
assert set(w['jobs'])=={'native','android','all_gates'}and w['jobs']['all_gates']['needs']==['native','android']
for component in ['native','android']:
 j=w['jobs'][component];assert j['strategy']['fail-fast']is False
 assert j['strategy']['matrix']['pr']==p['candidates']==sorted(candidates)
 assert j['env']['REBASE_PR']=='${{ matrix.pr }}'
assert w['env']['REBASE_PLAN_SHA256']==sha(HERE/'plan.json')
assert w['env']['ASAN_OPTIONS']=='detect_leaks=0:halt_on_error=1'
cache_counts=[]
for j in w['jobs'].values():
 for step in j.get('steps',[]):
  if step.get('uses')=='actions/cache@v4':
   keys=[step['with']['key']]+[x for x in step['with'].get('restore-keys','').splitlines()if x]
   assert len(keys)<=10 and len(keys)==len(set(keys));cache_counts.append(len(keys))
assert cache_counts==[10,10]
for relative in p['recipe_inputs']:
 path=HERE/relative
 if path.suffix=='.py':ast.parse(path.read_text(),filename=str(path))
 if path.suffix=='.sh':subprocess.run(['/bin/bash','-n',str(path)],check=True)
for pr,m in candidates.items():
 assert str(m['pr'])==pr and m['base']==p['base']and len(m['head'])==40 and len(m['tree'])==40
 assert m['exact_diff_paths']==sorted(e['path']for e in m['paths']+m['reviewed_test_paths'])
 assert (HERE/'fixtures'/('pr'+pr)/'entry.py').is_file()
print(json.dumps({'closed_recipe':True,'source_manifest_sha256':p['source_manifest_sha256'],'matrix_candidates':p['candidates'],'cache_keys':cache_counts,'sdk_producers':10}))
