#!/usr/bin/env python3
"""Compile and completely link the exact normal TV and touch apps on macOS."""
from pathlib import Path
import argparse, concurrent.futures, json, os, platform, subprocess
from common import sha, verify

p=argparse.ArgumentParser()
for name in ['source','proof','output']:p.add_argument('--'+name,type=Path,required=True)
a=p.parse_args(); source=a.source.resolve(); out=a.output.resolve(); out.mkdir(parents=True,exist_ok=True)
config,proof,manifest=verify(source,a.proof)
assert platform.system()=='Darwin', 'A real macOS SDK is required'
cc='/usr/bin/cc'; sdk=Path(subprocess.check_output(['brew','--prefix'],text=True).strip())
env=dict(os.environ,DYLD_LIBRARY_PATH=str(sdk/'lib')+':'+os.environ.get('DYLD_LIBRARY_PATH',''))
base=['-std=gnu11','-O1','-g','-Wno-deprecated-declarations','-I'+str(sdk/'include'),'-I'+str(sdk/'include/SDL2'),'-I'+str(source/'src')]
for name in ['NV_SUPABASE_URL','NV_SUPABASE_ANON_KEY','NV_TV_LOGIN_BASE','NV_TRAKT_CLIENT_ID','NV_TRAKT_CLIENT_SECRET','NV_SIMKL_CLIENT_ID','NV_SIMKL_APP','NV_TMDB_API_KEY']:
    base.append('-D'+name+'=""')
libs=['-L'+str(sdk/'lib'),'-Wl,-rpath,'+str(sdk/'lib'),'-lSDL2','-lSDL2_image','-lSDL2_ttf','-lz','-framework','OpenGL']
records=[]
def run(name,cmd):
    r=subprocess.run(cmd,capture_output=True,text=True,env=env,timeout=900)
    log=out/(name+'.log');log.write_text(r.stdout+r.stderr)
    assert not r.returncode,(name,(r.stdout+r.stderr)[-2500:])
    return {'name':name,'returncode':r.returncode,'log_sha256':sha(log)}
records.append(run('translations',['python3',str(source/'tools/idiomas.py')]))
sanity=out/'sdk-sanity.c';sanity.write_text('#include <SDL2/SDL.h>\n#include <OpenGL/gl3.h>\n#include <stdio.h>\nint main(void){printf("SDL SDK %d.%d.%d ready\\n",SDL_MAJOR_VERSION,SDL_MINOR_VERSION,SDL_PATCHLEVEL);return 0;}\n')
records.append(run('sdk-compile',[cc,*base,str(sanity),*libs,'-o',str(out/'sdk-sanity')]))
records.append(run('sdk-run',[str(out/'sdk-sanity')]))
units=sorted((source/'src').glob('*.c'))+sorted((source/'src/dts').glob('*.c'))
assert units and source/'src/main.c' in units
apps=[]
for ui in ['tv','touch']:
    directory=out/('objects-'+ui);directory.mkdir(exist_ok=True)
    flags=['-DNV_TOUCH_UI=1'] if ui=='touch' else []
    def compile(job):
        i,path=job;name=f'{i:04d}-{path.stem}';obj=directory/(name+'.o');dep=directory/(name+'.d')
        cmd=[cc,*base,*flags,'-MMD','-MF',str(dep),'-c',str(path),'-o',str(obj)]
        r=subprocess.run(cmd,capture_output=True,text=True,env=env,timeout=900)
        log=directory/(name+'.log');log.write_text(r.stdout+r.stderr)
        assert not r.returncode,(str(path),r.stderr[-2500:])
        return obj,{'source':str(path.relative_to(source)),'source_sha256':sha(path),'dependency_file_sha256':sha(dep),'object_sha256':sha(obj),'log_sha256':sha(log)}
    with concurrent.futures.ThreadPoolExecutor(max_workers=3) as pool:
        objects=list(pool.map(compile,enumerate(units)))
    binary=out/('pr431-'+ui);records.append(run('link-'+ui,[cc,*[str(o) for o,r in objects],*libs,'-o',str(binary)]))
    apps.append({'ui':ui,'complete_translation_units':len(objects),'binary_sha256':sha(binary),'objects':[r for o,r in objects]})
verify(source,a.proof)
receipt={'outcome':'passed','source_binding':proof,'compiler':cc,'compiler_version':subprocess.check_output([cc,'--version'],text=True).splitlines()[0],'apps':apps,'records':records,'scope':'Fresh complete native SDK compile/link for standalone TV/touch; app binaries are not launched; original sanitizer suites run separately.','reused_test_passes':False}
(out/'native-verified.json').write_text(json.dumps(receipt,indent=2)+'\n')
print('Passed exact standalone complete TV/touch native app links')
