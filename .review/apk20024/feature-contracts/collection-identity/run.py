from pathlib import Path
import argparse,hashlib,json,os,platform,re,subprocess,sys
r=Path(__file__).resolve().parent
p=argparse.ArgumentParser()
for n in ['source','output']:p.add_argument('--'+n,type=Path,required=True)
p.add_argument('--cc',required=True);p.add_argument('--sanitize',action='store_true');p.add_argument('--private',action='store_true');a=p.parse_args()
assert platform.system()=='Linux'
s=a.source.resolve();o=a.output.resolve();assert not o.exists();o.mkdir(parents=True)
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
git=lambda *v:subprocess.check_output(['git','-C',str(s),*v],text=True).strip()
head,tree=git('rev-parse','HEAD'),git('rev-parse','HEAD^{tree}');assert not git('status','--porcelain')
env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1:print_stacktrace=1'}
flags=['-std=gnu11','-O1','-g1','-fno-inline','-fno-pie','-ffunction-sections','-fdata-sections','-I'+str(s/'src'),'-I'+str(r/'sdk'),'-pthread','-DDISCOVERY_SOURCE='+json.dumps(str(s/'src/descoberta.c'))]
if a.sanitize:flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-sanitize-recover=all']
if a.private:flags+=['-DNV_TEST_LOCAL_LOGS=1']
commands=[];dependencies={};runs=[]
def run(cmd,label):
 commands.append(cmd);z=subprocess.run(cmd,text=True,capture_output=True,env=env,timeout=600)
 (o/(label+'.log')).write_text(z.stdout+z.stderr)
 assert not z.returncode,(label,z.stdout,z.stderr)
 assert not any(x in z.stderr for x in ['ERROR: AddressSanitizer','runtime error:','UNEXPECTED dependency:'])
 return z
for name in ['lifecycle','worker']:
 objects=[];inputs=[s/'src/addons.c',s/'src/js.c',s/'src/perfis.c',r/('identity_'+name+'.c')]
 if name=='worker':inputs += [s/'src/cwordem.c',r/'identity_worker_trap.c']
 for i,f in enumerate(inputs):
  obj=o/(name+'-'+str(i)+'.o');dep=obj.with_suffix('.d')
  run([a.cc,*flags,'-MMD','-MF',str(dep),'-c',str(f),'-o',str(obj)],name+'-compile-'+str(i))
  for path in dep.read_text().replace('\\\n',' ').split(':',1)[1].split():dependencies[str(Path(path).resolve())]=sha(path)
  objects.append(str(obj))
 binary=o/name
 run([a.cc,*objects,*flags,'-Wl,--gc-sections','-no-pie','-o',str(binary)],name+'-link')
 z=run([str(binary)],name+'-run');assert 'PASS:' in z.stdout
 runs.append({'name':name,'returncode':0,'log_sha256':sha(o/(name+'-run.log')),'binary_sha256':sha(binary)})
cmd=[sys.executable,str(r/'ui/run.py'),'--source',str(s),'--output',str(o/'ui'),'--cc',a.cc]+(['--sanitize']if a.sanitize else[])+(['--private']if a.private else[])
run(cmd,'ui-producer')
up=o/'ui/receipt.json';ui=json.loads(up.read_text())
expected={'late-y','late-x','late-xy','active-y','active-x','regroup-xy','regroup-active-x','reset-active','stale-load-before-redraw','stale-load-after-redraw','stale-reorder-before-redraw','stale-reorder-after-redraw','tv-follow','late-addon-recovery','late-addon-inflight','identity-status'}
assert ui['exit_code']==0 and ui['sanitized'] is a.sanitize and ui['private'] is a.private
assert len(ui['runs'])==16 and {x['case'].split('-r0-',1)[1] for x in ui['runs']}==expected
assert all(x['exit_code']==0 and x['abort_traps_fired'] is False for x in ui['runs']) and ui['abort_traps_fired'] is False
assert ui['source_commit']==head
dependencies.update(ui['dependency_sha256'])
for f,h in dependencies.items():assert sha(f)==h,(f,'source changed')
assert not git('status','--porcelain') and git('rev-parse','HEAD')==head
sources={str(Path(f).relative_to(s)):h for f,h in dependencies.items() if Path(f).is_relative_to(s)}
assert {'src/addons.c','src/js.c','src/perfis.c','src/descoberta.c','src/vertudo.c'} <= set(sources)
receipt={'passed':True,'source_commit':head,'source_tree':tree,'private':a.private,'sanitizers':['address','undefined']if a.sanitize else[],'groups':['lifecycle','worker','renderer'],'runs':runs,'renderer_cases':16,'renderer_case_names':sorted(expected),'raw_renderer_receipt_relative':'ui/receipt.json','raw_renderer_receipt_sha256':sha(up),'source_inputs_sha256':sources,'dependency_sha256':dependencies,'logs_sha256':{str(q.relative_to(o)):sha(q)for q in o.rglob('*.log')},'commands':commands,'scope':'Actual addon/profile lifecycle and full discovery worker/cache publication before CW/capabilities, owned base and scoped final capability guards; all16 real collection render/pointer/recovery cases. Typed offline SDK/render/transport boundaries, no app/device/network performance claim.'}
(o/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print('PASS collection identity lifecycle+worker+16 renderer cases')
