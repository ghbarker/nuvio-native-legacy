"""Portable producer for unchanged accepted #475 actual-C contracts."""
from pathlib import Path
import argparse, ctypes, hashlib, json, os, platform, re, shlex, shutil, subprocess, tempfile
p=argparse.ArgumentParser();p.add_argument('--source',required=True);p.add_argument('--output',required=True);p.add_argument('--cc',default='cc');p.add_argument('--platform',choices=['tv','mobile'],default='mobile');p.add_argument('--sanitize',action='store_true');p.add_argument('--private',action='store_true');p.add_argument('--group',choices=['all','player','secondary','ass','geometry'],default='all');p.add_argument('--libass-reference-runtime',help='Explicit local0.17.3runtime with matching pinned reference SDK; CI uses installed pkg-config SDK by default');a=p.parse_args()
R=Path(__file__).resolve().parent;S=Path(a.source).resolve();O=Path(a.output).resolve();O.mkdir(parents=True,exist_ok=False);darwin=platform.system()=='Darwin';cc=shutil.which(a.cc);assert cc,'Compiler missing';cc=str(Path(cc).resolve());sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
provenance=json.loads((R/'fixture-provenance.json').read_text())
for name,v in provenance['fixtures'].items():assert sha(R/name)==v['sha256'],'Packaged accepted assertion fixture changed: '+name
commit=subprocess.check_output(['git','rev-parse','HEAD'],cwd=S,text=True).strip();tree=subprocess.check_output(['git','rev-parse','HEAD^{tree}'],cwd=S,text=True).strip();assert subprocess.run(['git','diff','--quiet','HEAD','--','src'],cwd=S).returncode==0,'Tracked product C source changed'
paths=[S/x for x in subprocess.check_output(['git','ls-files','src'],cwd=S,text=True).splitlines()];paths+=[x for x in R.rglob('*')if x.is_file()];initial={str(x.resolve()):sha(x)for x in paths if x.is_file()};records=[]
base=['-std=gnu11','-O1','-g','-ffunction-sections','-fdata-sections','-I'+str(R/'sdk'),'-I'+str(S/'src'),'-include',str(R/'sdk-extra.h')]
base+=['-DNV_ANDROID=1','-DNV_TOUCH_UI=1']if a.platform=='mobile'else['-DNV_TPK=1']
if darwin:base+=['-DNV_GLES_NO_MAC=1']
if a.private:base+=['-DNV_TEST_LOCAL_LOGS=1']
if a.sanitize:base+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-sanitize-recover=all']+([]if darwin else['-fno-pie'])
libass=None
if a.group in ['all','ass']:
 if a.libass_reference_runtime:
  library=Path(a.libass_reference_runtime).resolve();assert library.is_file();handle=ctypes.CDLL(str(library));handle.ass_library_version.restype=ctypes.c_int;assert handle.ass_library_version()==0x01703000,'Reference SDK requires exact actual0.17.3runtime API version'
  include=R/'libass-reference';headers={str(x.resolve()):sha(x)for x in [include/'ass/ass.h',include/'ass/ass_types.h']};libdir=library.parent
  libass={'sdk_mode':'Explicit matching0.17.3reference SDK and actualruntimeAPI version','version':'0.17.3','runtime_api_version':'0x01703000','cflags':['-I'+str(include)],'libs':[str(library)],'headers_sha256':headers,'shared_library':{'path':str(library),'sha256':sha(library)},'font_sha256':sha(S/'deploy/app/fonts/InterDisplay-Regular.ttf')}
 else:
  def pkg(*args):return subprocess.check_output(['pkg-config',*args,'libass'],text=True).strip()
  include=Path(pkg('--variable=includedir'));libdir=Path(pkg('--variable=libdir'));headers={str(x.resolve()):sha(x)for x in [include/'ass/ass.h',include/'ass/ass_types.h']};candidates=sorted({str(x.resolve())for pat in(['libass*.dylib']if darwin else['libass.so*'])for x in libdir.glob(pat)if x.is_file()});assert len(candidates)==1,('Ambiguous actual shared libass',candidates)
  libass={'sdk_mode':'Actual installed pkg-config SDK/library pair','version':pkg('--modversion'),'cflags':shlex.split(pkg('--cflags')),'libs':shlex.split(pkg('--libs')),'headers_sha256':headers,'shared_library':{'path':candidates[0],'sha256':sha(candidates[0])},'font_sha256':sha(S/'deploy/app/fonts/InterDisplay-Regular.ttf')}
groups=['player','secondary','ass','palette','grid-font']if a.group=='all'else['palette','grid-font']if a.group=='geometry'else[a.group]
for group in groups:
 out=O/group;out.mkdir();fixture={'player':'player_outline.c','secondary':'secondary_outline.c','ass':'ass_outline.c','palette':'palette_bounds.c','grid-font':'grid_font_bounds.c'}[group];macro={'player':'PLAYER_SOURCE','secondary':'SECONDARY_SOURCE','ass':'ASS_SOURCE'}.get(group,'FAIXAS_SOURCE');module={'player':'player.c','secondary':'legendasui.c','ass':'assrender.c'}.get(group,'faixas.c');flags=base+['-D'+macro+'='+json.dumps(str(S/'src'/module))]
 files=[R/'fixtures'/fixture]
 if group=='secondary':files+=[S/'src/layout.c']
 if group in ['palette','grid-font']:files+=[S/'src/layout.c',S/'src/ponteiro.c']
 if group=='grid-font':files+=[S/'src/plrui.c',R/'plrui_grid_boundary.c']
 if group=='ass':flags+=['-DNV_ASS_LIBASS=1',*libass['cflags']]
 if group in ['player','secondary','ass']:files+=[R/'clock.c']
 objects=[];commands=[];closure={}
 for i,f in enumerate(files):
  obj=out/(str(i)+'.o');dep=out/(str(i)+'.d');tu_flags=['-D'+name+'=nv_fixture_unused_'+name for name in ['plrui_linha_foco','plrui_seg','plrui_dicas','plrui_decimal']]if group=='grid-font'and f==S/'src/plrui.c'else[];cmd=[cc,*flags,*tu_flags,'-MMD','-MF',str(dep),'-c',str(f),'-o',str(obj)];z=subprocess.run(cmd,capture_output=True,text=True,timeout=180);(out/(str(i)+'.compile.log')).write_text(z.stdout+z.stderr);assert z.returncode==0,z.stderr;commands.append(cmd);objects.append(str(obj))
  for raw in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
   q=Path(raw).resolve();digest=sha(q);assert str(q)in initial and initial[str(q)]==digest,'Dependency changed during compile: '+str(q);closure[str(q)]=digest
 link=[cc,*objects,'-Wl,-dead_strip'if darwin else'-Wl,--gc-sections','-pthread','-lm','-o',str(out/'probe')]
 if group=='ass':link+=libass['libs']+(['-Wl,-rpath,'+str(libdir)]if darwin else[])
 if a.sanitize:link+=['-fsanitize=address,undefined']+([]if darwin else['-no-pie'])
 z=subprocess.run(link,capture_output=True,text=True);traps=[]
 (out/'link.log').write_text(z.stdout+z.stderr);commands.append(link);assert z.returncode==0,z.stderr;cases=[]
 configs=([(1080,2340,1),(2340,1080,1),(1920,1080,0)]if a.platform=='mobile'else[(1920,1080,0)])if group in ['palette','grid-font']else[None]
 for config in configs:
  for ui in([1,1.2,1.3,1.5]if config and config[2]else[1,1.5]if config else[None]):
   with tempfile.TemporaryDirectory(prefix='nuvio-outline-')as data:
    argv=[str(out/'probe')]+([data]if group=='player'else[str(S/'deploy/app/fonts/InterDisplay-Regular.ttf')]if group=='ass'else[str(config[0]),str(config[1]),str(ui),str(config[2])]if config else[])
    z=subprocess.run(argv,capture_output=True,text=True,timeout=60,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1:print_stacktrace=1'})
   log=out/(str(len(cases))+'.run.log');log.write_text(z.stdout+z.stderr);assert z.returncode==0,z.stdout+z.stderr;assert 'UNEXPECTED dependency:'not in z.stderr;assert all(sha(q)==h for q,h in closure.items()),'Compiled dependency changed';cases.append({'argv':argv[1:],'returncode':0,'stdout':z.stdout,'stderr':z.stderr,'log_sha256':sha(log)})
 records.append({'group':group,'passed':True,'commands':commands,'dependency_sha256':closure,'binary_sha256':sha(out/'probe'),'unreached_abort_boundaries':traps,'cases':cases});print(group,'PASS',len(cases),'case(s)',flush=True)
logs={str(x.relative_to(O)):sha(x)for x in O.rglob('*.log')};recipe={str(x.relative_to(R)):sha(x)for x in R.rglob('*')if x.is_file()}
if libass:
 assert sha(libass['shared_library']['path'])==libass['shared_library']['sha256']
 assert all(sha(q)==h for q,h in libass['headers_sha256'].items())
 assert sha(S/'deploy/app/fonts/InterDisplay-Regular.ttf')==libass['font_sha256']
receipt={'logs_sha256':logs,'recipe_files_sha256':recipe,'passed':True,'source_commit':commit,'source_tree':tree,'source_pin':None,'platform':a.platform,'private':a.private,'sanitizers':['address','undefined']if a.sanitize else[],'compiler':cc,'compiler_version':subprocess.check_output([cc,'--version'],text=True).splitlines()[0],'fixture_provenance_sha256':sha(R/'fixture-provenance.json'),'libass':libass,'groups':records,'scope':'Unchanged accepted actual-C subtitle475 assertions; typed finite text/GFX/platform companions, actual layout/pointer/disk and actual host libass software frames. No GPU/physical Android/native backend claim.'};(O/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
