#!/usr/bin/env python3
"""Compile complete TV/touch apps, reuse only identical dependency objects, run focused actual C."""
from pathlib import Path
import argparse,concurrent.futures,hashlib,json,os,platform,shutil,subprocess,tempfile
from reuse import assert_translation_sets,dependency_snapshot,reuse_evidence
p=argparse.ArgumentParser();p.add_argument('--product',required=True,type=Path);p.add_argument('--test',required=True,type=Path);p.add_argument('--output',required=True,type=Path);p.add_argument('--cc',default='/usr/bin/cc');p.add_argument('--product-proof',required=True,type=Path);p.add_argument('--test-proof',required=True,type=Path);a=p.parse_args();assert platform.system()=='Darwin','Complete native SDK stage runs on macOS'
here=Path(__file__).resolve().parent;out=a.output.resolve();out.mkdir(parents=True,exist_ok=True);product=a.product.resolve();test=a.test.resolve();cc=str(Path(a.cc).resolve());assert Path(cc).is_absolute() and Path(cc).is_file();prefix=subprocess.check_output(['brew','--prefix'],text=True).strip();sdk=Path(prefix);sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest();records=[]
env=dict(os.environ,CC=cc,DYLD_LIBRARY_PATH=str(sdk/'lib')+':'+os.environ.get('DYLD_LIBRARY_PATH',''));base=['-std=gnu11','-O1','-g','-Wno-deprecated-declarations','-I'+str(sdk/'include'),'-I'+str(sdk/'include/SDL2')]
bindings={}
for name,source,file in [('product',product,a.product_proof),('test',test,a.test_proof)]:
 v=json.loads(file.read_text());assert v['verified'] and v['layer']==name and v['source_clean'] and not v['review_only']
 assert subprocess.check_output(['git','-C',str(source),'rev-parse','HEAD'],text=True).strip()==v['source_commit']
 assert subprocess.check_output(['git','-C',str(source),'rev-parse','HEAD^{tree}'],text=True).strip()==v['tree']
 assert not subprocess.check_output(['git','-C',str(source),'status','--porcelain'],text=True).strip()
 bindings[name]={'source_commit':v['source_commit'],'tree':v['tree'],'review_commit':v['review_commit'],'proof_sha256':sha(file),'manifest_sha256':v['manifest_sha256']}
macros=['NV_SUPABASE_URL','NV_SUPABASE_ANON_KEY','NV_TV_LOGIN_BASE','NV_TRAKT_CLIENT_ID','NV_TRAKT_CLIENT_SECRET','NV_SIMKL_CLIENT_ID','NV_SIMKL_APP','NV_TMDB_API_KEY'];base+=['-D'+x+'=""' for x in macros];libs=['-L'+str(sdk/'lib'),'-Wl,-rpath,'+str(sdk/'lib'),'-lSDL2','-lSDL2_image','-lSDL2_ttf','-lz','-framework','OpenGL']
def execute(label,cmd):
 limit=60 if label=='sdk-compile' else 20 if label=='sdk-run' else 900
 try:z=subprocess.run(cmd,capture_output=True,text=True,env=env,timeout=limit)
 except subprocess.TimeoutExpired as error:
  decode=lambda data:data.decode(errors='replace') if isinstance(data,bytes) else (data or '')
  (out/(label+'.log')).write_text(decode(error.stdout)+decode(error.stderr)+'\nTIMEOUT after '+str(limit)+' seconds\n')
  raise AssertionError(label+': timed out after '+str(limit)+' seconds') from error
 (out/(label+'.log')).write_text(z.stdout+z.stderr);assert not z.returncode,(label,(z.stdout+z.stderr)[-3000:]);return {'label':label,'command':cmd,'returncode':0,'timeout_seconds':limit,'log_sha256':sha(out/(label+'.log'))}
records.append(execute('translations',['python3',str(product/'tools/idiomas.py')]))
# Detect compiler/SDK errors before spending time on whole-source compilation.
sanity=out/'sdk-sanity.c';sanity.write_text('#include <stdio.h>\n#include <SDL2/SDL.h>\n#include <OpenGL/gl3.h>\nint main(void){printf("SDL %d.%d.%d SDK ready\\n",SDL_MAJOR_VERSION,SDL_MINOR_VERSION,SDL_PATCHLEVEL);return 0;}\n');records.append(execute('sdk-compile',[cc,*base,str(sanity),*libs,'-o',str(out/'sdk-sanity')]));records.append(execute('sdk-run',[str(out/'sdk-sanity')]))
# Actual merged parser/cache/filter/preparation/worker/profile storage, both normal and sanitizers.
f=here/'fixtures';s=product/'src';parser=[s/n for n in ['stream_parse.c','fontefiltro.c','js.c','badges.c']]
groups={'original-parser':([product/'tests/stream_parser.c']+parser,False),'cache-metadata':([f/'cache_metadata.c']+parser,False),'exclusion-parser':([f/'reviewed-fontefiltro_parse.c']+parser,False),'preparation':([f/'preparation_filters.c',s/'fontefalha.c',s/'fontefiltro.c',s/'fonteauto.c',s/'perfis.c'],True),'profile-bank':([f/'profile-bank-filters.c',s/'fontefalha.c',s/'fontefiltro.c'],False),'profile-scope':([f/'profile-scope-filters.c',s/'debrid.c',s/'fontefalha.c',s/'fontefiltro.c'],False),'worker':([f/'reviewed-fontefalha_worker.c',s/'fontefalha.c',s/'fontefiltro.c',s/'fonteauto.c',s/'perfis.c'],True),'cache-indicator':([f/'cache_indicator.c'],False)}
failures=[]
for name,(inputs,data) in groups.items():
 for san in [False,True]:
  label='focused-'+name+('-asan' if san else '-normal');binary=out/label;flags=['-D_DARWIN_C_SOURCE=1','-ffunction-sections','-fdata-sections','-I'+str(s),'-DNV_STREAMS_SOURCE="'+str(s/'streams.c')+'"','-DNV_CACHE_FIX=1']+(['-fsanitize=address,undefined,float-cast-overflow','-fno-omit-frame-pointer','-fno-sanitize-recover=all'] if san else [])
  try:
   records.append(execute(label+'-compile',[cc,*base,*flags,*map(str,inputs),'-Wl,-dead_strip','-lpthread','-lm','-o',str(binary)]))
   with tempfile.TemporaryDirectory(prefix='logs8-focus-') as d:records.append(execute(label+'-run',[str(binary)]+([d] if data else [])))
  except AssertionError as error:failures.append({'label':label,'error':str(error)});print('FAILED',label,str(error),flush=True)
for kind in ['dispatch','owner']:
 try:records.append(execute('actual513-'+kind,['python3',str(here/'issue513'/kind/'run.py'),'--source',str(product),'--output',str(out/('actual513-'+kind)),'--mode','both']))
 except AssertionError as error:failures.append({'label':'actual513-'+kind,'error':str(error)});print('FAILED actual513-'+kind,str(error),flush=True)
(out/'focused-preflight-status.json').write_text(json.dumps({'source_bindings':bindings,'outcome':'failed' if failures else 'passed','records':records,'failures':failures},indent=2)+'\n')
assert not failures,failures
(out/'focused-verified.json').write_text(json.dumps({'source_bindings':bindings,'outcome':'passed','records':records,'fixture_provenance_sha256':sha(here/'fixture-provenance.json')},indent=2)+'\n')
units=assert_translation_sets(product,test)
full=[]
for ui in ['tv','touch']:
 directory=out/('objects-'+ui);directory.mkdir(exist_ok=True);ui_flags=['-DNV_TOUCH_UI=1'] if ui=='touch' else []
 sources=[product/relative for relative in units];jobs=[(i,path) for i,path in enumerate(sources)]
 def compile(job):
  i,path=job;name=f'{i:04d}-{path.stem}';obj=directory/(name+'.o');dep=directory/(name+'.d');cmd=[cc,*base,*ui_flags,'-I'+str(product/'src'),'-MMD','-MF',str(dep),'-c',str(path),'-o',str(obj)];z=subprocess.run(cmd,capture_output=True,text=True,env=env,timeout=900);(directory/(name+'.log')).write_text(z.stdout+z.stderr);assert not z.returncode,(path,z.stderr[-3000:]);return obj,dep,path,dependency_snapshot(dep,obj,path,product)
 with concurrent.futures.ThreadPoolExecutor(max_workers=3) as pool:objects=list(pool.map(compile,jobs))
 binary=out/('app-product-'+ui);records.append(execute('link-product-'+ui,[cc,*map(lambda x:str(x[0]),objects),*libs,'-o',str(binary)]));full.append({'layer':'product','ui':ui,'sha256':sha(binary),'bytes':binary.stat().st_size,'objects':len(objects),'source_hashes':{str(x.relative_to(product)):sha(x) for x in sources}})
 test_objects=[];compiled=[];reused=[]
 for i,(obj,dep,path,snapshot) in enumerate(objects):
  evidence=reuse_evidence(snapshot,product,test)
  if evidence['eligible']:
   test_objects.append(obj);reused.append({'object':obj.name,'reuse_evidence':evidence});continue
  target=directory/(f'{i:04d}-{path.stem}-private.o');cmd=[cc,*base,*ui_flags,'-DNV_TEST_LOCAL_LOGS=1','-I'+str(test/'src'),'-c',str(test/path.relative_to(product)),'-o',str(target)];records.append(execute('private-'+ui+'-'+path.stem,cmd));test_objects.append(target);compiled.append(str(path.relative_to(product)))
 assert {'src/ajustes.c','src/app.c','src/posplay.c'}<=set(compiled),compiled
 binary=out/('app-test-'+ui);records.append(execute('link-test-'+ui,[cc,*map(str,test_objects),*libs,'-o',str(binary)]));full.append({'layer':'test','ui':ui,'sha256':sha(binary),'bytes':binary.stat().st_size,'objects':len(test_objects),'new_private_objects':compiled,'translation_unit_sets_identical':True,'private_macro_delta':['-DNV_TEST_LOCAL_LOGS=1'],'same_invocation_dependency_bound_object_reuse':reused})

for source in [product,test]:assert not subprocess.check_output(['git','-C',str(source),'status','--porcelain'],text=True).strip()
receipt={'source_bindings':bindings,'outcome':'passed','scope':'Complete linked macOS TV/touch product and private-test apps, same-job exact-dependency object reuse only; focused actual C normal/ASan/UBSan plus513 dispatch/owner','compiler':cc,'compiler_version':subprocess.check_output([cc,'--version'],text=True).splitlines()[0],'sdk_sanity_passed':True,'full_apps':full,'records':records,'fixture_provenance_sha256':sha(here/'fixture-provenance.json')};(out/'native-verified.json').write_text(json.dumps(receipt,indent=2)+'\n');print('Passed complete four app links and focused native source gates')
