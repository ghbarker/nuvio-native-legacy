#!/usr/bin/env python3
"""Reuse runtime assertions only for byte-identical player/fixture components.

The new app dispatcher is tested separately; this does not relabel old runtime
tests as an execution of the new complete application source.
"""
import argparse,hashlib,json,os,subprocess,xml.etree.ElementTree as ET,zipfile
from pathlib import Path

p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);p.add_argument('--source-proof',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
source=a.source.resolve();out=a.output.resolve();out.mkdir(parents=True)
OLD_REVIEW='1eac5860820e6e8b142140ceccd054ea02956793';OLD_SOURCE='04612e42fc58776d947062e908d32ce7322e58cf'
RUN=38668353286;JOB=116205360306;ARTIFACT=11942468468
REPO=os.environ['GH_REPO'];FIX=Path('.review/subtitle-logs6/android')
def gh(path):return json.loads(subprocess.check_output(['gh','api','repos/'+REPO+'/'+path],text=True))
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def blob(ref,path):return subprocess.check_output(['git','show',ref+':'+path])
job=gh('actions/jobs/'+str(JOB));assert job['run_id']==RUN and job['head_sha']==OLD_REVIEW and job['status']=='completed' and job['conclusion']=='success'
artifact=gh('actions/artifacts/'+str(ARTIFACT));assert not artifact['expired'] and artifact['workflow_run']['head_sha']==OLD_REVIEW
archive=out/'original-runtime.zip'
with archive.open('wb') as f:subprocess.run(['gh','api','repos/'+REPO+'/actions/artifacts/'+str(ARTIFACT)+'/zip'],stdout=f,check=True)
assert artifact['digest']=='sha256:'+sha(archive)=='sha256:59e52d8bf58130365887b739cca3c74e8b33c2ed4bb291a434234d110e8f7a47'
with zipfile.ZipFile(archive) as z:
 assert all(not Path(n).is_absolute() and '..' not in Path(n).parts for n in z.namelist());z.extractall(out/'original')
proofs=list((out/'original').rglob('media3-verified.json'));assert len(proofs)==1
old=json.loads(proofs[0].read_text());assert old['outcome']=='passed' and old['source_commit']==OLD_SOURCE and old['review_commit']==OLD_REVIEW
import sys
sys.path.insert(0,'.review/subtitle-logs6/build')
from media3_contracts import load_contracts
manifest,required,startup,classes,manifest_sha=load_contracts(FIX/'fixture-manifest.json')
assert len(required)==20 and set(old['required_tests'])==required and old['fixture_manifest_sha256']==manifest_sha
assert old['required_test_classes']==classes and required<=set(old['tests_seen'])
seen={};count=0
for path in (out/'original').rglob('TEST-*.xml'):
 for test in ET.parse(path).getroot().iter('testcase'):
  assert all(test.find(t) is None for t in ('failure','error','skipped'))
  seen.setdefault(test.attrib['classname'],set()).add(test.attrib['name'].split('[')[0]);count+=1
assert count==20
for klass,names in classes.items():assert set(names)<=seen.get(klass,set())
unchanged=[]
for e in manifest['staged_player_helpers']:
 path='android/app/src/main/java/'+e['path'];data=(source/path).read_bytes()
 assert data==blob(OLD_SOURCE,path) and hashlib.sha256(data).hexdigest()==e['sha256'],path
 unchanged.append({'path':path,'sha256':e['sha256']})
fixture_hashes=[]
for e in manifest['helper_files']:
 path=str(FIX/e['path']);data=Path(path).read_bytes()
 assert data==blob(OLD_REVIEW,path) and hashlib.sha256(data).hexdigest()==e['sha256'],path
 fixture_hashes.append({'path':path,'sha256':e['sha256']})
assert (FIX/'fixture-manifest.json').read_bytes()==blob(OLD_REVIEW,str(FIX/'fixture-manifest.json'))
current=json.loads(a.source_proof.read_text());assert current['verified'] and current['case']=='combined-delivery'
receipt={'outcome':'passed','scope':'unchanged isolated Media3 player and local-download components; not the new complete application dispatcher',
 'new_product_source_commit':current['source_commit'],'new_review_commit':os.environ['GITHUB_SHA'],
 'validation_origin':{'run_id':RUN,'job_id':JOB,'artifact_id':ARTIFACT,'review_commit':OLD_REVIEW,'source_commit':OLD_SOURCE,
                      'artifact_sha256':sha(archive),'proof_sha256':sha(proofs[0])},
 'unchanged_product_components':unchanged,'unchanged_fixture_graph':fixture_hashes,'fixture_manifest_sha256':manifest_sha,
 'required_runtime_contracts':20,'xml_tests_passed':20,'skipped':0,'failures':0,'old_validation_proof_unmodified':True,
 'new_dispatcher_validation_required_separately':True}
(out/'player-component-reuse.json').write_text(json.dumps(receipt,indent=2)+'\n')
(out/'media3-verified.json').write_bytes(proofs[0].read_bytes())
print('Verified honest runtime reuse:20passing contracts, exact unchanged player and helper bytes; new dispatcher tested separately')
