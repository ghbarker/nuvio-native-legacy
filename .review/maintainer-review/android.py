from pathlib import Path
import argparse,json,re,shlex,subprocess,sys,zipfile
from common import HERE,sha,source_binding
from dex_methods import apk_methods
from safe_android_metadata import cache_for_commands
p=argparse.ArgumentParser()
for key in ['source','proof','output']:p.add_argument('--'+key,type=Path,required=True)
for key in ['aapt','apksigner']:p.add_argument('--'+key,required=True)
a=p.parse_args();s=a.source.resolve();plan,v,m=source_binding(s,a.proof);a.output.parent.mkdir(parents=True,exist_ok=True)
files=list((s/'build/android').glob('*-android-debug.apk'));assert len(files)==1;apk=files[0]
badging=subprocess.check_output([a.aapt,'dump','badging',str(apk)],text=True);version=json.loads((s/'deploy/app/appinfo.json').read_text())['version'];x,y,z=map(int,version.split('.'));code=x*10000+y*100+z
assert "package: name='space.nuvio.nativelegacy'"in badging
assert "versionName='"+version+"'"in badging and "versionCode='"+str(code)+"'"in badging
assert 'Nuvio TV/Mobile Test'not in badging
signing=subprocess.check_output([a.apksigner,'verify','--verbose','--print-certs',str(apk)],text=True);cert=re.search(r'Signer #1 certificate SHA-256 digest: ([0-9a-f]{64})',signing);assert cert
subprocess.run([sys.executable,str(s/'tools/android-apk-pages.py'),str(apk)],check=True)
with zipfile.ZipFile(apk)as archive:
 names=set(archive.namelist())
 for abi in ['arm64-v8a','armeabi-v7a']:
  for lib in ['libmain.so','libSDL2.so','libcurl.so']:assert 'lib/'+abi+'/'+lib in names
 assert any(n.startswith('assets/fonts/')for n in names)
playerbridge=(s/'src/video_android.c').read_text();lookups=re.findall(r'GetStaticMethodID\(env,\s*gCls,\s*"([^"]+)",\s*"([^"]+)"\)',playerbridge)
assert lookups and len(lookups)==len(set(lookups));player,dexes=apk_methods(apk,'Lspace/nuvio/nativelegacy/NvPlayer;')
for signature in lookups:assert signature in player and player[signature]&8,signature
private,_=apk_methods(apk,'Lspace/nuvio/nativelegacy/TestLocalLogs;');assert not private
classes=s/'android/app/build/tmp/kotlin-classes/debug';activity=classes/'space/nuvio/nativelegacy/NuvioActivity.class';playerclass=classes/'space/nuvio/nativelegacy/NvPlayer.class';assert activity.is_file()and playerclass.is_file()
changed={Path(e['path']).name for e in m['paths']if e['path'].startswith('src/')and e['path'].endswith('.c')};arms=[];compiler_hashes={}
for abi in ['arm64-v8a','armeabi-v7a']:
 databases=[]
 for file in (s/'android/app').rglob('compile_commands.json'):
  if abi not in file.parts:continue
  commands=json.loads(file.read_text());main=[];units=[];compilers=set();targets=set()
  relevant=[e for e in commands if any('CMakeFiles/main.dir/'in x for x in (e.get('arguments')or shlex.split(e['command'])))]
  if not relevant:continue
  cache=cache_for_commands(s,file,abi,commands)
  for entry in relevant:
   argv=entry.get('arguments')or shlex.split(entry['command'])
   if not any('CMakeFiles/main.dir/'in token for token in argv):continue
   assert any(token in argv for token in ['-DNV_ANDROID','-DNV_ANDROID=1'])
   assert any(token in argv for token in ['-DNV_TOUCH_UI','-DNV_TOUCH_UI=1'])
   assert not any('NV_TEST_LOCAL_LOGS'in token for token in argv)
   path=Path(entry['file']);path=(path if path.is_absolute()else Path(entry['directory'])/path).resolve()
   assert path.is_relative_to(s/'src')and path.is_file(),'Actual main source is outside selected candidate'
   compiler=Path(argv[0]).resolve();assert compiler.is_file()and compiler.name.startswith('clang')
   assert 'ndk'in compiler.parts and '27.2.12479018'in compiler.parts,'Unexpected Android compiler/toolchain'
   target=[x.split('=',1)[1]for x in argv if x.startswith('--target=')];assert len(target)==1
   assert ('aarch64'in target[0])if abi=='arm64-v8a'else('armv7'in target[0]and'androideabi'in target[0])
   sysroots=[x.split('=',1)[1]for x in argv if x.startswith('--sysroot=')];assert len(sysroots)==1
   assert Path(sysroots[0]).resolve()==compiler.parent.parent/'sysroot'
   if compiler not in compiler_hashes:compiler_hashes[compiler]=sha(compiler)
   compilers.add((compiler.name,compiler_hashes[compiler]));targets.add(target[0]);units.append(str(path.relative_to(s)))
   main.append(path.name)
  if not main:continue
  assert changed<=set(main),sorted(changed-set(main));assert {'vertudo.c','ajustes.c','perfis.c','fontenota.c','sinopse.c','fontefiltro.c'}<=set(main)
  assert len(compilers)==1 and len(targets)==1
  facts={'abi':abi,'compile_database_sha256':sha(file),'cmake_cache_sha256':sha(cache),'source_abi_build_bound':True,'source_relative_units':sorted(units),'compiler':{'name':next(iter(compilers))[0],'sha256':next(iter(compilers))[1],'ndk_version':'27.2.12479018'},'targets':sorted(targets),'sysroot_bound':True,'scope':'Sanitized facts; no command arguments, raw cache or key values'}
  saved=a.output.parent/(abi+'-'+str(len(databases))+'-compile-facts.json');saved.write_text(json.dumps(facts,indent=2)+'\n')
  databases.append({'file':saved.name,'sha256':sha(saved),'compile_database_sha256':sha(file),'cmake_cache_sha256':sha(cache),'source_abi_compiler_bound':True,'main_translation_units':len(main),'changed_translation_units':sorted(changed)})
 assert databases,'Missing actual ARM compile database '+abi;arms.append({'abi':abi,'compile_databases':databases})
source_binding(s,a.proof)
result={'outcome':'passed','source_binding':v,'plan_sha256':sha(HERE/'plan.json'),'apk_sha256':sha(apk),'apk_bytes':apk.stat().st_size,'package':'space.nuvio.nativelegacy','version':version,'version_code':code,'certificate_sha256':cert.group(1),'signer_scope':'CI standard debug certificate; validation only, no release','native_16kb_pages':'passed','all_player_jni_lookups':lookups,'apk_dex_sha256':dexes,'android_classes':{'activity':sha(activity),'player':sha(playerclass)},'actual_arm_compiles':arms,'private_logging_absent':True}
a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(result,indent=2)+'\n');print('PASS standard Android bothARM/16KB/DEX/JNI',result['apk_sha256'])
