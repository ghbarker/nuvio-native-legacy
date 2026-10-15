#!/usr/bin/env python3
from pathlib import Path
import argparse,concurrent.futures,hashlib,json,subprocess
parser=argparse.ArgumentParser();parser.add_argument('--source',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--cc',default='cc');a=parser.parse_args();here=Path(__file__).resolve().parent;out=a.output.resolve();out.mkdir(parents=True,exist_ok=True);root=a.source.resolve();sdk=here.parent/'sdk';jni=sdk
base=[a.cc,'-std=gnu11','-fsyntax-only','-Werror=implicit-function-declaration','-include',str(here/'sdk-extra.h'),'-I'+str(sdk),'-I'+str(jni),'-I'+str(here/'sdk'),'-Isrc']
if __import__('sys').platform=='darwin':
 prefix=subprocess.check_output(['brew','--prefix'],text=True).strip();base[1:1]=['-I'+prefix+'/include','-I'+prefix+'/include/SDL2']
def check(item):
    name,args=item;cmd=base+args
    r=subprocess.run(cmd,cwd=root,text=True,capture_output=True,timeout=20)
    (out/(name+'-syntax.log')).write_text(r.stdout+r.stderr)
    return {'branch':name,'command':cmd,'exit_code':r.returncode,'passed':r.returncode==0}
cases=[('desktop',['-DNV_LINUX_DESKTOP','src/video.c']),('webos',['src/video.c']),('android',['-DNV_ANDROID','src/video_android.c']),('tizen-tpk',['-DNV_TPK','src/video_tpk.c']),('tizen-browser',['-D__EMSCRIPTEN__','src/video_tizen.c'])]
with concurrent.futures.ThreadPoolExecutor(max_workers=5) as pool: records=list(pool.map(check,cases))
receipt={'scope':'Host whole-translation-unit C syntax with typed SDK boundaries and real JNI headers; no native SDK link/device execution; Emscripten JavaScript body unchanged','compiler':subprocess.check_output([a.cc,'--version'],text=True).splitlines()[0],'records':records,'source_sha256':{p:hashlib.sha256((root/'src'/p).read_bytes()).hexdigest() for p in ['video.h','video.c','video_android.c','video_tizen.c','video_tpk.c']}}
(out/'syntax-receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
assert all(r['passed'] for r in records),records
print(json.dumps({'passed':len(records),'receipt':str(out/'syntax-receipt.json')}))
