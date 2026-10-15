#!/usr/bin/env python3
"""Select only15historical player tests with a byte-identical dependency closure."""
from pathlib import Path
import argparse,hashlib,json,os,subprocess,zipfile,xml.etree.ElementTree as ET
p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);p.add_argument('--source-proof',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--repository',required=True);p.add_argument('--artifact',type=Path);p.add_argument('--offline-review',action='store_true');a=p.parse_args();here=Path(__file__).resolve().parent;plan=json.loads((here/'player-reuse-plan.json').read_text());origin=plan['original_success'];out=a.output.resolve();out.mkdir(parents=True,exist_ok=True);sha=lambda path:hashlib.sha256(Path(path).read_bytes()).hexdigest();proof=json.loads(a.source_proof.read_text());assert proof['verified'] and proof['layer']=='test'
for e in plan['current_unchanged_player_components']:assert sha(a.source/e['path'])==e['sha256'],e['path']
old_manifest=json.loads((here/'original-fixture-manifest.json').read_text());old_graph={e['path']:e['sha256'] for e in old_manifest['helper_files']}
for e in plan['unchanged_fixture_graph']:
 assert old_graph[e['path']]==e['sha256'],e['path']
 assert sha(here/'fixture-graph'/e['path'])==e['sha256'],e['path']
classes=plan['required_test_classes'];assert sum(map(len,classes.values()))==15 and not any(k.endswith('LocalLogsContractTest') for k in classes)
if not a.offline_review:
 def gh(path):return json.loads(subprocess.check_output(['gh','api','repos/'+a.repository+'/'+path],text=True))
 job=gh('actions/jobs/'+str(origin['job_id']));assert job['head_sha']==origin['review_commit'] and job['run_id']==origin['run_id'] and job['conclusion']=='success' and job['status']=='completed'
 artifact=gh('actions/artifacts/'+str(origin['artifact_id']));assert not artifact['expired'] and artifact['workflow_run']['head_sha']==origin['review_commit'] and artifact['digest']=='sha256:'+origin['artifact_sha256']
archive=a.artifact or out/'original-runtime.zip'
if not a.artifact:
 with archive.open('wb') as f:subprocess.run(['gh','api','repos/'+a.repository+'/actions/artifacts/'+str(origin['artifact_id'])+'/zip'],stdout=f,check=True)
assert sha(archive)==origin['artifact_sha256'];seen={};original_proofs=[]
with zipfile.ZipFile(archive) as z:
 for name in z.namelist():
  if name.endswith('/media3-verified.json'):
   data=z.read(name);assert hashlib.sha256(data).hexdigest()==plan['original_media3_proof_sha256'];original_proofs.append(json.loads(data))
  if Path(name).name.startswith('TEST-') and name.endswith('.xml'):
   for t in ET.fromstring(z.read(name)).iter('testcase'):
    k=t.attrib.get('classname');n=t.attrib.get('name','').split('[')[0]
    if k in classes and n in classes[k]:assert not any(t.find(s) is not None for s in ['failure','error','skipped']);seen.setdefault(k,set()).add(n)
assert len(original_proofs)==1;old=original_proofs[0];assert old['source_commit']==origin['source_commit'] and old['review_commit']==origin['review_commit'] and old['outcome']=='passed'
assert old['fixture_manifest_sha256']==sha(here/'original-fixture-manifest.json')
old_components={e['path']:e['sha256'] for e in old['exact_staged_product_sources']}
for e in plan['current_unchanged_player_components']:
 relative=e['path'].removeprefix('android/app/src/main/java/');assert old_components[relative]==e['sha256']
for k,names in classes.items():assert set(names)<=seen.get(k,set()) and set(names)==set(old['required_test_classes'][k])
receipt={'outcome':'passed','source_commit':proof['source_commit'],'review_commit':proof['review_commit'],'scope':'Historical15isolated unchanged player contracts only; all5changed privateDownload contracts excluded; not a new whole-app timing run','original_success':origin,'current_unchanged_player_components':plan['current_unchanged_player_components'],'required_test_classes':classes,'passed_selected_tests':15,'excluded_old_private_log_tests':5,'online_original_job_verified':not a.offline_review,'source_manifest_sha256':proof['manifest_sha256']};(out/'player15-reuse-verified.json').write_text(json.dumps(receipt,indent=2)+'\n');print('Verified selected15unchanged historical player tests; changed5oldlog tests excluded')
