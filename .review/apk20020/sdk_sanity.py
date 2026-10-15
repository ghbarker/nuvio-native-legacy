#!/usr/bin/env python3
"""Reject real Darwin compiler/SDK setup errors before original full-source tests."""
from pathlib import Path
import argparse,hashlib,json,os,platform,subprocess
from require_plan import load_plan
p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);a=p.parse_args()
plan,_=load_plan(Path(__file__).resolve().parent/'plan.json')
assert platform.system()=='Darwin'
cc=Path(os.environ['NUVIO_REVIEW_REAL_CC']).resolve()
assert cc==Path('/usr/bin/cc').resolve() and cc.is_file()
assert not a.output.exists();a.output.mkdir(parents=True)
prefix=Path(subprocess.check_output(['brew','--prefix'],text=True).strip())
sdl3=Path(subprocess.check_output(['brew','--prefix','sdl3'],text=True).strip())/'lib';assert sdl3.is_dir()
src=a.output/'sanity.c';src.write_text('#include <stdio.h>\n#include <AvailabilityMacros.h>\n#include <SDL2/SDL.h>\n#include <OpenGL/gl3.h>\nint main(void){printf("SDL %d SDK ready\\n",SDL_MAJOR_VERSION);return 0;}\n')
cmd=[str(cc),'-std=gnu11','-I'+str(prefix/'include'),'-I'+str(prefix/'include/SDL2'),str(src),'-L'+str(prefix/'lib'),'-Wl,-rpath,'+str(prefix/'lib'),'-lSDL2','-lSDL2_image','-lSDL2_ttf','-framework','OpenGL','-o',str(a.output/'sanity')]
for label,args in [('compile',cmd),('execute',[str(a.output/'sanity')])]:
 r=subprocess.run(args,capture_output=True,text=True,timeout=60);(a.output/(label+'.log')).write_text(r.stdout+r.stderr);assert r.returncode==0,(label,r.stderr)
(a.output/'sdk-verified.json').write_text(json.dumps({'outcome':'passed','compiler':str(cc),'compiler_version':subprocess.check_output([str(cc),'--version'],text=True).splitlines()[0],'plan_sha256':hashlib.sha256((Path(__file__).resolve().parent/'plan.json').read_bytes()).hexdigest(),'review_commit':os.environ['GITHUB_SHA'],'sdk_headers':['stdio.h','AvailabilityMacros.h','SDL2/SDL.h','OpenGL/gl3.h'],'command':cmd,'sdl3_library_dir':str(sdl3)},indent=2)+'\n')
