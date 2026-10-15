"""Portable producer for the unchanged accepted #362 eight-choice UI fixture."""
from pathlib import Path
import argparse,hashlib,json,os,platform,re,shlex,shutil,subprocess
p=argparse.ArgumentParser();p.add_argument('--source',required=True);p.add_argument('--output',required=True);p.add_argument('--cc',default='cc');p.add_argument('--platform',choices=['all','tv','mobile'],default='all');p.add_argument('--sanitize',action='store_true');p.add_argument('--private',action='store_true');a=p.parse_args()
R=Path(__file__).resolve().parent;S=Path(a.source).resolve();O=Path(a.output).resolve();O.mkdir(parents=True,exist_ok=False);darwin=platform.system()=='Darwin';cc=shutil.which(a.cc);assert cc,'Compiler missing';cc=str(Path(cc).resolve());sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
origin=json.loads((R/'fixture-provenance.json').read_text());assert sha(R/'fixtures/collection-choice.c')==origin['fixture']['sha256'],'Accepted assertions changed'
adaptation=json.loads((R/'tv-assertion-adaptation.json').read_text());assert sha(R/'fixtures/collection-choice-composed.c')==adaptation['composed_fixture_sha256'],'Approved TV input adaptation changed'
commit=subprocess.check_output(['git','rev-parse','HEAD'],cwd=S,text=True).strip();tree=subprocess.check_output(['git','rev-parse','HEAD^{tree}'],cwd=S,text=True).strip();assert subprocess.run(['git','diff','--quiet','HEAD','--','src'],cwd=S).returncode==0,'Tracked product C source changed'
paths=[S/x for x in subprocess.check_output(['git','ls-files','src'],cwd=S,text=True).splitlines()]+[x for x in R.rglob('*')if x.is_file()];initial={str(x.resolve()):sha(x)for x in paths if x.is_file()}
# Preserve the accepted touch-capable Android compilation; tv means saved TV
# runtime mode, not a new native-platform variant of these UI assertions.
flags=['-std=gnu11','-O1','-g','-ffunction-sections','-fdata-sections','-DNV_ANDROID=1','-DNV_TOUCH_UI=1','-I'+str(R/'sdk'),'-I'+str(S/'src'),'-Wno-deprecated-declarations','-DSETTINGS_SOURCE='+json.dumps(str(S/'src/ajustes.c'))]
if darwin:flags+=['-DNV_GLES_NO_MAC=1']
if a.private:flags+=['-DNV_TEST_LOCAL_LOGS=1']
if a.sanitize:flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-sanitize-recover=all']+([]if darwin else['-fno-pie'])
modules=['layout','ponteiro','fileiras','dados','linguas','fontefalha','fonteregra','regexjs','js']
# Combined-source source filters require their authentic producer at link time;
# the accepted fixture/assertions remain byte-identical.
if (S/'src/fontefiltro.c').is_file():modules+=['fontefiltro']
files=[R/'fixtures/collection-choice-composed.c',R/'offline-account.c']+[S/'src'/(x+'.c')for x in modules];objects=[];commands=[];closure={}
for i,f in enumerate(files):
 obj=O/(str(i)+'.o');dep=O/(str(i)+'.d');cmd=[cc,*flags,'-MMD','-MF',str(dep),'-c',str(f),'-o',str(obj)];z=subprocess.run(cmd,capture_output=True,text=True,timeout=180);(O/(str(i)+'.compile.log')).write_text(z.stdout+z.stderr);assert z.returncode==0,z.stderr;objects.append(str(obj));commands.append(cmd)
 for raw in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
  q=Path(raw).resolve();digest=sha(q);assert str(q)in initial and initial[str(q)]==digest,'Dependency changed during compile: '+str(q);closure[str(q)]=digest
link=[cc,*objects,'-Wl,-dead_strip'if darwin else'-Wl,--gc-sections','-pthread','-lm','-o',str(O/'probe')]+(['-fsanitize=address,undefined']if a.sanitize else[])+(['-no-pie']if a.sanitize and not darwin else[]);z=subprocess.run(link,capture_output=True,text=True);names=sorted(set(re.findall(r"undefined reference to [`']([^'`]+)['`]",z.stderr)+re.findall(r'"(_[^"\n]+)", referenced from:',z.stderr)))
if z.returncode and not names:raise AssertionError(z.stderr)
text='#include <stdio.h>\n#include <stdlib.h>\n'
for i,n in enumerate(names):text+=f'void never_{i}(void) __asm__({json.dumps(n)});void never_{i}(void){{fputs("UNEXPECTED dependency: {n}\\n",stderr);abort();}}\n'
(O/'unreached.c').write_text(text);z=subprocess.run([cc,*flags,'-c',str(O/'unreached.c'),'-o',str(O/'unreached.o')],capture_output=True,text=True);assert z.returncode==0,z.stderr;link.insert(1,str(O/'unreached.o'));z=subprocess.run(link,capture_output=True,text=True);(O/'link.log').write_text(z.stdout+z.stderr);assert z.returncode==0,z.stderr;commands.append(link);cases=[]
for mobile in([True,False]if a.platform=='all'else[a.platform=='mobile']):
 for portrait in([1,0]if mobile else[0]):
  for size in[0,1,2]:
   for ui in[1,1.2,1.3,1.5]:
    args=[str(portrait),str(size),str(ui),'1'if mobile else'0'];z=subprocess.run([str(O/'probe'),*args],capture_output=True,text=True,timeout=90,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1:print_stacktrace=1'});log=O/('-'.join(args)+'.run.log');log.write_text(z.stdout+z.stderr);assert z.returncode==0,z.stdout+z.stderr;assert 'UNEXPECTED dependency:'not in z.stderr;assert all(sha(q)==h for q,h in closure.items()),'Compiled dependency changed';cases.append({'args':args,'returncode':0,'stdout':z.stdout,'stderr':z.stderr,'log_sha256':sha(log)});print(args,z.stdout.splitlines()[-1],flush=True)
logs={str(x.relative_to(O)):sha(x)for x in O.rglob('*.log')};recipe={str(x.relative_to(R)):sha(x)for x in R.rglob('*')if x.is_file()}
receipt={'passed':True,'source_commit':commit,'source_tree':tree,'source_pin':None,'platform':a.platform,'private':a.private,'sanitizers':['address','undefined']if a.sanitize else[],'compile_platform':'NV_ANDROID+NV_TOUCH_UI; tv selects savedTV runtime mode','compiler':cc,'compiler_version':subprocess.check_output([cc,'--version'],text=True).splitlines()[0],'fixture_provenance_sha256':sha(R/'fixture-provenance.json'),'tv_assertion_adaptation_sha256':sha(R/'tv-assertion-adaptation.json'),'recipe_files_sha256':recipe,'dependency_sha256':closure,'commands':commands,'unreached_abort_boundaries':names,'binary_sha256':sha(O/'probe'),'cases':cases,'logs_sha256':logs,'scope':'Unchanged accepted global8choice Settings assertions and actual full Settings/layout/pointer/dados; fixed offline profile1, typed text/GFX/account/platform companions. No profile/sync/physical-device/GPU claim.'};(O/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
