"""GNU/Linux CPU contract checks using complete current production C units.

No GL, playback or network is performed. Unexpected reachable service calls
abort. All real source/header inputs, commands, binaries and raw logs are bound.
"""
from pathlib import Path
import argparse, hashlib, json, os, re, shlex, subprocess

HERE=Path(__file__).resolve().parent
def sha(path): return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def git(source,*args): return subprocess.check_output(['git','-C',str(source),*args],text=True).strip()

p=argparse.ArgumentParser()
p.add_argument('--source',type=Path,required=True)
p.add_argument('--output',type=Path,required=True)
p.add_argument('--cc',default='/usr/bin/cc')
p.add_argument('--kind',choices=['cinema','settings'],required=True)
p.add_argument('--sanitize',action='store_true')
p.add_argument('--touch',action='store_true')
a=p.parse_args(); source=a.source.resolve(); out=a.output.resolve()
out.mkdir(parents=True,exist_ok=False)
head=git(source,'rev-parse','HEAD'); tree=git(source,'rev-parse','HEAD^{tree}')
assert not git(source,'status','--porcelain')
commands=[]
def run(label,cmd,allow_undefined=False,env=None):
    r=subprocess.run(cmd,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,
                     timeout=600,env=env)
    log=out/(label+'.log'); log.write_text(r.stdout)
    commands.append({'label':label,'command':cmd,'returncode':r.returncode,
                     'log':log.name,'log_sha256':sha(log)})
    if r.returncode and not allow_undefined:
        print('\n'.join(r.stdout.splitlines()[-70:]),flush=True)
        raise RuntimeError((label,r.returncode))
    return r

run('compiler-version',[a.cc,'--version'])
flags=['-O1','-g','-ffunction-sections','-fdata-sections','-DNV_TIZEN',
       '-I'+str(HERE/'sdk'),'-I'+str(source/'src'),'-Wno-deprecated-declarations',
       '-DISSUE508']
if a.touch: flags+=['-DNV_TOUCH_UI=1']
if a.sanitize:
    flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer',
            '-fno-sanitize-recover=all','-fno-pie']
if a.kind=='cinema':
    flags+=['-DJOINED_DETAIL','-DPLAYER_SOURCE="'+str(source/'src/player.c')+'"',
            '-DDETAIL_SOURCE="'+str(source/'src/detail.c')+'"']
    inputs=[('fixture',HERE/'cinema-contract.c'),('detail',HERE/'detail-boundary.c'),
            ('artehero',source/'src/artehero.c'),('abrecine',source/'src/abrecine.c')]
else:
    flags+=['-DSETTINGS_SOURCE="'+str(source/'src/ajustes.c')+'"']
    inputs=[('fixture',HERE/'settings-contract.c')]
objects=[]; depfiles=[]
for name,path in inputs:
    obj=out/(name+'.o'); dep=out/(name+'.d'); depfiles.append(dep)
    run(name+'-compile',[a.cc,*flags,'-MMD','-MF',str(dep),'-c',str(path),'-o',str(obj)])
    objects.append(str(obj))
binary=out/'probe'
link=[a.cc,*objects,'-Wl,--gc-sections','-lm','-o',str(binary)]
if a.sanitize: link+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']
first=run('unresolved-link',link,allow_undefined=True)
names=sorted(set(re.findall(r"undefined reference to [`']([^'`]+)['`]",first.stdout)))
if first.returncode and not names: raise RuntimeError(first.stdout)
assert all(re.fullmatch(r'[A-Za-z_][A-Za-z_0-9]*',n) for n in names),names
traps='#include <stdio.h>\n#include <stdlib.h>\n'
for i,name in enumerate(names):
    traps+=f'void unused_{i}(void) __asm__({json.dumps(name)});\n'
    traps+=f'void unused_{i}(void) {{fprintf(stderr,"UNEXPECTED dependency: {name}\\n");abort();}}\n'
(out/'unreached-traps.c').write_text(traps)
run('traps-compile',[a.cc,*flags,'-c',str(out/'unreached-traps.c'),'-o',str(out/'traps.o')])
link.insert(1,str(out/'traps.o')); run('final-link',link)
env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0:halt_on_error=1:abort_on_error=1',
         UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
r=run('run',[str(binary)],env=env)
assert 'UNEXPECTED dependency:' not in r.stdout
if a.kind=='cinema':
    match=re.search(r'^RESULT checks=(\d+) failures=(\d+)$',r.stdout,re.M)
    assert match and int(match[1])==38 and int(match[2])==0,r.stdout
    checks=int(match[1])
else:
    match=re.search(r'^PASS SETTINGS checks=(\d+) ',r.stdout,re.M)
    assert match and int(match[1])==19,r.stdout
    checks=int(match[1])
deps=set()
for dep in depfiles:
    text=dep.read_text().replace('\\\n',' ')
    for name in shlex.split(text.split(':',1)[1]): deps.add(Path(name).resolve())
assert all(path.is_relative_to(source) or path.is_relative_to(HERE) for path in deps),deps
dependency_sha256={str(path):sha(path) for path in sorted(deps)}
source_sha256={str(path.relative_to(source)):sha(path) for path in sorted(deps) if path.is_relative_to(source)}
assert git(source,'rev-parse','HEAD')==head and git(source,'rev-parse','HEAD^{tree}')==tree
assert not git(source,'status','--porcelain')
raw={str(path.relative_to(out)):sha(path) for path in sorted(out.rglob('*')) if path.is_file() and path.suffix!='.o'}
receipt={'passed':True,'outcome':'passed','kind':a.kind,'checks':checks,
         'source_commit':head,'source_tree':tree,'source_clean':True,
         'source_sha256':source_sha256,'dependency_sha256':dependency_sha256,
         'sanitize':a.sanitize,'sanitizers':['address','undefined'] if a.sanitize else [],
         'touch_compiled':a.touch,'runtime_mobile':False,'actual_detail_joined':a.kind=='cinema',
         'actual_units':[str(path) for name,path in inputs],
         'binary':'probe','binary_sha256':sha(binary),'commands':commands,
         'unreached_traps':names,'abort_traps_fired':False,'raw_files_sha256':raw,
         'fixture_sha256':{str(path.relative_to(HERE)):sha(path) for path in sorted(HERE.rglob('*')) if path.is_file()},
         'scope':'Complete real production units with typed offline texture/drawing/catalog boundaries. TV and touch compilation at fixed1920x1080; no device/GL/network/render screenshot or mobile geometry claim.'}
(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(f'PASS {a.kind}: {checks} checks; touch={a.touch}; sanitize={a.sanitize}',flush=True)
