#!/usr/bin/env python3
from pathlib import Path
import argparse,subprocess,re,json,hashlib,os,sys
p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--sanitize',action='store_true');p.add_argument('--private',action='store_true');p.add_argument('--cc',default='cc');p.add_argument('--matrix',action='store_true');p.add_argument('--extended',action='store_true');a=p.parse_args()
s=a.source.resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False);here=Path(__file__).resolve().parent
sdk=here
flags=['-std=gnu11','-O1','-g','-ffunction-sections','-fdata-sections','-fno-pie','-I'+str(sdk/'sdk'),'-I'+str(s/'src'),'-include',str(sdk/'sdk-ticks-compat.h'),'-DNV_ANDROID=1','-DNV_TOUCH_UI=1','-DVERTUDO_SOURCE="'+str(s/'src/vertudo.c')+'"']
if 'static void vitProjetar(' in (s/'src/vertudo.c').read_text():flags+=['-DHAS_VIT_PROJECTION=1']
if a.private:flags+=['-DNV_TEST_LOCAL_LOGS=1']
darwin=sys.platform=='darwin'
if darwin:
 flags=[v for v in flags if v not in ['-ffunction-sections','-fdata-sections','-fno-pie']]+['-D_DARWIN_C_SOURCE=1']
linkflags=['-Wl,-dead_strip']if darwin else['-Wl,--gc-sections','-no-pie']
san=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-sanitize-recover=all']if a.sanitize else[]
env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1:print_stacktrace=1'}
cmds=[];hashes={};sha=lambda f:hashlib.sha256(Path(f).read_bytes()).hexdigest()
source_start_sha=sha(s/'src/vertudo.c')
def run(cmd,log):
 cmds.append(cmd);r=subprocess.run(cmd,capture_output=True,text=True,env=env);log.write_text(r.stdout+r.stderr);return r
discovery=(s/'src/descoberta.c').read_text()
assert 'int desc_filme_visto(' in discovery, 'Require real upstream watched-state predicate'
producer=out/'actual-desc-filme-visto.c'
start=discovery.index('int desc_filme_visto(');end=discovery.index('\n}',start)+2
producer.write_text('#include \"catalogo.h\"\n#include <string.h>\n'+discovery[start:end]+'\n')
objects=[]
for i,path in enumerate([here/'collection-taps.c']+[s/'src'/(m+'.c')for m in ['dados','layout','ponteiro','ctxlista','perfis']]+[producer]):
 obj=out/(str(i)+'.o');dep=out/(str(i)+'.d');extra=['-Ddados_gravar=collection_real_dados_gravar']if path.name=='dados.c'else[]
 r=run([a.cc,*flags,*san,*extra,'-MMD','-MF',str(dep),'-c',str(path),'-o',str(obj)],out/(str(i)+'-compile.log'))
 if r.returncode:print(r.stdout+r.stderr);raise SystemExit(r.returncode)
 for f in dep.read_text().replace('\\\n',' ').split(':',1)[1].split():hashes[str(Path(f).resolve())]=sha(f)
 objects.append(str(obj))
link=[a.cc,*objects,*linkflags,*san,'-lm','-pthread','-o',str(out/'probe')]
r=run(link,out/'initial-link.log');names=sorted(set(re.findall(r'^\s*"([^\"]+)", referenced from:\s*$'if darwin else r"undefined reference to [`']([^'`]+)['`]",r.stderr,re.M)))
assert not r.returncode or names,r.stderr
traps='#include <stdio.h>\n#include <stdlib.h>\n'
for i,name in enumerate(names):
 assert re.fullmatch(r'[A-Za-z_][A-Za-z_0-9]*',name),name
 traps+=f'void never_{i}(void) __asm__({json.dumps(name)});\nvoid never_{i}(void){{fputs("UNEXPECTED dependency: {name}\\n",stderr);abort();}}\n'
(out/'traps.c').write_text(traps);r=run([a.cc,*flags,*san,'-c',str(out/'traps.c'),'-o',str(out/'traps.o')],out/'traps-compile.log');assert not r.returncode,r.stderr
link.insert(1,str(out/'traps.o'));r=run(link,out/'link.log');assert not r.returncode,r.stderr
results=[]
if a.extended or not a.matrix:
 cases=[(v,o,z,0,'open')for v in range(8)for o in range(2)for z in range(3)]
 cases += [(7,o,z,1,'open')for o in range(2)for z in range(3)]
 cases += [(7,0,0,0,c)for c in ['stale-filter','stale-reopen','stale-source','stale-session','stale-mode','stale-closed','stale-layout','stale-key','drag-x','drag-y','drag-diag-x','drag-diag-y','resume-x','resume-y','transition-x','transition-y','clip-top','multi','hold','tmdb']]
 cases += [(5,0,0,0,'decades-clip')]
else:cases=[(v,0,0,0,'open')for v in (range(8)if a.matrix else[7])]
for view,orientation,zoom,row,test in cases:
 label=f'v{view}-o{orientation}-z{zoom}-r{row}-{test}'
 r=run([str(out/'probe'),str(view),str(orientation),str(zoom),str(row),test],out/('run-'+label+'.log'))
 if r.returncode:print(label,r.stdout+r.stderr)
 else:assert 'PASS 'in r.stdout,(label,'missing fixture pass marker')
 results.append({'case':label,'exit_code':r.returncode,'abort_traps_fired':'UNEXPECTED dependency:'in r.stderr})
print(f'{sum(x["exit_code"]==0 for x in results)}/{len(results)} production render/pointer cases passed')
assert sha(s/'src/vertudo.c')==source_start_sha,'Collection source changed while probe ran'
for f,h in hashes.items():assert sha(f)==h,(f,'dependency changed while probe ran')
receipt={'scope':'Actual full vertudo render and production pointer/event/ctxhold/abrirItem/layout/local-storage/profile epoch paths; typed headless SDL/GPU/text/catalog/TMDB boundaries. Grid cases prove one visible noninitial card per view, not complete grid reachability. No app/SDK/device/network build claim.','source':str(s),'source_commit':subprocess.check_output(['git','-C',str(s),'rev-parse','HEAD'],text=True).strip(),'actual_watched_producer_origin':{'source_sha256':sha(s/'src/descoberta.c'),'actual_function_present':True,'generated_sha256':sha(producer)},'source_status':subprocess.check_output(['git','-C',str(s),'status','--porcelain'],text=True),'runs':results,'exit_code':max(x['exit_code']!=0 for x in results),'private':a.private,'sanitized':a.sanitize,'commands':cmds,'dependency_sha256':hashes,'unreached_abort_traps':names,'abort_traps_fired':any(x['abort_traps_fired']for x in results),'binary_sha256':sha(out/'probe'),'tooling_sha256':{str(f.relative_to(here)):sha(f)for f in [here/'collection-taps.c',Path(__file__).resolve(),here/'sdk-ticks-compat.h',*sorted((here/'sdk').rglob('*'))]if f.is_file()}}
(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
raise SystemExit(1 if any(x['exit_code']for x in results)else 0)
