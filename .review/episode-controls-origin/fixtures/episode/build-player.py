#!/usr/bin/env python3
"""Compile complete actual player with explicit host boundaries and aborting unused-code traps."""
from pathlib import Path
import argparse,hashlib,json,os,re,subprocess,shlex
p=argparse.ArgumentParser();p.add_argument('--cc',default='cc');p.add_argument('--source',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
p.add_argument('--real-pointer',action='store_true');p.add_argument('--render',action='store_true');p.add_argument('--san',action='store_true');p.add_argument('--touch',action='store_true');p.add_argument('--android',action='store_true');p.add_argument('--episode-buttons',action='store_true');a=p.parse_args()
here=Path(__file__).resolve().parent;source=a.source.resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest();watched=['src/player.c','src/player.h','src/proximo.c','src/proximo.h','src/perfis.c','src/layout.c','src/ponteiro.c','src/intro.c'];before={p:sha(source/p)for p in watched};records=[]
snapshotFiles=[p for p in (source/'src').rglob('*') if p.is_file() and p.suffix in {'.c','.h','.inc'}]+list(here.glob('*.c'))+list(here.glob('*.inc'))+list(Path(str(here/'sdk')).rglob('*.h'))+[Path(str(here/'sdk-extra.h'))]
sourceSnapshot={str(p.resolve()):sha(p) for p in snapshotFiles}
closure={}
flags=[a.cc,'-D_DARWIN_C_SOURCE','-std=gnu11','-O1','-g','-ffunction-sections','-fdata-sections','-Werror=implicit-function-declaration','-include',str(here/'sdk-extra.h'),'-I'+str(here/'sdk'),'-I'+str(source/'src')]
if a.real_pointer:flags+=['-DHAVE_REAL_POINTER=1','-DNV_LINUX_DESKTOP=1']
if a.render:flags+=['-DHAVE_RENDER=1']
if a.san:flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-sanitize-recover=all',*([]if __import__('sys').platform=='darwin'else['-fno-pie','-no-pie'])]
if a.touch:flags+=['-DNV_TOUCH_UI=1']
if a.android:flags+=['-DNV_ANDROID=1']
if a.episode_buttons:flags+=['-DHAVE_EPISODE_BUTTONS=1']
flags+=['-DPLAYER_SOURCE="'+str(source/'src/player.c')+'"']
def call(label,command,required=True):
 z=subprocess.run(command,capture_output=True,text=True);log=out/(label+'.log');log.write_text(z.stdout+z.stderr);records.append({'label':label,'command':command,'returncode':z.returncode,'log_sha256':sha(log)})
 if required:assert not z.returncode,z.stdout+z.stderr
 return z
objects=[]
inputs=[('fixture',here/'player-controls.c'),('proximo',source/'src/proximo.c'),('perfis',source/'src/perfis.c'),('layout',source/'src/layout.c'),('intro',source/'src/intro.c')]
if a.real_pointer:inputs+=[('ponteiro',source/'src/ponteiro.c')]
for name,file in inputs:
 obj=out/(name+'.o');dep=out/(name+'.d');call('compile-'+name,flags+['-MMD','-MF',str(dep),'-c',str(file),'-o',str(obj)]);objects.append(obj)
 deps=shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]);assert deps and str(file.resolve())in deps
 for d in deps:
  dp=Path(d).resolve();assert str(dp)in sourceSnapshot,str(dp)
  assert sha(dp)==sourceSnapshot[str(dp)],'Dependency changed during compile: '+str(dp)
  closure[str(dp)]=sourceSnapshot[str(dp)]
link=flags+list(map(str,objects))+[*(['-Wl,-dead_strip']if __import__('sys').platform=='darwin'else['-Wl,--gc-sections']),'-lm','-pthread','-o',str(out/'player-checks')]
z=call('link-probe',link,False);missing=sorted(set(re.findall(r"undefined reference to [`']([^'`]+)['`]",z.stderr)) | set(re.findall(r'^\s+"(_[^"\n]+)", referenced from:\s*$',z.stderr,re.M)))
assert not z.returncode or missing,z.stderr
traps='#include <stdio.h>\n#include <stdlib.h>\n'
for i,name in enumerate(missing):
 assert name.lstrip('_') not in ['nv_layout_w','nv_layout_h'],'Provide actual typed layout data instead of an aborting function'
 traps+='void unreachable_'+str(i)+'(void) __asm__('+json.dumps(name)+');\nvoid unreachable_'+str(i)+'(void){fputs('+json.dumps('UNEXPECTED dependency '+name+'\n')+',stderr);abort();}\n'
(out/'traps.c').write_text(traps);call('compile-traps',flags+['-c',str(out/'traps.c'),'-o',str(out/'traps.o')]);call('link',link[:-2]+[str(out/'traps.o')]+link[-2:])
z=call('run',[str(out/'player-checks')]);assert 'UNEXPECTED dependency'not in z.stdout+z.stderr
assert before=={p:sha(source/p)for p in watched}
assert all(sha(p)==h for p,h in closure.items()),'Compiled dependency changed before fixture completion'
(out/'result.json').write_text(json.dumps({'outcome':'passed','source':str(source),'source_hashes':before,'source_unchanged':True,'dependency_closure':closure,'dependency_closure_sha256':hashlib.sha256(json.dumps(closure,sort_keys=True).encode()).hexdigest(),'records':records,'unreached_trap_symbols':missing,'traps_fired':False,'ui':'touch'if a.touch else'tv','android':a.android,'sanitized':a.san,'fixture_sha256':{p.name:sha(p)for p in [here/'player-controls.c',here/'player-boundaries.inc',here/'player-render.inc']},'scope':'Actual complete player module with explicit idle service boundaries, real episode/profile modules and abort-on-use unused dependencies; no actual decoder/network/GPU claim.'},indent=2)+'\n')
print('\n'.join(line for line in z.stdout.splitlines() if line.startswith('PASS ')))
