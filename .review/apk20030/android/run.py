#!/usr/bin/env python3
"""Build exactly one already materialized layer; no dispatch, upload or release."""
from pathlib import Path
import argparse,json,os,shutil,subprocess,sys
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
 version=json.loads((s/'deploy/app/appinfo.json').read_text())['version'];built=s/'build/android'/('Nuvio-'+version+'-android-debug.apk');assert built.is_file()
 name=('Nuvio-'+plan['identity']['version_name']+'-tv-mobile-test.apk') if a.layer=='test' else built.name
 apk=out/name;shutil.copy2(built,apk)
 run([sys.executable,str(here/'verify_apk.py'),*common,'--transforms',str(transform),'--apk',str(apk),'--aapt',str(tools/'aapt'),'--apksigner',str(tools/'apksigner'),'--output',str(out/'android-verified.json')])
 run([sys.executable,str(here/'prepare_source.py'),*common,'--output',str(transform),'--restore'])
finally:
 run([sys.executable,str(here/'android_workspace.py'),'--source',str(s),'--output',str(workspace),'--phase','after'])
_,_,_,git=binding(s,a.source_proof,a.layer);assert not git('status','--porcelain')
record={'verified':True,'layer':a.layer,'source_commit':proof['source_commit'],'tree':proof['tree'],'review_commit':proof['review_commit'],'plan_sha256':file_sha(here.parent/'plan.json'),'source_proof_sha256':file_sha(a.source_proof),'source_restored_clean':True,'workspace_before_sha256':file_sha(workspace/'before.json'),'workspace_after_sha256':file_sha(workspace/'after.json'),'apk_sha256':file_sha(apk),'apk_name':name,'verification_sha256':file_sha(out/'android-verified.json'),'ci_only_transforms_sha256':file_sha(transform),'fresh_build_of_both_arms':True,'prior_apk_proofs_reused':False,'no_dispatch_release_upload_or_backend_actions':True}
(out/'build-receipt.json').write_text(json.dumps(record,indent=2)+'\n');print('Complete actual Android build verified; source restored clean:',a.layer,name)
