from pathlib import Path
import argparse,subprocess,json,hashlib,os,platform
p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--sanitize',action='store_true');p.add_argument('--profile-expected-rejection',action='store_true');a=p.parse_args()
s=a.source.resolve();o=a.output.resolve();o.mkdir(parents=True,exist_ok=False);r=Path(__file__).resolve().parent
flags=['-O1','-g','-ffunction-sections','-fdata-sections','-I'+str(s/'src'),'-I'+str(s/'tests'),'-I'+str(r/'tests'),'-Wno-macro-redefined','-Wno-deprecated-declarations']
darwin=platform.system()=='Darwin'
if darwin:
 sdk=Path(subprocess.check_output(['brew','--prefix'],text=True).strip());flags+=['-I'+str(sdk/'include'),'-I'+str(sdk/'include/SDL2')];linkflag='-Wl,-dead_strip'
else:
 flags+=['-DNV_TIZEN','-I/workspace/review/runtime-syntax-sdk','-include',str(r/'tests/sdk-extra.h')];linkflag='-Wl,--gc-sections'
if a.sanitize:flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
records=[]
for name,fixture,ui in [('profile-session','profile-late-page.c',False),('all-eight-tv','all-eight-watched.c',False),('all-eight-touch','all-eight-watched.c',True)]:
 command=['cc',*flags,*(['-DNV_TOUCH_UI=1']if ui else[]),str(r/'tests'/fixture),str(s/'src/catalogo.c'),str(s/'src/cwordem.c'),str(s/'src/js.c'),str(s/'src/metaprov.c'),*([str(s/'src/descoberta.c')]if name!='profile-session'else[]),'-lm','-lpthread',linkflag,'-o',str(o/name)]
 z=subprocess.run(command,capture_output=True,text=True,timeout=120);(o/(name+'-compile.log')).write_text(z.stdout+z.stderr);assert z.returncode==0,z.stderr[-3000:]
 env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
 z=subprocess.run([str(o/name)],capture_output=True,text=True,timeout=20,env=env);(o/(name+'.log')).write_text(z.stdout+z.stderr)
 expectedFailure=name=='profile-session' and a.profile_expected_rejection
 assert (z.returncode!=0)if expectedFailure else(z.returncode==0),(name,z.returncode,z.stdout[-1500:],z.stderr[-1500:])
 if expectedFailure:assert 'vtN == 0' in z.stderr,z.stderr
 records.append({'name':name,'expected_baseline_failure':expectedFailure,'returncode':z.returncode,'passed_lines':z.stdout.count('PASS '),'command':command,'log_sha256':hashlib.sha256((z.stdout+z.stderr).encode()).hexdigest()})
 print(name,'expected upstream guard absent'if expectedFailure else'passed',flush=True)
(o/'verified.json').write_text(json.dumps({'source':str(s),'source_commit':subprocess.check_output(['git','-C',str(s),'rev-parse','HEAD'],text=True).strip(),'sanitized':a.sanitize,'sdk_mode':'real macOS/SDL SDK'if darwin else'local typed CPU boundaries','records':records,'scope':'Actual discovery request-session guard with authoritative synthetic epoch; actual raw catalog projection/layout focus across all8 display choices, TV and Mobile. No real profile module, GPU/device or network claim.'},indent=2)+'\n')
