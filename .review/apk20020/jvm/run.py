#!/usr/bin/env python3
"""Fresh 32 JVM and 15 isolated real-Media3 contracts; no historical test reuse."""
from pathlib import Path
import argparse,hashlib,json,os,shutil,subprocess,sys,xml.etree.ElementTree as ET
HERE=Path(__file__).resolve().parent
sys.path.insert(0,str(HERE.parent))
from require_plan import load_plan,verify_source,verify_inherited
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def write(p,value):p.parent.mkdir(parents=True,exist_ok=True);p.write_text(json.dumps(value,indent=2)+'\n')
def invoke(command,log):
 result=subprocess.run(command,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
 log.parent.mkdir(parents=True,exist_ok=True);log.write_text(result.stdout)
 assert result.returncode==0,(str(log),result.stdout[-3000:])
 return {'command':command,'log_sha256':sha(log)}
def java_identity():
 home=Path(os.environ['JAVA_HOME']).resolve();java=home/'bin/java';release=home/'release'
 assert java.is_file() and release.is_file()
 result=subprocess.run([str(java),'-version'],text=True,capture_output=True);assert result.returncode==0
 lines=[line for line in (result.stdout+result.stderr).splitlines() if line.startswith(('openjdk version','java version','OpenJDK Runtime','OpenJDK 64-Bit','Java(TM)'))]
 assert lines and ('"21.' in lines[0] or '"21"' in lines[0]),'Temurin Java21 is required'
 return {'executable':str(java.resolve()),'executable_sha256':sha(java.resolve()),'release_sha256':sha(release),'modules_sha256':sha(home/'lib/modules'),'version_output':lines,'option_environment_sha256':{key:hashlib.sha256(os.environ[key].encode()).hexdigest() for key in ['JAVA_TOOL_OPTIONS','JDK_JAVA_OPTIONS','GRADLE_OPTS'] if key in os.environ}}
def fixture_graph(legacy):
 manifest=json.loads((legacy/'runtime/original-fixture-manifest.json').read_text())
 files=[e for e in manifest['helper_files'] if not e['path'].endswith('/LocalLogsContractTest.kt')]
 for entry in files:assert sha(legacy/'runtime/fixture-graph'/entry['path'])==entry['sha256'],entry['path']
 return manifest,files
def verify_probe(probe,prepared,source):
 for entry in prepared['fixture_files']:assert sha(probe/entry['path'])==entry['sha256'],entry['path']
 for entry in prepared['current_player_components']:
  assert sha(source/entry['path'])==entry['sha256']
  relative=Path(entry['path']).relative_to('android/app/src/main/java');assert sha(probe/'probe/src/player'/relative)==entry['sha256']
 assert sha(probe/'probe/src/androidTest/java/space/nuvio/preloadtest/StartupContractTest.kt')==sha(probe/'startup-diagnostics/StartupContractTest.kt')
def main():
 p=argparse.ArgumentParser();p.add_argument('--phase',choices=['helpers','prepare-media3','media3','verify'],required=True)
 for key in ['source','source-proof','output']:p.add_argument('--'+key,type=Path,required=True)
 p.add_argument('--tools',type=Path,default=Path.home()/'.cache/nuvio-kotlin-preflight');a=p.parse_args()
 plan,_=load_plan(HERE.parent/'plan.json');source=a.source.resolve();proof=json.loads(a.source_proof.read_text());verify_source(plan,proof,'test',source);verify_inherited(plan,HERE.parents[2])
 legacy=HERE.parents[1]/'logs8';out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
 identity=java_identity();write(out/'java-identity.json',identity)
 binding={key:proof[key] for key in ['source_commit','tree','review_commit','manifest_sha256','bundle_sha256']}
 binding.update(plan_sha256=sha(HERE.parent/'plan.json'),source_proof_sha256=sha(a.source_proof))
 manifest,files=fixture_graph(legacy);selection=json.loads((legacy/'runtime/player-reuse-plan.json').read_text())['required_test_classes'];assert sum(map(len,selection.values()))==15
 if a.phase=='helpers':
  target=out/'kotlin';assert not target.exists(),'Use fresh helper outputs'
  invocation=invoke(['python3',str(legacy/'kotlin/run.py'),'--source',str(source),'--source-proof',str(a.source_proof.resolve()),'--output',str(target),'--tools',str(a.tools.resolve())],out/'helpers-run.log')
  result=json.loads((target/'kotlin-verified.json').read_text());assert result['outcome']=='passed' and result['passed_contracts']==32 and [r['passed_contracts'] for r in result['records']]==[0,26,6]
  assert result['source_binding']['source_commit']==proof['source_commit'] and result['source_binding']['review_commit']==proof['review_commit']
  assert result['tool_hashes']=={e['name']:e['sha256'] for e in json.loads((legacy/'kotlin/tools.json').read_text())}
  write(out/'helpers-verified.json',{'outcome':'passed','fresh_executed_contracts':32,'source_binding':binding,'java_identity':identity,'invocation':invocation,'actual_receipt_sha256':sha(target/'kotlin-verified.json'),'runner_sha256':sha(legacy/'kotlin/run.py'),'scope':'Fresh isolated JVM helper contracts with original Android boundary doubles; no APK/device claim'})
 elif a.phase=='prepare-media3':
  probe=out/'media3-probe';assert not probe.exists(),'Use a fresh isolated probe'
  for entry in files:
   original=legacy/'runtime/fixture-graph'/entry['path'];target=probe/entry['path'];target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(original,target)
  # Keep all original assertions. Only the obsolete five log contracts/helper
  # are absent; fresh helper32 covers the current implementations separately.
  assertions=probe/'probe/src/androidTest/java/space/nuvio/preloadtest'
  shutil.copy2(probe/'startup-diagnostics/StartupContractTest.kt',assertions/'StartupContractTest.kt')
  components=[e for e in manifest['staged_player_helpers'] if not e['path'].endswith('/TestLocalLogs.kt')];assert len(components)==10
  current=[]
  for entry in components:
   original=source/'android/app/src/main/java'/entry['path'];target=probe/'probe/src/player'/entry['path'];target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(original,target)
   current.append({'path':str(original.relative_to(source)),'sha256':sha(original)})
  assert sorted(p.name for p in assertions.glob('*.kt'))==['PreloadContractTest.kt','StartupContractTest.kt']
  invocations=[invoke(['bash',str(probe/'generate-fixture.sh')],out/'generate-media.log'),invoke(['gradle','-p',str(probe),'--console=plain',':probe:assembleDebug',':probe:assembleDebugAndroidTest'],out/'probe-build.log')]
  invocations.append(invoke(['gradle','-p',str(probe),'--console=plain','-I',str(HERE/'dependencies.init.gradle'),'-PreviewDependencyOutput='+str(out/'resolved-dependencies.json'),':probe:apk20020DependencyEvidence'],out/'dependency-graph.log'))
  dependencies=json.loads((out/'resolved-dependencies.json').read_text())['artifacts'];assert dependencies
  media={item['component'] for item in dependencies if item['component'].startswith('androidx.media3:')}
  assert media and all(item.endswith(':1.8.0') for item in media),'Unexpected actual resolved Media3 version'
  invocations.append(invoke(['gradle','--version'],out/'gradle-version.log'))
  assert 'Gradle 8.9' in (out/'gradle-version.log').read_text()
  invocations.append(invoke(['ffmpeg','-version'],out/'ffmpeg-version.log'))
  tools={}
  sdk=Path(os.environ['ANDROID_HOME'])
  for name,path in [('adb',sdk/'platform-tools/adb'),('emulator',sdk/'emulator/emulator'),('clang',sdk/'ndk/27.2.12479018/toolchains/llvm/prebuilt/linux-x86_64/bin/clang'),('gradle',Path(shutil.which('gradle')).resolve())]:
   assert path.is_file(),name;tools[name]={'path':str(path.resolve()),'sha256':sha(path.resolve())}
  gradle_home=Path(tools['gradle']['path']).parents[1]
  write(out/'media3-prepared.json',{'source_binding':binding,'java_identity':identity,'fixture_manifest_sha256':sha(legacy/'runtime/original-fixture-manifest.json'),'fixture_files':files,'current_player_components':current,'selected_classes':selection,'excluded_obsolete_log_methods':5,'assertions_modified':False,'invocations':invocations,'tool_files':tools,'gradle_distribution_jars':{str(p.relative_to(gradle_home)):sha(p) for p in sorted((gradle_home/'lib').rglob('*.jar'))},'resolved_dependencies_sha256':sha(out/'resolved-dependencies.json'),'probe_apks':{str(p.relative_to(out)):sha(p) for p in sorted((probe/'probe/build/outputs/apk').rglob('*.apk'))},'generated_assets':{str(p.relative_to(probe)):sha(p) for p in sorted((probe/'probe/src/androidTest/assets').rglob('*')) if p.is_file()}})
 elif a.phase=='media3':
  probe=out/'media3-probe';prepared=json.loads((out/'media3-prepared.json').read_text());assert prepared['source_binding']==binding
  verify_probe(probe,prepared,source)
  reports=out/'media3-runtime';assert not reports.exists(),'Use fresh emulator reports'
  invocation=invoke(['bash',str(probe/'startup-diagnostics/run-runtime.sh'),str(probe),str(reports),'all'],out/'media3-run.log')
  xmls=sorted((probe/'probe/build/outputs/androidTest-results').rglob('TEST-*.xml'));assert xmls,'No fresh instrumentation XML'
  seen={}
  for xml in xmls:
   for case in ET.fromstring(xml.read_text()).iter('testcase'):
    klass=case.attrib['classname'];name=case.attrib['name'].split('[')[0];assert klass in selection and name in selection[klass]
    assert not any(case.find(kind) is not None for kind in ['failure','error','skipped'])
    assert name not in seen.setdefault(klass,set()),'Duplicate instrumentation testcase';seen[klass].add(name)
  assert {key:sorted(value) for key,value in seen.items()}=={key:sorted(value) for key,value in selection.items()}
  assert json.loads((reports/'metrics-collection.json').read_text())['complete']
  verify_probe(probe,prepared,source)
  emulator={}
  for key in ['ro.build.fingerprint','ro.build.version.sdk','ro.product.cpu.abi']:
   result=subprocess.run(['adb','shell','getprop',key],text=True,capture_output=True);assert result.returncode==0;emulator[key]=result.stdout.strip()
  assert emulator['ro.build.version.sdk']=='35' and emulator['ro.product.cpu.abi']=='x86_64'
  image=Path(os.environ['ANDROID_HOME'])/'system-images/android-35/google_apis/x86_64/source.properties';assert image.is_file();emulator['system_image_source_properties_sha256']=sha(image)
  write(out/'media3-verified.json',{'outcome':'passed','fresh_executed_contracts':15,'source_binding':binding,'java_identity':identity,'prepared_receipt_sha256':sha(out/'media3-prepared.json'),'invocation':invocation,'test_xml_sha256':{str(p.relative_to(out)):sha(p) for p in xmls},'selected_test_classes':selection,'emulator':emulator,'assertions_modified':False,'scope':'Fresh15 isolated Media3 playback contracts in API35 x86_64 emulator; excludes5obsolete log contracts; no whole-app/physical-device or native-player timing claim'})
 else:
  helper=json.loads((out/'helpers-verified.json').read_text());media=json.loads((out/'media3-verified.json').read_text())
  for result in [helper,media]:assert result['outcome']=='passed' and result['source_binding']==binding and result['java_identity']==identity
  write(out/'jvm47-verified.json',{'outcome':'passed','source_binding':binding,'fresh_contracts':47,'helper_contracts':32,'media3_contracts':15,'historical_execution_reused':False,'java_identity':identity,'helper_receipt_sha256':sha(out/'helpers-verified.json'),'media3_receipt_sha256':sha(out/'media3-verified.json'),'scope':'Fresh32 JVM helper plus15 isolated real-Media3 emulator contracts; no new full-app/native/physical-device timing claim'})
 verify_source(plan,proof,'test',source)
 print('PASS fresh JVM stage',a.phase)
if __name__=='__main__':main()
