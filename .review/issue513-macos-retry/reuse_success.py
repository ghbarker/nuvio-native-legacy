#!/usr/bin/env python3
"""Reuse only terminal-success jobs; retain original review and source identity."""
import hashlib,json,os,subprocess,zipfile
from pathlib import Path
OLD='632445ec8f99b16dac209e1be028ba377245e60f';RUN=38672075385;REPO=os.environ['GH_REPO'];out=Path('delivery');out.mkdir(exist_ok=True)
SOURCES={'standalone-issue513':'ac81ae7ef86668fe975efdbabb26aabfd8c43d83','combined-delivery':'02777c0802d7d5ce1b6dd405f871db6c3c6b46b8'}
def api(path):return json.loads(subprocess.check_output(['gh','api','repos/'+REPO+'/'+path],text=True))
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
jobs=[]
for job_id in (116217288244,116217414465,116217414539,116217414530):
 job=api('actions/jobs/'+str(job_id));assert job['run_id']==RUN and job['head_sha']==OLD and job['status']=='completed' and job['conclusion']=='success'
 jobs.append({'job_id':job_id,'name':job['name'],'status':'completed','conclusion':'success'})
artifacts=[(11943933144,'d37990d88108a937982dd2c926e12fd2d61f34c17c915340359bcd25abfac7bf','materialization'),(11945250044,'127eca8db70b65b99c2f666797208a3fe9d8815fda28a9f4665f8ddf844ca014','native-standalone-issue513'),(11945080869,'c5ba151cafb892fdb0ec4f992785050c603488d3b4ed17437700770238432b42','android'),(11944795975,'ffc69fe7d8956ae282573feed448ec2d6a1525be356f03ba1bc89a25fc0be79c','apk'),(11944459449,'872755f03078e80688e43f66dba4dd38fefd49a6cc5243b90ef3b04d4f7cc59d','runtime')]
records=[]
for artifact_id,digest,folder in artifacts:
 meta=api('actions/artifacts/'+str(artifact_id));assert not meta['expired'] and meta['workflow_run']['head_sha']==OLD and meta['digest']=='sha256:'+digest
 archive=out/(folder+'.zip')
 with archive.open('wb') as f:subprocess.run(['gh','api','repos/'+REPO+'/actions/artifacts/'+str(artifact_id)+'/zip'],stdout=f,check=True)
 assert sha(archive)==digest
 with zipfile.ZipFile(archive) as z:
  assert all(not Path(n).is_absolute() and '..' not in Path(n).parts for n in z.namelist());z.extractall(out/folder)
 records.append({'artifact_id':artifact_id,'sha256':digest,'folder':folder,'original_review_commit':OLD})
 archive.unlink()
prepared=json.loads((out/'materialization/prepared-cases.json').read_text());plan=json.loads(Path('.review/issue513-delivery/plans.json').read_text())
for case in prepared['matrix']['include']:
 assert case['source_commit']==SOURCES[case['id']]
 def git(*args):return subprocess.check_output(['git',*args],text=True).strip()
 assert git('rev-list','--parents','-n','1',case['source_commit']).split()==[case['source_commit'],case['base']]
 manifest=json.loads((Path('.review/issue513-delivery')/case['manifest']).read_text())
 assert git('diff','--name-only',case['base'],case['source_commit']).splitlines()==['src/detail.c','src/streams.c']
 assert git('rev-parse',case['base']+':tests')==git('rev-parse',case['source_commit']+':tests')
 for e in manifest['files']:assert git('rev-parse',case['source_commit']+':'+e['path'])==e['sha']
clean=json.loads((out/'native-standalone-issue513/native/native-gates-verified.json').read_text());assert clean['outcome']=='passed' and clean['source_commit']==SOURCES['standalone-issue513'] and clean['review_commit']==OLD and clean['assertions_passed']==272 and clean['source_unchanged']
for ui,entry in clean['full_source_compiles'].items():assert sha(out/'native-standalone-issue513/native'/('app-'+ui))==entry['sha256']
android=json.loads((out/'android/update-identity.json').read_text());assert android['verified'] and android['source_commit']==SOURCES['combined-delivery'] and android['build_commit']==OLD
apk=list((out/'apk').glob('*-tv-mobile-test.apk'));assert len(apk)==1 and sha(apk[0])==android['apk_sha256']=='aba2b112aa50cb648d25d9b77510d2247aa26280a2c62230b681286870fa19cc' and apk[0].stat().st_size==android['apk_bytes']==58819304
runtime=json.loads((out/'runtime/player-component-reuse.json').read_text());assert runtime['outcome']=='passed' and runtime['new_product_source_commit']==SOURCES['combined-delivery'] and runtime['new_review_commit']==OLD and runtime['required_runtime_contracts']==runtime['xml_tests_passed']==20
# All runtime fixture inputs remain exactly the successful review's bytes.
manifest_path='.review/subtitle-logs6/android/fixture-manifest.json';b=subprocess.check_output(['git','show',OLD+':'+manifest_path]);assert Path(manifest_path).read_bytes()==b
for e in json.loads(b)['helper_files']:
 path='.review/subtitle-logs6/android/'+e['path'];assert Path(path).read_bytes()==subprocess.check_output(['git','show',OLD+':'+path])
receipt={'outcome':'passed','review_commit':os.environ['GITHUB_SHA'],'reused_original_review_commit':OLD,'original_run_id':RUN,'successful_jobs':jobs,'artifacts':records,'source_commits':SOURCES,'failed_combined_native_job_reused':False,'complete_combined_native_rerun_required':True,'apk_sha256':android['apk_sha256']}
(out/'successful-validation-origin.json').write_text(json.dumps(receipt,indent=2)+'\n')
print('Verified only successful original clean/native/runtime/Android jobs; failed combined native result not reused')
