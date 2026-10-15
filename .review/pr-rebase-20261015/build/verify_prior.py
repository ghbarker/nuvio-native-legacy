#!/usr/bin/env python3
"""Reuse23 proofs only from successful source-bound jobs; rerun the other five."""
import hashlib
import json
from pathlib import Path
import shutil
import sys
from verify_source import plan
old=Path(sys.argv[1]).resolve();out=Path(sys.argv[2]).resolve()
value,_=plan('pr431')
prior=json.loads((old/'reuse-receipt.json').read_text())
assert prior['outcome']=='passed' and prior['source_candidates_changed'] is False
previous={(r['case'],r['gate']):r for r in prior['reused']}
assert len(previous)==21
jobs=json.loads(Path('build/review/prior-jobs.json').read_text())['jobs']
by_name={j['name']:j for j in jobs}
new=[]
for group in ('profile-state','providers'):
 job=by_name['Repair reduced fixtures: '+group]
 assert job['status']=='completed' and job['conclusion']=='success',(group,job['conclusion'])
 new.append({'case':'pr493','gate':'originals-'+group,'origin_run':38668819623,
   'origin_review_commit':'2501484817c26215e5128110a3bd90529986cac0','job_id':job['id'],'job_name':job['name']})
reused=[]
for item in [*previous.values(),*new]:
 case_id,gate=item['case'],item['gate'];path=old/case_id/gate/'verified.json'
 data=json.loads(path.read_text());_,case=plan(case_id)
 assert data['outcome']=='passed' and data['source_verified'] is True
 assert data['source_commit']==case['head'] and data['source_tree']==case['tree']
 assert data['original_test_sha256']==case['original_tests'] and data['review_commit']==item['origin_review_commit']
 digest=hashlib.sha256(path.read_bytes()).hexdigest()
 if (case_id,gate) in previous:assert digest==item['proof_sha256']
 target=out/case_id/gate/'verified.json';target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(path,target)
 reused.append({**item,'proof_sha256':digest})
assert len(reused)==23
replaced=[{'case':'pr493','gate':g} for g in ('trackers','trackers-existing','auth-owned','auth-existing','http')]
assert not {(r['case'],r['gate']) for r in replaced}&{(r['case'],r['gate']) for r in reused}
(out/'reuse-receipt.json').write_text(json.dumps({'outcome':'passed','source_candidates_changed':False,
 'reused':reused,'replaced':replaced,'basis':'23 exact-source required groups from successful jobs; full tracker and HTTP batch retested'},indent=2)+'\n')
print('Verified reusable gates from successful jobs:',len(reused))
