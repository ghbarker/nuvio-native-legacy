#!/usr/bin/env python3
"""Verify the real APK/DEX/Kotlin/JNI and both actual native compile databases."""
from pathlib import Path
import argparse,hashlib,json,re,shlex,subprocess,sys,zipfile
from common import file_sha,transformed
from dex_methods import apk_methods
from safe_android_metadata import collect,cache_for_commands
p=argparse.ArgumentParser()
for k in ('source','proof','transforms','apk','output'):p.add_argument('--'+k,type=Path,required=True)
for k in ('aapt','apksigner'):p.add_argument('--'+k,required=True)
p.add_argument('--layer',choices=['product','test'],required=True);a=p.parse_args();s=a.source.resolve();apk=a.apk.resolve()
plan,v,m,tr=transformed(s,a.proof,a.layer,a.transforms);test=a.layer=='test';ident=plan['identity'];assert apk.is_file()
# Persist safe facts before any later verifier assertion can abort the job.
safe=collect(s,a.layer);safe.update(source_commit=v['source_commit'],tree=v['tree'],review_commit=v['review_commit'],apk_sha256=file_sha(apk))
a.output.parent.mkdir(parents=True,exist_ok=True)
a.output.with_name('android-metadata-evidence.json').write_text(json.dumps(safe,indent=2)+'\n')
badging=subprocess.check_output([a.aapt,'dump','badging',str(apk)],text=True)
package=ident['package'] if test else 'space.nuvio.nativelegacy'
version=ident['version_name'] if test else json.loads((s/'deploy/app/appinfo.json').read_text())['version']
vparts=list(map(int,version.split('.'))) if not test else None
code=ident['version_code'] if test else vparts[0]*10000+vparts[1]*100+vparts[2]
assert re.search(r"^package: name='"+re.escape(package)+r"'",badging,re.M)
assert "versionCode='"+str(code)+"'" in badging and "versionName='"+version+"'" in badging
if test:assert re.search(r"^application-label:'"+re.escape(ident['label'])+r"'",badging,re.M)
else:assert ident['label'] not in badging
signing=subprocess.check_output([a.apksigner,'verify','--verbose','--print-certs',str(apk)],text=True)
assert 'Signer #1 certificate SHA-256 digest: '+ident['certificate_sha256'] in signing
subprocess.run([sys.executable,str(s/'tools/android-apk-pages.py'),str(apk)],check=True)
xml=subprocess.check_output([a.aapt,'dump','xmltree',str(apk),'AndroidManifest.xml'],text=True);lines=xml.splitlines();providers=[]
for n,line in enumerate(lines):
 match=re.match(r'^(\s*)E: provider\b',line)
 if not match:continue
 indent=len(match.group(1));end=next((i for i in range(n+1,len(lines)) if re.match(r'^\s*E:',lines[i]) and len(lines[i])-len(lines[i].lstrip())<=indent),len(lines))
 providers.append('\n'.join(lines[n:end]))
authority=package+'.atualizacao';blocks=[b for b in providers if re.search(r'android:authorities[^\n]*="'+re.escape(authority)+r'"',b)];assert len(blocks)==1
block=blocks[0];assert 'androidx.core.content.FileProvider' in block and 'android.support.FILE_PROVIDER_PATHS' in block
assert re.search(r'android:exported[^\n]*\(type\s+0x12\)0x0\b',block)
assert re.search(r'android:grantUriPermissions[^\n]*\(type\s+0x12\)0x(?:ffffffff|1)\b',block)
paths=subprocess.check_output([a.aapt,'dump','xmltree',str(apk),'res/xml/caminhos_atualizacao.xml'],text=True)
assert not re.search(r'E: (?:cache-path|root-path|external-path|external-cache-path)\b',paths)
assert 'name="atualizacao"' in paths and 'path="dados/atualizacao/"' in paths
with zipfile.ZipFile(apk) as z:
 names=set(z.namelist())
 for abi in ('arm64-v8a','armeabi-v7a'):
  for lib in ('libmain.so','libSDL2.so','libcurl.so'):assert 'lib/'+abi+'/'+lib in names
 assert any(n.startswith('assets/art/') for n in names) and any(n.startswith('assets/fonts/') for n in names)
classes=s/'android/app/build/tmp/kotlin-classes/debug';compiled={};dexes={}
requirements={
 'NuvioActivity':[('baixarRegistro','(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V'),('onActivityResult','(IILandroid/content/Intent;)V')],
 'RegistroDownload':[('abrir','(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V'),('resultado','(IILandroid/content/Intent;)Z'),('fechar','()V')],
 'RegistroDownload$Companion':[('documento','()Landroid/content/Intent;'),('lerFim','(Ljava/io/File;)Ljava/lang/String;'),('limpar','(Ljava/lang/String;)Ljava/lang/String;')]}
external_activity=[('playerExternoPedir','(JI[B[B[BJ[Ljava/lang/String;)Z'),('playerExternoAbrir','(J)Z'),('playerExternoCancelar','(J)V'),('playerExternoEvento','()[J'),('playerExternoOcupado','()Z'),('playerExternoAviso','(Ljava/lang/String;)V')]
requirements['NuvioActivity']+=external_activity
for klass,required in requirements.items():
 name='space.nuvio.nativelegacy.'+klass;file=classes/(name.replace('.','/')+'.class');assert file.is_file()
 text=subprocess.check_output(['javap','-s','-p','-classpath',str(classes),name],text=True)
 actual,dexes=apk_methods(apk,'L'+name.replace('.','/')+';')
 for method,descriptor in required:
  assert re.search(r'\b'+re.escape(method)+r'\([^\n]*\);\s*descriptor:\s*'+re.escape(descriptor),text),(name,method)
  assert (method,descriptor) in actual and not actual[method,descriptor]&8
 compiled[name]={'class_sha256':file_sha(file),'methods':required,'javap_sha256':hashlib.sha256(text.encode()).hexdigest()}
bridge=(s/'src/android.c').read_text();download=re.findall(r'GetMethodID\(env,\s*cls,\s*"baixarRegistro",\s*"([^"]+)"\)',bridge);assert download==[requirements['NuvioActivity'][0][1]]

literal=re.findall(r'GetMethodID\(env,\s*cls,\s*"(playerExterno[^" ]+)",\s*"([^" ]+)"\)',bridge)
assert set(literal)=={(n,d) for n,d in external_activity if n not in ('playerExternoAbrir','playerExternoCancelar')}
assert re.search(r'GetMethodID\(env,\s*cls,\s*nome,\s*resultado\s*\?\s*"\(J\)Z"\s*:\s*"\(J\)V"\)',bridge)
assert re.search(r'android_player_externo_abrir\(uint64_t token\)\s*\{\s*return externoToken\("playerExternoAbrir", token, 1\);\s*\}',bridge)
assert re.search(r'android_player_externo_cancelar\(uint64_t token\)\s*\{\s*\(void\)externoToken\("playerExternoCancelar", token, 0\);\s*\}',bridge)
playerbridge=(s/'src/video_android.c').read_text();lookups=re.findall(r'GetStaticMethodID\(env,\s*gCls,\s*"([^"]+)",\s*"([^"]+)"\)',playerbridge)
assert len(lookups)>=19 and len(lookups)==len(set(lookups));required={'preparar','prepararProximo','prepararEstado','cancelarPreparo','abrirPreparada','pararMantendoPreparo'};assert required<={n for n,d in lookups}
actual,_=apk_methods(apk,'Lspace/nuvio/nativelegacy/NvPlayer;')
legacy=json.loads((Path(__file__).with_name('legacy-jni-descriptors.json')).read_text())
for signature in lookups+[(e['name'],e['descriptor']) for e in legacy['methods']]:assert signature in actual and actual[signature]&8,signature
playerclass=classes/'space/nuvio/nativelegacy/NvPlayer.class';assert playerclass.is_file()
text=subprocess.check_output(['javap','-s','-p','-classpath',str(classes),'space.nuvio.nativelegacy.NvPlayer'],text=True)
for method,descriptor in lookups:assert re.search(r'\bstatic\s+[^\n]*\b'+re.escape(method)+r'\([^\n]*\);\s*descriptor:\s*'+re.escape(descriptor),text),(method,descriptor)
private,_=apk_methods(apk,'Lspace/nuvio/nativelegacy/TestLocalLogs;')
assert bool(private)==test,'Private diagnostics class leaked into product or is absent from test'
if test:assert ('snapshot','()[B') in private
config=list((s/'android/app/build/generated').rglob('BuildConfig.java'));fields=[]
for f in config:
 content=f.read_text();matches=re.findall(r'public static final boolean NV_TEST_LOCAL_LOGS\s*=\s*(true|false)\s*;',content)
 fields.extend((str(f),x) for x in matches)
if test:assert len(fields)==1 and fields[0][1]=='true'
else:assert all(value=='false' for _,value in fields)
arms=[];compiler_hashes={}
for abi in ('arm64-v8a','armeabi-v7a'):
 records=[]
 for f in (s/'android/app').rglob('compile_commands.json'):
  if abi not in f.parts:continue
  main=[];reports=set();units=[];compilers=set();targets=set()
  for entry in json.loads(f.read_text()):
   argv=entry.get('arguments') or shlex.split(entry['command'])
   if not any('CMakeFiles/main.dir/' in x for x in argv):continue
   assert any(x in argv for x in ('-DNV_ANDROID','-DNV_ANDROID=1')) and any(x in argv for x in ('-DNV_TOUCH_UI','-DNV_TOUCH_UI=1'))
   assert ('-DNV_TEST_LOCAL_LOGS=1' in argv)==test
   report=[x.split('=',1)[1] for x in argv if x.startswith('-DNV_REC_URL=')];assert len(report)==1
   reports.add(bool(report[0].replace(chr(92),'').strip(chr(34))))
   path=Path(entry['file']);path=(path if path.is_absolute() else Path(entry['directory'])/path).resolve()
   assert path.is_relative_to(s/'src') and path.is_file(),'Compiler source is outside selected tree'
   compiler=Path(argv[0]).resolve();assert compiler.is_file() and compiler.name.startswith('clang')
   assert 'ndk' in compiler.parts and '27.2.12479018' in compiler.parts
   target=[x.split('=',1)[1] for x in argv if x.startswith('--target=')];assert len(target)==1
   assert ('aarch64' in target[0]) if abi=='arm64-v8a' else ('armv7' in target[0] and 'androideabi' in target[0])
   sysroots=[x.split('=',1)[1] for x in argv if x.startswith('--sysroot=')];assert len(sysroots)==1
   assert Path(sysroots[0]).resolve()==compiler.parent.parent/'sysroot'
   if compiler not in compiler_hashes:compiler_hashes[compiler]=file_sha(compiler)
   compilers.add((compiler.name,compiler_hashes[compiler]));targets.add(target[0])
   units.append(str(path.relative_to(s)));main.append(path.name)
  if not main:continue
  assert {'app.c','posplay.c','player.c','proximo.c','video_android.c','text.c','avisos.c','android.c','ajustes.c','stream_parse.c','fontefiltro.c'}<=set(main)
  cmake=(s/'android/app/src/main/cpp/CMakeLists.txt').read_text()
  assert r'list(FILTER NUVIO_SRC EXCLUDE REGEX "/(video_tizen|video_tpk|tpk)\\.c$")' in cmake
  excluded={'video_tizen.c','video_tpk.c','tpk.c'}
  changed={Path(e['path']).name for e in m['files'] if e['path'].startswith('src/') and e['path'].endswith('.c') and '/' not in e['path'][4:]}-excluded
  assert changed<=set(main),'Changed native units are absent from actual ARM compile commands: '+str(sorted(changed-set(main)))
  assert len(units)==len(set(units)) and set(units)==set(m['expected_android_units']),'Full actual selected Android source coverage differs'
  assert len(compilers)==len(targets)==1
  assert reports=={True},'Original Send logs endpoint must be configured'
  cache=cache_for_commands(s,f,abi,json.loads(f.read_text()))
  entries=[line for line in cache.read_text().splitlines() if re.match(r'^\s*NV_TEST_LOCAL_LOGS(?=[:=]|$)',line)]
  if test:
   assert entries==['NV_TEST_LOCAL_LOGS:BOOL=ON'],'Private build requires exactly one explicit BOOL=ON cache entry'
  else:
   assert 'NV_TEST_LOCAL_LOGS' not in cmake,'Product CMake must not declare private diagnostics'
   assert not entries,'Product cache must not contain the private diagnostics option'
  facts={'abi':abi,'compile_database_sha256':file_sha(f),'cmake_cache_sha256':file_sha(cache),
   'source_relative_units':sorted(units),'compiler':{'name':next(iter(compilers))[0],
    'sha256':next(iter(compilers))[1],'ndk_version':'27.2.12479018'},'targets':sorted(targets),
   'sysroot_bound':True,'source_abi_build_bound':True,'private_native_opt_in':test,
   'scope':'Sanitized facts only: no raw command arguments, config values or cache text.'}
  factpath=a.output.parent/(abi+'-'+str(len(records))+'-compile-facts.json')
  factpath.write_text(json.dumps(facts,indent=2)+'\n')
  records.append({'compile_commands_sha256':file_sha(f),'main_sources':len(main),'report_endpoint_configured':True,
   'private_native_opt_in':test,'source_abi_compiler_bound':True,'facts_file':factpath.name,'facts_sha256':file_sha(factpath)})
 assert records,'Missing actual full native compile proof: '+abi;arms.append({'abi':abi,'builds':records})
# Recheck every permitted CI mutation and all other manifest files after compilation.
transformed(s,a.proof,a.layer,a.transforms)
r={'outcome':'passed','source_commit':v['source_commit'],'tree':v['tree'],'layer':a.layer,'review_commit':v['review_commit'],'plan_sha256':file_sha(Path(__file__).resolve().parent.parent/'plan.json'),'source_proof_sha256':file_sha(a.proof),'ci_transforms_sha256':file_sha(a.transforms),'apk_sha256':file_sha(apk),'apk_bytes':apk.stat().st_size,'apk_name':apk.name,'package':package,'version_name':version,'version_code':code,'certificate_sha256':ident['certificate_sha256'],'compiled_android_classes':compiled,'apk_dex_sha256':dexes,'external_player_activity_jni_methods':external_activity,'external_player_activity_jni_instance_methods_verified':True,'all_player_jni_lookups':lookups,'legacy_jni_overloads_preserved':True,'actual_arm_compiles':arms,'native_16kb_pages':'passed','private_log_opt_in':test,'file_provider':{'authority':authority,'nonexported':True,'updater_path_only':True},'scope':'Fresh complete Kotlin/API and ARM32/ARM64 compilation plus actual APK DEX/JNI/package checks. No emulator or physical-device execution claimed.','prior_apk_proofs_reused':False,'release_or_backend_actions':False}
a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(r,indent=2)+'\n');print('PASS verified actual',a.layer,'Android APK:',r['apk_sha256'])
