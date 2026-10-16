#!/usr/bin/env python3
"""Build exactly one already materialized layer; no dispatch, upload or release."""
from pathlib import Path
import argparse,hashlib,json,os,re,shutil,subprocess,sys,xml.etree.ElementTree as ET
from common import binding,file_sha
p=argparse.ArgumentParser()
for k in ('source','source-proof','output'):p.add_argument('--'+k,type=Path,required=True)
p.add_argument('--layer',choices=['product','test'],required=True);p.add_argument('--jobs',type=int,default=2);a=p.parse_args();s=a.source.resolve();out=a.output.resolve();here=Path(__file__).resolve().parent
plan,proof,manifest,git=binding(s,a.source_proof,a.layer);assert not git('status','--porcelain');assert 1<=a.jobs<=4
for path in ('android/app/build','android/app/.cxx','android/app/.externalNativeBuild','build/android'):
 assert not (s/path).exists(),'Require fresh materialization without prior APK/native build outputs: '+path
out.mkdir(parents=True,exist_ok=True)
assert not (Path.home()/'.nuvio-android/release.env').exists(),'Use an isolated runner without release credentials'
for k in ('NUVIO_KEYSTORE','NUVIO_KEYSTORE_PASS','NUVIO_KEY_ALIAS','NUVIO_KEY_PASS'):assert not os.environ.get(k),'Release signing must not be configured'
assert Path(os.environ['NUVIO_PROPERTIES']).is_file() and Path(os.environ['NUVIO_TEST_DEBUG_KEYSTORE']).is_file()
sdk=Path(os.environ['ANDROID_HOME']);tools=sdk/'build-tools/35.0.0';assert (tools/'aapt').is_file() and (tools/'apksigner').is_file()
assert shutil.which('sdkmanager'),'setup-android must put sdkmanager on PATH'
for path in ('build-tools/35.0.0/aapt','build-tools/35.0.0/apksigner','ndk/27.2.12479018/toolchains/llvm/prebuilt/linux-x86_64/bin/clang','ndk/27.2.12479018/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-readelf'):
 assert os.access(sdk/path,os.X_OK),'Pinned Android build executable unavailable: '+path
transform=out/'ci-transforms.json';common=['--source',str(s),'--proof',str(a.source_proof.resolve()),'--layer',a.layer]
workspace=out/'workspace'

def unit_tests(env):
 expected={
  'ordinaryStreamsUseOneImplicitSystemChooserTarget',
  'requiredHeadersUseOnlyExplicitVerifiedMxComponents',
  'absentOrUnsupportedHeaderPlayersNeverCreateGenericBase',
  'malformedHeadersRefuseRatherThanSilentlyDropCredentials',
  'urlsAndCapacityAreValidatedWithoutRewritingSignedQuery',
  'resumeUsesIntMillisecondsWithoutOutputOnlyVlcExtras'}
 klass='space.nuvio.nativelegacy.PlayerExternoContratoTest'
 report=s/'android/app/build/test-results/testDebugUnitTest'
 assert not report.exists(),'Unit reports must not predate this actual test invocation'
 env=dict(env);env.setdefault('ANDROID_NDK_HOME',str(sdk/'ndk/27.2.12479018'))
 cache=Path(env.get('NUVIO_ANDROID_CACHE',str(Path.home()/'.cache/nuvio-android')))
 command=['bash','android/gradlew','-p','android','--console=plain',
  '-Pnuvio.sdlSrc='+str(cache/'src'),'-Pnuvio.estagio='+str(s/'build/android'),
  '-Pnuvio.ffmpegAar='+str(s/'build/android/media3-ffmpeg-16kb.aar'),
  '--max-workers='+str(a.jobs),':app:testDebugUnitTest']
 result=subprocess.run(command,cwd=s,env=env,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
 print(result.stdout,flush=True)
 assert result.returncode==0,'Actual Android JUnit task failed'
 task=[x.strip() for x in result.stdout.splitlines() if re.match(r'^> Task :app:testDebugUnitTest(?:\s|$)',x)]
 assert task==['> Task :app:testDebugUnitTest'],'Require actual task execution, not skipped/up-to-date/no-source'
 reports=[];methods=[]
 for file in sorted(report.glob('TEST-*.xml')):
  raw=file.read_text();root=ET.fromstring(raw);assert root.tag=='testsuite'
  assert all(int(root.get(k,'0'))==0 for k in ('failures','errors','skipped'))
  cases=root.findall('testcase');assert int(root.get('tests','-1'))==len(cases)>0
  assert not root.findall('.//failure') and not root.findall('.//error') and not root.findall('.//skipped')
  assert not root.findall('.//property') and all(not (x.text or '').strip() for tag in ('system-out','system-err') for x in root.findall(tag)),'Unexpected private test output'
  if root.get('name')==klass:
   assert len(cases)==6 and {x.get('name') for x in cases}==expected
   assert all(x.get('classname')==klass for x in cases);methods=[x.get('name') for x in cases]
  reports.append({'name':file.name,'sha256':file_sha(file),'xml':raw,'tests':len(cases)})
 assert reports and len(methods)==6
 classes=s/'android/app/build/tmp/kotlin-classes/debugUnitTest'
 compiled=classes/(klass.replace('.','/')+'.class');assert compiled.is_file()
 test_source=s/'android/app/src/test/java/space/nuvio/nativelegacy/PlayerExternoContratoTest.kt'
 assert set(re.findall(r'@Test\s+fun\s+(\w+)\(',test_source.read_text()))==expected
 cc=Path(shutil.which('cc')).resolve();native=[]
 for mode in ('0','1'):
  for script,count,prefix in [('tests/playerexterno.sh',8,'PASS: '),('tests/playerexterno_integracao.sh',6,'ok external native: ')]:
   fixture=s/script.replace('.sh','.c');markers=re.findall(r'puts\("([^"\n]+)"\)',fixture.read_text())
   assert len(markers)==count and all(x.startswith(prefix) for x in markers)
   cenv=dict(env,CC=str(cc),SANITIZE=mode,ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1')
   run_result=subprocess.run(['bash',script],cwd=s,env=cenv,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
   print(run_result.stdout,flush=True);assert run_result.returncode==0,(script,mode,'host C regression failed')
   lines=run_result.stdout.splitlines();assert [x for x in lines if x.startswith(prefix)]==markers
   native.append({'script':script,'script_sha256':file_sha(s/script),'fixture_sha256':file_sha(fixture),'sanitize':mode=='1','returncode':0,'markers':markers,'output':run_result.stdout,'output_sha256':hashlib.sha256(run_result.stdout.encode()).hexdigest()})
 manifest_script='tests/android_manifest_queries.sh';manifest_python='tests/android_manifest_queries.py'
 manifest_result=subprocess.run(['bash',manifest_script],cwd=s,env=env,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
 print(manifest_result.stdout,flush=True)
 assert manifest_result.returncode==0 and manifest_result.stdout.splitlines()[-1]=='android_manifest_queries: 14 parser cases passed; 6 query intents valid','Installer source-query regression failed'
 manifest_test={'passed':True,'script':manifest_script,'script_sha256':file_sha(s/manifest_script),'python_sha256':file_sha(s/manifest_python),'parser_cases':14,'source_query_intents':6,'returncode':0,'output':manifest_result.stdout,'output_sha256':hashlib.sha256(manifest_result.stdout.encode()).hexdigest()}
 receipt={'passed':True,'source_commit':proof['source_commit'],'tree':proof['tree'],'review_commit':proof['review_commit'],
  'manifest_query_source_tests':manifest_test,'task':':app:testDebugUnitTest','unfiltered':True,'task_executed':True,'returncode':0,
  'gradle_log_sha256':hashlib.sha256(result.stdout.encode()).hexdigest(),'reports':reports,'required_class':klass,
  'required_methods':sorted(methods),'required_tests':6,'tests':sum(x['tests'] for x in reports),
  'test_source_sha256':file_sha(test_source),'compiled_test_class_sha256':file_sha(compiled),
  'host_c_compiler':{'name':cc.name,'sha256':file_sha(cc)},'host_c_groups':native,'host_c_groups_required':4,
  'scope':'Actual unfiltered Gradle JVM unit tests plus four hosted C groups; no Android UI, emulator, device or external-player installation execution.'}
 (out/'unit-tests.json').write_text(json.dumps(receipt,indent=2)+'\n')

def run(c,env=None):subprocess.run(c,cwd=s,env=env,check=True)
run([sys.executable,str(here/'android_workspace.py'),'--source',str(s),'--output',str(workspace),'--phase','before'])
try:
 run([sys.executable,str(here/'prepare_source.py'),*common,'--output',str(transform)])
 env=dict(os.environ,NUVIO_P2P_MOTOR='none',NUVIO_REQUIRE_CORE='1',NUVIO_ANDROID_JOBS=str(a.jobs),NUVIO_BUILD_JOBS=str(a.jobs))
 # gradlew runs under /bin/sh, which drops environment keys containing dots.
 # Its POSIX-safe GRADLE_OPTS reaches Java as an explicit project system property.
 env.pop('ORG_GRADLE_PROJECT_nuvio.testLocalLogs', None)
 flag = 'true' if a.layer=='test' else 'false'
 env['GRADLE_OPTS'] = (env.get('GRADLE_OPTS', '') + ' -Dorg.gradle.project.nuvio.testLocalLogs=' + flag).strip()
 run(['bash','tools/android/deps.sh','arm64-v8a','armeabi-v7a'],env)
 run(['bash','tools/android.sh'],env)
 unit_tests(env)
 version=json.loads((s/'deploy/app/appinfo.json').read_text())['version'];built=s/'build/android'/('Nuvio-'+version+'-android-debug.apk');assert built.is_file()
 name=('Nuvio-'+plan['identity']['version_name']+'-tv-mobile-test.apk') if a.layer=='test' else built.name
 apk=out/name;shutil.copy2(built,apk)
 run([sys.executable,str(here/'verify_apk.py'),*common,'--transforms',str(transform),'--apk',str(apk),'--aapt',str(tools/'aapt'),'--apksigner',str(tools/'apksigner'),'--output',str(out/'android-verified.json')])
 run([sys.executable,str(here/'prepare_source.py'),*common,'--output',str(transform),'--restore'])
finally:
 run([sys.executable,str(here/'android_workspace.py'),'--source',str(s),'--output',str(workspace),'--phase','after'])
_,_,_,git=binding(s,a.source_proof,a.layer);assert not git('status','--porcelain')
record={'unit_tests_sha256':file_sha(out/'unit-tests.json'),'android_junit_required_passed':True,'host_c_groups_passed':4,'verified':True,'layer':a.layer,'source_commit':proof['source_commit'],'tree':proof['tree'],'review_commit':proof['review_commit'],'plan_sha256':file_sha(here.parent/'plan.json'),'source_proof_sha256':file_sha(a.source_proof),'source_restored_clean':True,'workspace_before_sha256':file_sha(workspace/'before.json'),'workspace_after_sha256':file_sha(workspace/'after.json'),'apk_sha256':file_sha(apk),'apk_name':name,'verification_sha256':file_sha(out/'android-verified.json'),'ci_only_transforms_sha256':file_sha(transform),'fresh_build_of_both_arms':True,'prior_apk_proofs_reused':False,'no_dispatch_release_upload_or_backend_actions':True}
(out/'build-receipt.json').write_text(json.dumps(record,indent=2)+'\n');print('Complete actual Android build verified; source restored clean:',a.layer,name)
