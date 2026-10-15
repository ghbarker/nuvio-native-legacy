#!/usr/bin/env python3
"""Build and verify one exact standalone public app; never publish an APK."""
from pathlib import Path
import argparse, hashlib, json, os, re, shlex, shutil, subprocess, sys, zipfile
from common import git, sha, verify
from dex_methods import apk_methods

p=argparse.ArgumentParser()
for key in ['source','proof','output']:p.add_argument('--'+key,type=Path,required=True)
a=p.parse_args();source=a.source.resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
config,proof,manifest=verify(source,a.proof)
assert not (Path.home()/'.nuvio-android/release.env').exists()
for key in ['NUVIO_KEYSTORE','NUVIO_KEYSTORE_PASS','NUVIO_KEY_ALIAS','NUVIO_KEY_PASS']:
    assert not os.environ.get(key), 'Release signing must not be configured'
assert Path(os.environ['NUVIO_PROPERTIES']).is_file() and Path(os.environ['NUVIO_TEST_DEBUG_KEYSTORE']).is_file()
tools=Path(os.environ['ANDROID_HOME'])/'build-tools/35.0.0'
for name in ['aapt','apksigner','zipalign']:assert (tools/name).is_file()
gradle=source/'android/app/build.gradle.kts';original=gradle.read_bytes();text=original.decode()
marker='    signingConfigs {\n'
signer='''        getByName("debug") {
            storeFile = file(System.getenv("NUVIO_TEST_DEBUG_KEYSTORE") ?: error("Persistent test signing key missing"))
            storePassword = "android"
            keyAlias = "androiddebugkey"
            keyPassword = "android"
        }
'''
assert text.count(marker)==1 and 'getByName("debug")' not in text
transformed=text.replace(marker,marker+signer,1).encode()
env=dict(os.environ,NUVIO_REQUIRE_CORE='1',NUVIO_P2P_MOTOR='none',NUVIO_BUILD_JOBS='2',NUVIO_ANDROID_JOBS='2')
private_values=[]
for line in Path(os.environ['NUVIO_PROPERTIES']).read_text().splitlines():
    if '=' in line and not line.lstrip().startswith('#'):
        value=line.split('=',1)[1].strip()
        if len(value)>=6:private_values.append(value)
def safe_log(text):
    for value in sorted(private_values,key=len,reverse=True):text=text.replace(value,'<withheld>')
    return text
def run(cmd):return subprocess.check_output(list(map(str,cmd)),cwd=source,env=env,text=True,stderr=subprocess.STDOUT)
try:
    gradle.write_bytes(transformed);verify(source,a.proof,signer_transform=True)
    # Production dependency recipe verifies its own ABI/NDK/version stamps even after cache restoration.
    for label,cmd in [('deps',['bash','tools/android/deps.sh','arm64-v8a','armeabi-v7a']),('build',['bash','tools/android.sh'])]:
        result=subprocess.run(cmd,cwd=source,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
        logged=safe_log(result.stdout)
        (out/(label+'.log')).write_text(logged)
        assert not result.returncode,(label,logged[-2000:])
    version=json.loads((source/'deploy/app/appinfo.json').read_text())['version']
    built=source/'build/android'/('Nuvio-'+version+'-android-debug.apk');assert built.is_file()
    apk=out/built.name;shutil.copy2(built,apk)
    badging=run([tools/'aapt','dump','badging',apk]);code=sum(n*v for n,v in zip(map(int,version.split('.')),[10000,100,1]))
    assert re.search(r"^package: name='space\.nuvio\.nativelegacy'",badging,re.M)
    assert "versionName='"+version+"'" in badging and "versionCode='"+str(code)+"'" in badging
    assert 'Nuvio TV/Mobile Test' not in badging
    signing=run([tools/'apksigner','verify','--verbose','--print-certs',apk])
    assert 'Signer #1 certificate SHA-256 digest: '+config['ci_signing_certificate_sha256'] in signing
    pages=run([sys.executable,source/'tools/android-apk-pages.py',apk]);(out/'pages.log').write_text(pages)
    run([tools/'zipalign','-c','-P','16','4',apk])
    with zipfile.ZipFile(apk) as z:
        names=set(z.namelist())
        for abi in ['arm64-v8a','armeabi-v7a']:
            for lib in ['libmain.so','libSDL2.so','libcurl.so']:assert 'lib/'+abi+'/'+lib in names
        assert {n.split('/')[1] for n in names if n.startswith('lib/') and n.endswith('.so')}=={'arm64-v8a','armeabi-v7a'}
        assert any(n.startswith('assets/art/') for n in names) and any(n.startswith('assets/fonts/') for n in names)
    methods,dexes=apk_methods(apk,'Lspace/nuvio/nativelegacy/NvPlayer;');assert methods
    bridge=(source/'src/video_android.c').read_text()
    lookups=re.findall(r'GetStaticMethodID\(env,\s*gCls,\s*"([^"]+)",\s*"([^"]+)"\)',bridge)
    assert lookups and len(lookups)==len(set(lookups))
    for signature in lookups:assert signature in methods and methods[signature]&8, signature
    private,_=apk_methods(apk,'Lspace/nuvio/nativelegacy/TestLocalLogs;');assert not private
    arms=[]
    for abi in ['arm64-v8a','armeabi-v7a']:
        records=[]
        for file in (source/'android/app').rglob('compile_commands.json'):
            if abi not in file.parts:continue
            main=[]
            for entry in json.loads(file.read_text()):
                argv=entry.get('arguments') or shlex.split(entry['command'])
                if not any('CMakeFiles/main.dir/' in x for x in argv):continue
                assert any(x in argv for x in ['-DNV_ANDROID','-DNV_ANDROID=1'])
                assert any(x in argv for x in ['-DNV_TOUCH_UI','-DNV_TOUCH_UI=1'])
                assert '-DNV_TEST_LOCAL_LOGS=1' not in argv
                file_source=Path(entry['file']).resolve();assert file_source.is_relative_to(source/'src')
                main.append(file_source.name)
            if not main:continue
            assert {'app.c','ajustes.c','registro.c','layout.c','ponteiro.c'}<=set(main)
            records.append({'compile_commands_sha256':sha(file),'actual_main_sources':len(main)})
        assert records,'Missing actual dual-ABI compile metadata: '+abi
        arms.append({'abi':abi,'actual_compilations':records})
    assert gradle.read_bytes()==transformed
    verify(source,a.proof,signer_transform=True)
    receipt={'outcome':'passed','source_binding':proof,'apk_sha256':sha(apk),'apk_bytes':apk.stat().st_size,'package':'space.nuvio.nativelegacy','version':version,'version_code':code,'ci_debug_certificate_sha256':config['ci_signing_certificate_sha256'],'native_16kb_pages':'passed','zip_alignment':'passed','actual_arms':arms,'actual_jni_lookups':lookups,'apk_dex_sha256':dexes,'private_logs_absent':True,'ci_only_signer_transform':{'before_sha256':hashlib.sha256(original).hexdigest(),'after_sha256':hashlib.sha256(transformed).hexdigest()},'reused_test_passes':False,'scope':'Real SDK complete dual-ABI Android app build and actual APK/DEX/JNI verification. No device/emulator execution or APK publication.'}
finally:
    gradle.write_bytes(original)
    verify(source,a.proof)
assert receipt['outcome']=='passed'
receipt['source_restored_clean']=True
(out/'android-verified.json').write_text(json.dumps(receipt,indent=2)+'\n')
print('Passed actual standalone ARM32/ARM64 Android app, JNI/DEX and16KB checks')
