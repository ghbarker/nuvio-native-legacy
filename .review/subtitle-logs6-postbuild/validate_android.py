#!/usr/bin/env python3
"""Validate exact built APKs; acknowledge and replace only the bad JNI check."""
import hashlib,json,os,re,subprocess,time,zipfile
from pathlib import Path
from dex_static import apk_static_methods

REPO=os.environ['GH_REPO'];BUILD='b1f00ebd3b7ca660a6160fe8fa2d53e8e6e80e12';RUN=38666973073
ROOT=Path('build/android-artifacts');ROOT.mkdir(parents=True)
deadline=time.monotonic()+20*60
def gh(path):return json.loads(subprocess.check_output(['gh','api','repos/'+REPO+'/'+path],text=True))
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def blob(ref,path):return subprocess.check_output(['git','show',ref+':'+path])
def download(name,target):
 items=gh('actions/runs/'+str(RUN)+'/artifacts')['artifacts'];items=[x for x in items if x['name']==name]
 assert len(items)==1 and not items[0]['expired'];a=items[0]
 assert a['workflow_run']['head_sha']==BUILD and a['workflow_run']['id']==RUN
 archive=ROOT/(str(a['id'])+'.zip')
 with archive.open('wb') as f:subprocess.run(['gh','api','repos/'+REPO+'/actions/artifacts/'+str(a['id'])+'/zip'],stdout=f,check=True)
 assert a['digest']=='sha256:'+sha(archive)
 target.mkdir(parents=True)
 with zipfile.ZipFile(archive) as z:
  assert all(not Path(n).is_absolute() and '..' not in Path(n).parts for n in z.namelist());z.extractall(target)
 return {'artifact_id':a['id'],'artifact_sha256':sha(archive),'artifact_name':name}
preload={'preparar','prepararProximo','prepararEstado','cancelarPreparo','abrirPreparada','pararMantendoPreparo'}
legacy=json.loads(Path('.review/subtitle-logs6/build/legacy-jni-descriptors.json').read_text())
cases=[('standalone-subtitles',116201039420,'039b50d4f6d9eaea05504d906e77b2b3b6538a56',False),('combined-delivery',116201039200,'04612e42fc58776d947062e908d32ce7322e58cf',True)]
reports=[]
for case,jid,source,touch in cases:
 job=gh('actions/jobs/'+str(jid))
 while job['status'] in ('queued','in_progress','waiting','pending'):
  assert time.monotonic()<deadline,('Build wait timed out',jid)
  print('Waiting for original Android build:',case,jid,flush=True);time.sleep(15);job=gh('actions/jobs/'+str(jid))
 assert job['head_sha']==BUILD and job['run_id']==RUN and job['status']=='completed'
 failed=[s['name'] for s in job['steps'] if s['conclusion']=='failure']
 expected_failure='Verify compiled production NvPlayer JNI lookup descriptors'
 assert (job['conclusion']=='success' and not failed) if touch else (job['conclusion']=='failure' and failed==[expected_failure])
 required=['Materialize and verify exact pinned product source','Public application configuration and debug signing','Standard production-tool debug APK','Verify standard package, both ARM cores, persistent signature and 16 KB native pages']
 if touch:required += [expected_failure,'Compatible TV and Mobile test APK','Verify compatible test package, label, persistent signature and native packaging','Verify real Kotlin/ARM local-log opt-in true','Verify real Kotlin/ARM local-log opt-in false']
 steps={s['name']:s for s in job['steps']}
 assert all(steps[n]['status']=='completed' and steps[n]['conclusion']=='success' for n in required)
 scope=ROOT/case
 kind='test' if touch else 'standard'
 asset=download('subtitle-logs6-'+kind+'-'+case+'-'+BUILD,scope/'apk')
 evidence=download('subtitle-logs6-android-proof-'+case+'-'+BUILD,scope/'proof')
 proof=json.loads((scope/'proof'/('update-identity.json' if touch else 'standard-identity.json')).read_text())
 assert proof['verified'] and proof['source_commit']==source and proof['build_commit']==BUILD and proof['case']==case
 assert proof['phase']=='final' and proof['test_package']==touch
 assert proof['package']==('space.nuvio.nativelegacy.touch' if touch else 'space.nuvio.nativelegacy')
 assert proof['certificate_sha256']=='862d7ef63a0dbde2929f6f231cae295510726d9ac28f9292496d2a38304be5e6'
 assert proof['native_page_packaging']=='passed' and proof['abis']==['arm64-v8a','armeabi-v7a']
 files=list((scope/'apk').glob('*.apk'));assert len(files)==1;apk=files[0]
 assert sha(apk)==proof['apk_sha256'] and apk.stat().st_size==proof['apk_bytes']
 badging=subprocess.check_output([os.environ['AAPT'],'dump','badging',str(apk)],text=True)
 assert "package: name='"+proof['package']+"'" in badging
 signing=subprocess.check_output([os.environ['APKSIGNER'],'verify','--verbose','--print-certs',str(apk)],text=True)
 assert 'Signer #1 certificate SHA-256 digest: '+proof['certificate_sha256'] in signing
 if touch:assert "versionCode='20016'" in badging and "versionName='2.0.16-test-logs6'" in badging
 if subprocess.run(['git','cat-file','-e',source],capture_output=True).returncode:subprocess.run(['git','fetch','origin',source],check=True)
 target=ROOT/(case+'-source');subprocess.run(['git','worktree','add','--detach',str(target.resolve()),source],check=True)
 subprocess.run(['python3',str(target/'tools/android-apk-pages.py'),str(apk)],check=True)
 c=blob(source,'src/video_android.c').decode()
 assert '"space.nuvio.nativelegacy.NvPlayer"' in c
 lookups=re.findall(r'GetStaticMethodID\(env,\s*gCls,\s*"([^"]+)",\s*"([^"]+)"\)',c)
 assert len(lookups)==(19 if touch else 13) and len({n for n,d in lookups})==len(lookups)
 assert (preload<={n for n,d in lookups}) if touch else not(preload&{n for n,d in lookups})
 actual,dexhash=apk_static_methods(apk)
 wanted=set(lookups)|{(e['name'],e['descriptor']) for e in legacy['methods'] if touch or e['name'] not in preload}
 assert wanted<=actual,('Missing compiled static DEX methods',wanted-actual)
 if touch:
  jni=json.loads((scope/'proof/product-jni-verified.json').read_text())
  assert jni['outcome']=='passed' and jni['source_commit']==source and jni['review_commit']==BUILD
  assert jni['static_method_descriptors']==dict(lookups)
 report={'case':case,'outcome':'passed','source_commit':source,'apk_build_review_commit':BUILD,'postbuild_validation_review_commit':os.environ['GITHUB_SHA'],
  'original_run_id':RUN,'original_job_id':jid,'original_job_conclusion':job['conclusion'],
  'original_failed_check':None if touch else expected_failure,'replacement_check':'Actual APK DEX static method/access/prototype matching against every pinned C lookup and feature-appropriate legacy overload',
  'actual_c_lookup_count':len(lookups),'required_static_method_descriptors':sorted(wanted),'dex_sha256':dexhash,
  'apk_sha256':sha(apk),'apk_bytes':apk.stat().st_size,'package':proof['package'],'certificate_sha256':proof['certificate_sha256'],
  'native_page_packaging':'passed','actual_apk_reverified':True,'artifact':asset,'build_evidence':evidence,'required_original_steps_passed':required}
 (scope/'postbuild-verified.json').write_text(json.dumps(report,indent=2)+'\n');reports.append(report)
 print('Actual APK static DEX/JNI, signer and 16KB verification passed:',case,len(lookups),flush=True)
(ROOT/'android-postbuild-verified.json').write_text(json.dumps({'outcome':'passed','validation_review_commit':os.environ['GITHUB_SHA'],'apk_build_review_commit':BUILD,'cases':reports,'arm_compiler_rerun':False},indent=2)+'\n')
