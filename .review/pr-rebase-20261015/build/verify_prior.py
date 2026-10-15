#!/usr/bin/env python3
"""Reuse27 exact-source proofs only from successful jobs; retest provider group."""
import hashlib,json,shutil,sys
from pathlib import Path
from verify_source import plan
old=Path(sys.argv[1]).resolve();out=Path(sys.argv[2]).resolve();value,_=plan('pr431')
previous=json.loads((old/'reuse-receipt.json').read_text());assert previous['outcome']=='passed' and len(previous['reused'])==21
jobs=json.loads(Path('build/review/prior-jobs.json').read_text())
by1={j['name']:j for j in jobs['first_retry']['jobs']};by2={j['name']:j for j in jobs['second_retry']['jobs']}
for job in (by1['Verify and reuse successful original exact-source gates'],by1['Repair reduced fixtures: profile-state'],by2['Repair reduced fixtures: profile-trackers-http']):
 assert job['status']=='completed' and job['conclusion']=='success',(job['name'],job['conclusion'])
items=list(previous['reused']);j=by1['Repair reduced fixtures: profile-state']
items.append({'case':'pr493','gate':'originals-profile-state','origin_run':38668819623,'origin_review_commit':'2501484817c26215e5128110a3bd90529986cac0','job_id':j['id'],'job_name':j['name']})
j=by2['Repair reduced fixtures: profile-trackers-http']
for group in ('trackers','trackers-existing','auth-owned','auth-existing','http'):
 items.append({'case':'pr493','gate':group,'origin_run':38669355249,'origin_review_commit':'845ea6137755e40a486b6bc4d77dc173a4ae4567','job_id':j['id'],'job_name':j['name']})
reused=[]
for item in items:
 case_id,gate=item['case'],item['gate'];path=old/case_id/gate/'verified.json';data=json.loads(path.read_text());_,case=plan(case_id)
 assert data['outcome']=='passed' and data['source_verified'] is True
 assert data['source_commit']==case['head'] and data['source_tree']==case['tree'] and data['original_test_sha256']==case['original_tests']
 assert data['review_commit']==item['origin_review_commit']
 digest=hashlib.sha256(path.read_bytes()).hexdigest()
 if 'proof_sha256' in item:assert digest==item['proof_sha256']
 target=out/case_id/gate/'verified.json';target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(path,target)
 reused.append({**item,'proof_sha256':digest})
assert len(reused)==27 and len({(i['case'],i['gate']) for i in reused})==27
(out/'reuse-receipt.json').write_text(json.dumps({'outcome':'passed','source_candidates_changed':False,'reused':reused,
 'replaced':[{'case':'pr493','gate':'originals-providers'}],'basis':'27 exact-source required groups from successful jobs; complete provider batch rerun with real language code and documented #205 vectors'},indent=2)+'\n')
print('Verified successful-job proof groups reused:',len(reused))
