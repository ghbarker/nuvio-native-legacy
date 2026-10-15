from pathlib import Path
import hashlib,json,os,subprocess,sys,platform
cc=os.environ.get("REVIEW_CC","cc");darwin=platform.system()=="Darwin"
from fontTools.ttLib import TTFont
root=Path(__file__).resolve().parent
source=Path(os.environ.get('SOURCE',str(root.parent/'source')))
font=source/'deploy/app/fonts/InterDisplay-Regular.ttf'
f=TTFont(font);cmap=f.getBestCmap();upm=f['head'].unitsPerEm
text=(root/'reader_probe.c').read_text()
chars=set(range(32,127))|set(map(ord,text))
rows=[]
for cp in sorted(chars):
    if cp in cmap:rows.append('{%d, %.12f}'%(cp,f['hmtx'][cmap[cp]][0]/upm))
(root/'font_metrics.inc').write_text('static const struct {unsigned cp;double advance;} advances[]={'+','.join(rows)+'};\n')
san='--san' in sys.argv
route='--route' in sys.argv
close='--close' in sys.argv
preview='--preview' in sys.argv
platform='touch' if '--tv' not in sys.argv else 'tv'
out=Path(os.environ.get('PROOF_OUTPUT',str(root)))/('asan' if san else 'normal')/(platform+('-route' if route else '-close' if close else '-preview' if preview else ''));out.mkdir(parents=True,exist_ok=True)
flags=['-O1','-g','-ffunction-sections','-fdata-sections','-I'+str(root/'sdk'),'-I'+str(source/'src'),'-DSINOPSE_SOURCE='+json.dumps(str(source/'src/sinopse.c')),'-DDETAIL_SOURCE='+json.dumps(str(source/'src/detail.c'))]
flags+=['-DNV_ANDROID=1','-DNV_TOUCH_UI=1'] if platform=='touch' else ['-DNV_TPK=1']
if os.environ.get('PRIVATE_TEST')=='1':flags+=['-DNV_TEST_LOCAL_LOGS=1']
if san:flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-sanitize-recover=all',*([]if darwin else['-fno-pie'])]
inputs=[root/('route_probe.c' if route else 'close_refinement.c' if close else 'preview_refinement.c' if preview else 'reader_probe.c'),source/'src/layout.c',source/'src/ponteiro.c']
if route:inputs+=[source/'src/detmais.c']
bound=inputs+[root/'reader_probe.c',source/'src/detail.c',source/'src/sinopse.c',source/'src/sinopse.h',source/'src/text.h',font,Path(__file__).resolve(),root/'font_metrics.inc']
before={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in bound}
objs=[]
for i,p in enumerate(inputs):
    obj=out/(str(i)+'.o');objs.append(str(obj));r=subprocess.run([cc,*flags,'-c',str(p),'-o',str(obj)],capture_output=True,text=True,timeout=150)
    (out/(str(i)+'.compile.log')).write_text(r.stdout+r.stderr)
    if r.returncode:print(r.stderr);sys.exit(r.returncode)
link=[cc,*objs,'-Wl,-dead_strip'if darwin else'-Wl,--gc-sections','-lm','-o',str(out/'probe')]
if san:link+=['-fsanitize=address,undefined']+([]if darwin else['-no-pie'])
r=subprocess.run(link,capture_output=True,text=True,timeout=150)
(out/'link.log').write_text(r.stdout+r.stderr)
if r.returncode and route:
    import re
    names=sorted(set(re.findall(r"undefined reference to [`']([^'`]+)['`]",r.stderr)+re.findall(r'"(_[^"]+)", referenced from:',r.stderr)))
    trap='#include <stdio.h>\n#include <stdlib.h>\n'
    for i,name in enumerate(names):trap+=f'void trap{i}(void) __asm__({json.dumps(name)});\nvoid trap{i}(void){{fprintf(stderr,"UNEXPECTED dependency: {name}\\n");abort();}}\n'
    (out/'traps.c').write_text(trap);subprocess.run([cc,*flags,'-c',str(out/'traps.c'),'-o',str(out/'traps.o')],check=True)
    link.insert(1,str(out/'traps.o'));r=subprocess.run(link,capture_output=True,text=True,timeout=150)
if r.returncode:print(r.stderr);sys.exit(r.returncode)
cases=[]
screens=[(1080,2340,1),(2340,1080,1)] if platform=='touch' else [(1920,1080,0)]
if route or preview:screens=[(1080,2340,1)]
for width,height,mobile in screens:
  for scale in ([1] if route or preview else [1,1.2,1.3,1.5]):
    r=subprocess.run([str(out/'probe'),str(width),str(height),str(scale),str(mobile)],capture_output=True,text=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0:abort_on_error=1'},timeout=45)
    case={'width':width,'height':height,'scale':scale,'mobile':mobile,'returncode':r.returncode,'stdout':r.stdout,'stderr':r.stderr}
    cases.append(case);print(case)
    if r.returncode:break
  if r.returncode:break
after={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in bound}
receipt={'source':str(source),'platform':platform,'sanitizers':'address,undefined' if san else None,'inputs_sha256':before,'source_inputs_unchanged':before==after,'cases':cases,'font_boundary':'Shipped InterDisplay-Regular hmtx advances; per-glyph widths without shaping/kerning. Real module/layout/pointer code and captured draw/clip geometry. Not GL/device rendering.'}
(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
sys.exit(0 if before==after and all(c['returncode']==0 for c in cases) else 1)
