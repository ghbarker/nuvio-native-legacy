"""Actual #499 parser/render/native/budget contracts. No product modifications."""
from pathlib import Path
import argparse,hashlib,json,os,platform,re,subprocess
A=argparse.ArgumentParser();A.add_argument('--source',required=True);A.add_argument('--output',required=True);A.add_argument('--cc',default='cc');A.add_argument('--sdk',default=str(Path(__file__).parent/'sdk'));A.add_argument('--platform',choices=['tv','mobile'],default='tv');A.add_argument('--sanitize',action='store_true');A.add_argument('--private',action='store_true');a=A.parse_args()
R=Path(__file__).resolve().parent;src=Path(a.source).resolve()/'src';O=Path(a.output).resolve();O.mkdir(parents=True,exist_ok=True);darwin=platform.system()=='Darwin'
mods={'metadata':['stream_parse.c','js.c','badges.c','fontefiltro.c','linguas.c'],'native':['fontefalha.c','fontefiltro.c','badges.c','naovideo.c'],'caption':['linguas.c','fontefalha.c','fontefiltro.c'],'refresh':['fontefalha.c','fontefiltro.c','badges.c','naovideo.c','linguas.c','stream_parse.c','js.c']}
raw=(src/'streams.c').read_text();neg=re.search(r'if\(s->foraCache[^\n]+caixaAlta\("Fora do cache"[^\n]+;',raw);cap=re.search(r'\{ char pv\[160\];\n.*?lp=txt_linha_corta\([^\n]+; \}',raw,re.S);assert neg and cap
rows=[]
for group,modules in mods.items():
 out=O/group;out.mkdir(exist_ok=True);fixture=R/'fixtures'/(group+'.c');text=fixture.read_text()
 if group in ['caption','refresh']:text=re.sub(r'if\(s->foraCache[^\n]+caixaAlta\("Fora do cache"[^\n]+;',lambda _:neg[0],text)
 if group=='caption':
  old=re.search(r'\{ char pv\[160\];\n.*?lp=txt_linha_corta\([^\n]+; \}',text,re.S);assert old;text=text[:old.start()]+cap[0]+text[old.end():]
 f=out/'fixture.c';f.write_text(text)
 flags=['-std=gnu11','-O1','-g','-ffunction-sections','-fdata-sections','-I'+a.sdk,'-I'+str(src),'-DNV_STREAMS_SOURCE='+json.dumps(str(src/'streams.c')),'-DNV_CACHE_FIX=1','-DNV_CAPTION_NEGATIVE=1','-DNV_NATIVE_REFRESH=1']
 if a.platform=='mobile':flags+=['-DNV_ANDROID=1','-DNV_TOUCH_UI=1']
 if a.private:flags+=['-DNV_TEST_LOCAL_LOGS=1']
 if a.sanitize:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer']+([]if darwin else['-fno-pie','-no-pie'])
 files=[f]+[src/n for n in modules];bound=files+[src/'streams.c',src/'streams.h'];before={str(p):hashlib.sha256(p.read_bytes()).hexdigest()for p in bound}
 cmd=[a.cc,*flags,*map(str,files),'-Wl,-dead_strip'if darwin else'-Wl,--gc-sections','-pthread','-lm','-o',str(out/'probe')]
 c=subprocess.run(cmd,capture_output=True,text=True,timeout=150);(out/'compile.log').write_text(c.stdout+c.stderr);row={'group':group,'compile_returncode':c.returncode,'inputs_sha256':before,'command':cmd}
 if c.returncode==0:
  p=subprocess.run([str(out/'probe')],capture_output=True,text=True,timeout=45,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0:abort_on_error=1'});(out/'execute.log').write_text(p.stdout+p.stderr);row.update(returncode=p.returncode,stdout=p.stdout,stderr=p.stderr)
 row['source_inputs_unchanged']=before=={str(p):hashlib.sha256(p.read_bytes()).hexdigest()for p in bound};row['passed']=c.returncode==0 and row.get('returncode')==0 and row['source_inputs_unchanged'];rows.append(row)
receipt={'passed':all(r['passed']for r in rows),'source_commit':subprocess.check_output(['git','rev-parse','HEAD'],cwd=src.parent,text=True).strip(),'source_tree':subprocess.check_output(['git','rev-parse','HEAD^{tree}'],cwd=src.parent,text=True).strip(),'platform':a.platform,'private':a.private,'sanitizers':'address,undefined'if a.sanitize else None,'groups':rows,'fixture_compatibility_sha256':hashlib.sha256((R/'fixture-compatibility.json').read_bytes()).hexdigest()};(O/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');assert receipt['passed']
