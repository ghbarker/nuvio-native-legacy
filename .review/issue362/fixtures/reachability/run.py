#!/usr/bin/env python3
from pathlib import Path
import argparse,subprocess,re,json,hashlib,os,sys
p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--sanitize',action='store_true');p.add_argument('--cc',default='cc');p.add_argument('--private',action='store_true');p.add_argument('--matrix',action='store_true',default=True);p.add_argument('--view',type=int,default=3);p.add_argument('--zoom',type=int,default=2);p.add_argument('--orientation',type=int,default=0);p.add_argument('--tv',action='store_true');p.add_argument('--all-configurations',action='store_true',default=True);a=p.parse_args()
s=a.source.resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False);here=Path(__file__).resolve().parent
sdk=here
flags=['-std=gnu11','-O1','-g','-ffunction-sections','-fdata-sections','-fno-pie','-I'+str(sdk/'sdk'),'-I'+str(s/'src'),'-include',str(sdk/'fixtures/sdk-ticks-compat.h'),'-DNV_ANDROID=1','-DNV_TOUCH_UI=1','-DVERTUDO_SOURCE="'+str(s/'src/vertudo.c')+'"']
if a.private:flags+=['-DNV_TEST_LOCAL_LOGS=1']
if 'static void vitProjetar(' in (s/'src/vertudo.c').read_text():flags+=['-DHAS_VIT_PROJECTION=1']
darwin=sys.platform=='darwin'
if darwin:
 flags=[v for v in flags if v not in ['-ffunction-sections','-fdata-sections','-fno-pie']]+['-D_DARWIN_C_SOURCE=1']
linkflags=['-Wl,-dead_strip']if darwin else['-Wl,--gc-sections','-no-pie']
san=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-sanitize-recover=all']if a.sanitize else[]
env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1:print_stacktrace=1'}
commit=subprocess.check_output(['git','-C',str(s),'rev-parse','HEAD'],text=True).strip();tree=subprocess.check_output(['git','-C',str(s),'rev-parse','HEAD^{tree}'],text=True).strip();assert not subprocess.check_output(['git','-C',str(s),'status','--porcelain'],text=True),'Use a frozen clean source'
cmds=[];hashes={};sha=lambda f:hashlib.sha256(Path(f).read_bytes()).hexdigest()
def run(cmd,log):
 cmds.append(cmd);r=subprocess.run(cmd,capture_output=True,text=True,env=env);log.write_text(r.stdout+r.stderr);return r
discovery=(s/'src/descoberta.c').read_text()
assert 'int desc_filme_visto(' not in discovery, 'This adapter is bound to the raw upstream collection model'
objects=[]
for i,path in enumerate([here/'fixtures/geometry.c']+[s/'src'/(m+'.c')for m in ['dados','layout','ponteiro','ctxlista','perfis']]):
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
import itertools
for view,orientation,zoom in itertools.product(range(1,7) if a.matrix else [a.view],range(2) if a.all_configurations else [a.orientation],range(3) if a.all_configurations else [a.zoom]):
 log=out/('run-view%d-orientation%d-zoom%d.log'%(view,orientation,zoom));r=run([str(out/'probe'),str(view),str(zoom),str(orientation),str(int(a.tv))],log);print(log.name,r.returncode,(r.stdout+r.stderr).splitlines()[-1:]);results.append({'view':view,'orientation':orientation,'zoom':zoom,'exit_code':r.returncode,'abort_traps_fired':'UNEXPECTED dependency:'in r.stderr,'log_sha256':sha(log)})
receipt={'source':str(s),'source_commit':commit,'tree':tree,'private':a.private,'source_status':subprocess.check_output(['git','-C',str(s),'status','--porcelain'],text=True),'runs':results,'exit_code':max(x['exit_code']!=0 for x in results),'sanitized':a.sanitize,'commands':cmds,'dependency_sha256':hashes,'unreached_abort_traps':names,'abort_traps_fired':any(x['abort_traps_fired']for x in results),'binary_sha256':sha(out/'probe')}
assert subprocess.check_output(['git','-C',str(s),'rev-parse','HEAD'],text=True).strip()==commit and subprocess.check_output(['git','-C',str(s),'rev-parse','HEAD^{tree}'],text=True).strip()==tree and not receipt['source_status']
summary=[]
for record in results:
 text=(out/('run-view%d-orientation%d-zoom%d.log'%(record['view'],record['orientation'],record['zoom']))).read_text();match=re.search(r'PASS view=(\d+) orientation=(\d+) zoom=(\d+) titles=(\d+) real_drags=(\d+) usable48dp_targets=(\d+) checks=(\d+)',text)
 if not record['exit_code']:assert match and int(match[4])==60 and int(match[5])>1 and int(match[6])>0
 if match:summary.append(dict(zip(['view','orientation','zoom','titles','real_drags','usable48dp_targets','checks'],map(int,match.groups()))))
assert len(results)==36
receipt.update(outcome='passed' if not receipt['exit_code'] else 'failed',passed=not receipt['exit_code'],cases=len(results),title_activations=sum(v['titles']for v in summary),case_summaries=summary,actual_producer_origin={'source':'src/descoberta.c','source_sha256':sha(s/'src/descoberta.c'),'unused_watched_producer_omitted':True,'actual_function_present':False},tooling_sha256={str(x.relative_to(here)):sha(x)for x in sorted(here.rglob('*'))if x.is_file()},scope='Actual complete collection render, real pointer/ctxhold/title-selection path and real layout/storage/profile producers; typed offline catalog/GPU/text/SDL boundaries. Every60 distinct title selections in six non-Showcase resolved layouts at both orientations/three scales, actual horizontal and vertical drags, usable48dp targets and live same-open resize bounds. No device/SDK/full-app claim.')
if not receipt['exit_code']:assert receipt['title_activations']==2160
(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
raise SystemExit(1 if any(x['exit_code']for x in results)else 0)
