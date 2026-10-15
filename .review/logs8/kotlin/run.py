#!/usr/bin/env python3
"""Run32actual integrated Kotlin contracts with hash-pinned compiler tools and Android boundaries."""
from pathlib import Path
import argparse,hashlib,json,os,shutil,subprocess,urllib.request
p=argparse.ArgumentParser();p.add_argument('--source',required=True,type=Path);p.add_argument('--output',required=True,type=Path);p.add_argument('--tools',required=True,type=Path);p.add_argument('--source-proof',type=Path);a=p.parse_args();here=Path(__file__).resolve().parent;out=a.output.resolve();out.mkdir(parents=True,exist_ok=True);tools=a.tools.resolve();tools.mkdir(parents=True,exist_ok=True);sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
tool_plan=json.loads((here/'tools.json').read_text())
for e in tool_plan:
 target=tools/e['name']
 if target.is_file() and sha(target)==e['sha256']:continue
 assert e['url'].startswith('https://repo.maven.apache.org/maven2/');tmp=target.with_suffix('.part');urllib.request.urlretrieve(e['url'],tmp);assert sha(tmp)==e['sha256'],e['name'];tmp.replace(target)
java=str(Path(os.environ['JAVA_HOME'])/'bin/java') if os.environ.get('JAVA_HOME') else shutil.which('java');assert java
src=a.source.resolve()/'android/app/src/main/java/space/nuvio/nativelegacy';sources=[src/'RegistroDownload.kt',src/'TestLocalLogs.kt'];fixtures=[here/'Preflight.kt',here/'LongSecretPreflight.kt',here/'PrivateSnapshotPreflight.kt'];stubs=sorted((here/'stubs').glob('*.kt'));classes=out/'classes';classes.mkdir(exist_ok=True);data=out/'runtime-data';data.mkdir(exist_ok=True);jars=[tools/e['name'] for e in tool_plan]
compiler=[java,'-cp',':'.join(map(str,jars)),'org.jetbrains.kotlin.cli.jvm.K2JVMCompiler','-no-stdlib','-no-reflect','-jvm-target','17','-classpath',':'.join(str(tools/n) for n in ['kotlin-stdlib-1.9.24.jar','annotations-13.0.jar']),'-d',str(classes),*map(str,stubs+sources+fixtures)]
bin=out/'fakebin';bin.mkdir(exist_ok=True);fake=bin/'logcat';fake.write_text('#!/bin/sh\nprintf "%s\\n" "$*" > "$LOGCAT_ARGS"\nprintf "x" >> "$LOGCAT_COUNT"\nprintf "APP_LOGCAT_LINE token=logcat-secret\\n"\n');fake.chmod(0o755)
env=dict(os.environ,PATH=str(bin)+':'+os.environ['PATH'],LOGCAT_ARGS=str(data/'logcat-args.txt'),LOGCAT_COUNT=str(data/'logcat-count.txt'));records=[];commands=[('compile',compiler)]
for label,main in [('public-opt-in-false','PreflightKt'),('private-opt-in-true','PrivateSnapshotPreflightKt')]:commands.append((label,[java,'-Dpreflight.data='+str(data),'-cp',str(classes)+':'+str(tools/'kotlin-stdlib-1.9.24.jar'),main]))
for label,cmd in commands:
 z=subprocess.run(cmd,capture_output=True,text=True,env=env);text=z.stdout+z.stderr;(out/(label+'.log')).write_text(text);assert z.returncode==0,text;records.append({'gate':label,'command':cmd,'passed_contracts':sum(x.startswith('PASS ') for x in text.splitlines()),'log_sha256':sha(out/(label+'.log'))});print('PASS',label,records[-1]['passed_contracts'],flush=True)
assert [x['passed_contracts'] for x in records]==[0,26,6]
binding={'source_commit':subprocess.check_output(['git','-C',str(a.source),'rev-parse','HEAD'],text=True).strip(),'tree':subprocess.check_output(['git','-C',str(a.source),'rev-parse','HEAD^{tree}'],text=True).strip(),'review_commit':os.environ.get('GITHUB_SHA','local-review')}
if a.source_proof:
 proof=json.loads(a.source_proof.read_text());assert proof['verified'] and proof['layer']=='test' and proof['source_commit']==binding['source_commit'] and proof['tree']==binding['tree'];binding['source_proof_sha256']=sha(a.source_proof);binding['manifest_sha256']=proof['manifest_sha256']
receipt={'source_binding':binding,'outcome':'passed','scope':'Actual integrated helper+utility executed on JVM with minimal Android boundary doubles; no device/emulator claim','sources':{str(x.relative_to(a.source.resolve())):sha(x) for x in sources},'fixtures':{str(x.relative_to(here)):sha(x) for x in stubs+fixtures},'tool_hashes':{x.name:sha(x) for x in jars},'records':records,'passed_contracts':32};(out/'kotlin-verified.json').write_text(json.dumps(receipt,indent=2)+'\n')
