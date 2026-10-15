from pathlib import Path
import subprocess, re, sys, json, hashlib, os, platform
cc=os.environ.get("REVIEW_CC","cc");darwin=platform.system()=="Darwin"

root=Path(__file__).resolve().parent
label, source_arg=sys.argv[1:3]
source=Path(source_arg).resolve();san='--san' in sys.argv
extra_modules=['legenda2','linguas','fontefalha','fonteregra','regexjs','js']+(['fontefiltro']if(source/'src/fontefiltro.c').is_file()else[])
modules=['legenda2','layout','ponteiro','fileiras','dados',*extra_modules]
bound=[source/'src/ajustes.c',*(source/'src').glob('*.h'),*(source/'src').glob('ajustes*.inc'),root/'settings.c',root/'player-prefs-companion.c',source/'src/player.c',Path(__file__).resolve()]+[source/'src'/(m+'.c')for m in modules]
before={str(p):hashlib.sha256(p.read_bytes()).hexdigest()for p in bound}
out=Path(os.environ.get('PROOF_OUTPUT',str(root)))/label;out.mkdir(parents=True,exist_ok=True)
flags=['-O1','-g','-ffunction-sections','-fdata-sections','-DNV_ANDROID=1', '-DNV_TOUCH_UI=1',
       '-I'+str(root/'sdk'),'-I'+str(source/'src'),
       '-DPLAYER_SOURCE='+json.dumps(str(source/'src/player.c')),'-Wno-deprecated-declarations','-DSETTINGS_SOURCE="'+str(source/'src/ajustes.c')+'"']

if os.environ.get('PRIVATE_TEST')=='1':flags+=['-DNV_TEST_LOCAL_LOGS=1']
if '--touch' in sys.argv:flags+=['-DNV_TOUCH_UI=1']
if san:flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer',*([]if darwin else['-fno-pie'])]
subprocess.run([cc,*flags,'-c',str(root/'settings.c'),'-o',str(out/'fixture.o')],check=True)
subprocess.run([cc,*flags,'-c',str(source/'src/layout.c'),'-o',str(out/'layout.o')],check=True)
subprocess.run([cc,*flags,'-c',str(source/'src/ponteiro.c'),'-o',str(out/'pointer.o')],check=True)
subprocess.run([cc,*flags,'-c',str(source/'src/fileiras.c'),'-o',str(out/'fileiras.o')],check=True)
subprocess.run([cc,*flags,'-c',str(source/'src/dados.c'),'-o',str(out/'dados.o')],check=True)

for module in extra_modules:
 subprocess.run([cc,*flags,'-c',str(source/'src'/f'{module}.c'),'-o',str(out/f'{module}.o')],check=True)
subprocess.run([cc,*flags,'-c',str(root/'player-prefs-companion.c'),'-o',str(out/'player-prefs.o')],check=True)
link=[cc,str(out/'player-prefs.o'),*[str(out/(n+'.o'))for n in ['fixture','layout','pointer','fileiras','dados',*extra_modules]],'-Wl,-dead_strip'if darwin else'-Wl,--gc-sections','-lm','-o',str(out/'settings-contract')]
if san:link+=['-fsanitize=address,undefined']+([]if darwin else['-no-pie'])
r=subprocess.run(link,capture_output=True,text=True,timeout=150)
names=sorted(set(re.findall(r"undefined reference to [`']([^'`]+)['`]",r.stderr)+re.findall(r'"(_[^"]+)", referenced from:',r.stderr)))
if r.returncode and not names:raise RuntimeError(r.stderr)
traps='#include <stdio.h>\n#include <stdlib.h>\n'
for i,name in enumerate(names):
    traps+=f'void unused_{i}(void) __asm__({json.dumps(name)});\nvoid unused_{i}(void) {{fprintf(stderr,"UNEXPECTED dependency: {name}\\n");abort();}}\n'
(out/'traps.c').write_text(traps)
subprocess.run([cc,*flags,'-c',str(out/'traps.c'),'-o',str(out/'traps.o')],check=True)
link.insert(1,str(out/'traps.o'));subprocess.run(link,check=True)
r=subprocess.run([str(out/'settings-contract'),os.environ.get('SETTING_OP','137')],capture_output=True,text=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'},timeout=45)
(out/'result.log').write_text(r.stdout+r.stderr)
(out/'result.json').write_text(json.dumps({'source':str(source),'inputs_sha256':before,'source_inputs_unchanged':before=={str(p):hashlib.sha256(p.read_bytes()).hexdigest()for p in bound},'source_sha256':hashlib.sha256((source/'src/ajustes.c').read_bytes()).hexdigest(),
  'sanitizers':'address,undefined' if san else None,'touch_compiled':'-DNV_TOUCH_UI=1' in flags,'linked_production_modules':modules,'returncode':r.returncode,
  'unreached_traps':len(names),'traps_fired':'UNEXPECTED dependency:' in r.stderr,'stdout':r.stdout,'stderr':r.stderr},indent=2)+'\n')
print(r.stdout+r.stderr);sys.exit(r.returncode)
