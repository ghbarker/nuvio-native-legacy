#!/usr/bin/env python3
"""Reuse only successful, immutable-source validation with honest provenance."""
import hashlib,json,os,shutil,subprocess,time,xml.etree.ElementTree as ET,zipfile
from pathlib import Path

HERE=Path(__file__).resolve().parent
OLD='03c7ac63bb73f2448099f20e11da62728193ec72'
RUN=38665838503
REPO=os.environ['GH_REPO']
OUT=Path('build/reused');OUT.mkdir(parents=True,exist_ok=True)
FIX=Path('.review/subtitle-logs6')
def gh(path):return json.loads(subprocess.check_output(['gh','api','repos/'+REPO+'/'+path],text=True))
def blob(ref,path):return subprocess.check_output(['git','show',ref+':'+path])
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
run=gh('actions/runs/'+str(RUN));assert run['head_sha']==OLD
plan=json.loads((FIX/'build/plans.json').read_text())
source_ids={'standalone-subtitles':'039b50d4f6d9eaea05504d906e77b2b3b6538a56','combined-delivery':'04612e42fc58776d947062e908d32ce7322e58cf'}
source_proofs={}
for case in plan['cases']:
 source=source_ids[case['id']]
 if subprocess.run(['git','cat-file','-e',source],capture_output=True).returncode:
  subprocess.run(['git','fetch','origin',source],check=True)
 assert subprocess.check_output(['git','rev-list','--parents','-n','1',source],text=True).split()==[source,case['base']]
 manifest=json.loads((FIX/'build'/case['manifest']).read_text())
 changed=subprocess.check_output(['git','diff','--name-only','-z',case['base'],source]).decode().strip('\0').split('\0')
 assert sorted(changed)==sorted(e['path'] for e in manifest['files'])
 for e in manifest['files']:
  data=blob(source,e['path']);assert hashlib.sha1(b'blob '+str(len(data)).encode()+b'\0'+data).hexdigest()==e['sha']
 source_proofs[case['id']]={'source_commit':source,'parent':case['base'],
  'tree':subprocess.check_output(['git','rev-parse',source+'^{tree}'],text=True).strip(),
  'verified_changed_paths':len(manifest['files'])}
jobs=[('standalone-subtitles',116197627368,'Subtitle names beneath language (macOS)','subtitle-logs6-native-standalone-subtitles-',source_ids['standalone-subtitles']),
 ('combined-delivery',116197627432,'TV/Mobile test with original logs and download (macOS)','subtitle-logs6-native-combined-delivery-',source_ids['combined-delivery'])]
origins=[]
deadline=time.monotonic()+15*60
required_base={'subtitle-title-fixtures','linguas-normal','linguas-asan'}|{n+s for n in ('legendasui','legendasui_425') for s in ('-original','-original-asan')}|{n+s for n in ('legmemoria','faixasmkv','faixasmkv_casar') for s in ('-original','-sanitized-replay')}
for case,job_id,name,prefix,source in jobs:
 job=gh('actions/jobs/'+str(job_id))
 while job['status'] in ('queued','in_progress','waiting','pending'):
  assert time.monotonic()<deadline,('Timed out waiting for original validation',case,job_id)
  print('Waiting for original validation to finish:',case,job_id,flush=True)
  time.sleep(15)
  job=gh('actions/jobs/'+str(job_id))
 assert job['run_id']==RUN and job['head_sha']==OLD and job['name']==name
 assert job['status']=='completed' and job['conclusion']=='success',(case,job['status'],job['conclusion'])
 assert all(s['status']=='completed' and s['conclusion'] in ('success','skipped') for s in job['steps'])
 # Refresh only after this exact job is terminal; earlier artifact listings
 # cannot yet contain a still-running job's final attestation.
 artifacts=gh('actions/runs/'+str(RUN)+'/artifacts')['artifacts']
 found=[a for a in artifacts if a['name']==prefix+OLD]
 assert len(found)==1 and not found[0]['expired'];artifact=found[0]
 assert artifact['workflow_run']['id']==RUN and artifact['workflow_run']['head_sha']==OLD
 target=OUT/case;target.mkdir()
 archive=OUT/(case+'.zip')
 with archive.open('wb') as f:subprocess.run(['gh','api','repos/'+REPO+'/actions/artifacts/'+str(artifact['id'])+'/zip'],stdout=f,check=True)
 assert artifact['digest']=='sha256:'+sha(archive)
 with zipfile.ZipFile(archive) as z:
  assert all(not Path(n).is_absolute() and '..' not in Path(n).parts for n in z.namelist())
  z.extractall(target)
 proof_name='media3-verified.json' if case=='media3' else 'native-gates-verified.json'
 matches=list(target.rglob(proof_name));assert len(matches)==1
 proof=json.loads(matches[0].read_text())
 assert proof['outcome']=='passed' and proof['source_commit']==source and proof['review_commit']==OLD
 assert proof['case']==('combined-delivery' if case=='media3' else case)
 if case=='media3':
  import sys
  sys.path.insert(0,str(FIX/'build'))
  from media3_contracts import load_contracts
  manifest,required,startup,classes,manifest_sha=load_contracts(FIX/'android/fixture-manifest.json')
  assert proof['fixture_manifest_sha256']==manifest_sha
  assert set(proof['required_tests'])==required and required<=set(proof['tests_seen'])
  assert proof['required_test_classes']==classes and set(proof['startup_tests'])==startup
  seen={}
  reports=list(target.rglob('TEST-*.xml'));assert reports
  for report in reports:
   for test in ET.parse(report).getroot().iter('testcase'):
    assert all(test.find(t) is None for t in ('failure','error','skipped'))
    seen.setdefault(test.attrib['classname'],set()).add(test.attrib['name'].split('[')[0])
  for klass,tests in classes.items():assert set(tests)<=seen.get(klass,set()),klass
  for e in manifest['helper_files']:
   path=str(FIX/'android'/e['path']);data=Path(path).read_bytes()
   assert data==blob(OLD,path) and hashlib.sha256(data).hexdigest()==e['sha256']
  for e in manifest['staged_player_helpers']:
   data=blob(source,'android/app/src/main/java/'+e['path'])
   assert hashlib.sha256(data).hexdigest()==e['sha256']
  assert {e['path']:e['sha256'] for e in proof['exact_staged_product_sources']}=={e['path']:e['sha256'] for e in manifest['staged_player_helpers']}
 else:
  expected=required_base|({'subtitle-visual-tv','subtitle-visual-touch'} if case=='standalone-subtitles' else {'original-log-source','original-log-and-download-guards','original-log-and-download-guards-asan'})
  assert set(proof['gates'])==expected
  assert len(proof['original_test_bytes'])==11
  for path,want in proof['original_test_bytes'].items():assert hashlib.sha256(blob(source,path)).hexdigest()==want
  assert len(proof['sanitized_original_replays'])==3 and all(r['exit_code']==0 and r['original_source_arguments_and_assertions_unchanged'] for r in proof['sanitized_original_replays'])
  assert {(r['case'],r['mode']) for r in proof['subtitle_fixture_records']}=={(c,m) for c in ('title_rows','android_names','upstream_425') for m in ('normal','asan')}
  assert all(r['passed'] and r['compile']['exit_code']==0 and r['runs'] and all(x['exit_code']==0 for x in r['runs']) for r in proof['subtitle_fixture_records'])
  for path,want in proof['subtitle_source_sha256'].items():assert hashlib.sha256(blob(source,path)).hexdigest()==want
  for mode in ('tv','mobile'):
   binary=target/'native'/('app-'+mode);assert binary.is_file()
   records=[json.loads(p.read_text()) for p in (target/'compile').glob('*-invocation.json')]
   builds=[r for r in records if Path(r.get('linked_binary','')).name=='app-'+mode]
   assert len(builds)==1 and builds[0]['returncode']==0 and builds[0]['sha256']==sha(binary)
  if case=='standalone-subtitles':
   assert len(proof['subtitle_visual_captures'])==2
   for mode in ('tv','touch'):
    visual_root=target/'native'/('subtitle-visual-'+mode)
    visual=json.loads((visual_root/'visual-report.json').read_text())
    assert visual['passed'] and visual['source_unchanged'] and len(visual['captures'])==3 and visual['touch']==(mode=='touch')
    for filename,e in visual['captures'].items():assert sha(visual_root/filename)==e['sha256']
  else:assert proof['original_log_behavior']['original_reporting_preserved'] and proof['original_log_behavior']['original_live_viewer_preserved']
 origin={'case':case,'source_commit':source,'validation_run_id':RUN,'validation_job_id':job_id,'validation_review_commit':OLD,
  'artifact_id':artifact['id'],'artifact_sha256':sha(archive),'job_conclusion':'success','proof_sha256':sha(matches[0])}
 origins.append(origin)
 # Keep original proof bytes and provenance. Never rewrite their review SHA.
 verified=OUT/'verified'/case;verified.mkdir(parents=True)
 shutil.copy2(matches[0],verified/proof_name)
 if case=='standalone-subtitles':
  for mode in ('tv','touch'):shutil.copytree(target/'native'/('subtitle-visual-'+mode),verified/('subtitle-visual-'+mode),ignore=shutil.ignore_patterns('title-visual','compile.log','capture.log'))
 print('Verified reusable successful validation:',case,job_id,flush=True)
result={'outcome':'passed','validation_origin':{'run_id':RUN,'review_commit':OLD},'postbuild_validation_review_commit':os.environ['GITHUB_SHA'],
 'source_pins':source_proofs,'checks':origins,'source_unchanged':True,'original_validation_proofs_unmodified':True,
 'reason':'Only successful original macOS checks are reused; APKs and fresh runtime assertions are validated separately against the same product sources.'}
(OUT/'verified/validation-origin.json').write_text(json.dumps(result,indent=2)+'\n')
