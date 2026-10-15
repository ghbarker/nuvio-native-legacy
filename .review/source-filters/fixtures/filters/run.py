"""Replay existing source-exclusion contracts against one exact clean source."""
from pathlib import Path
import argparse,hashlib,json,os,platform,shlex,shutil,subprocess,tempfile
p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--cc',default='cc');p.add_argument('--sanitize',action='store_true');a=p.parse_args()
R=Path(__file__).resolve().parent;S=a.source.resolve();O=a.output.resolve();O.mkdir(parents=True,exist_ok=False);darwin=platform.system()=='Darwin';cc=shutil.which(a.cc);assert cc
sha=lambda f:hashlib.sha256(Path(f).read_bytes()).hexdigest()
def git(*args):return subprocess.check_output(['git','-C',str(S),*args],text=True).strip()
commit,tree=git('rev-parse','HEAD'),git('rev-parse','HEAD^{tree}');assert not git('status','--porcelain')
ui=O/'current-ui.c';subprocess.run(['python3',str(R/'prepare_ui.py'),str(S),str(ui)],check=True,capture_output=True,text=True)
src=S/'src';parser=[src/n for n in ['stream_parse.c','fontefiltro.c','js.c','badges.c']]
groups={
 'original-parser':([S/'tests/stream_parser.c',*parser],False),
 'original-planner':([S/'tests/fontefalha.c',src/'fontefalha.c',src/'fontefiltro.c'],True),
 'core':([R/'reviewed-fontefiltro.c',src/'fontefiltro.c'],False),
 'parser':([R/'reviewed-fontefiltro_parse.c',*parser],False),
 'planner':([R/'reviewed-fontefalha.c',src/'fontefalha.c',src/'fontefiltro.c'],True),
 'worker':([R/'reviewed-fontefalha_worker.c',src/'fontefalha.c',src/'fontefiltro.c',src/'fonteauto.c',src/'perfis.c'],True),
 'ui-handlers':([ui,src/'fontefalha.c',src/'fontefiltro.c'],True),
 'preparation':([R/'preparation_filters.c',src/'fontefalha.c',src/'fontefiltro.c',src/'fonteauto.c',src/'perfis.c'],True),
 'concurrency':([R/'config_concurrency.c',src/'fontefalha.c',src/'fontefiltro.c'],True),
 'profile-bank':([R/'profile-bank-filters.c',src/'fontefalha.c',src/'fontefiltro.c'],False),
 'profile-scope':([R/'profile-scope-filters.c',src/'debrid.c',src/'fontefalha.c',src/'fontefiltro.c'],False),
}
flags=['-std=gnu11','-O1','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-unused-parameter','-Wno-unused-but-set-variable','-Wno-misleading-indentation','-I'+str(R/'sdk'),'-I'+str(src),'-DNV_STREAMS_SOURCE="'+str(src/'streams.c')+'"','-ffunction-sections','-fdata-sections']
flags+=['-D_DARWIN_C_SOURCE=1','-DNV_GLES_NO_MAC=1']if darwin else['-Wno-format-truncation','-Wno-restrict']
if a.sanitize:flags+=['-fsanitize=address,undefined,float-cast-overflow','-fno-omit-frame-pointer','-fno-sanitize-recover=all']+([]if darwin else['-fno-pie'])
initial={str(f.resolve()):sha(f)for root in [S/'src',S/'tests',R]for f in root.rglob('*')if f.is_file()};initial[str(ui)]=sha(ui);records=[]
for group,(files,data)in groups.items():
 out=O/group;out.mkdir();commands=[];objects=[];dependencies={}
 for i,source in enumerate(files):
  obj=out/(str(i)+'.o');dep=out/(str(i)+'.d');cmd=[cc,*flags,'-MMD','-MF',str(dep),'-c',str(source),'-o',str(obj)];z=subprocess.run(cmd,capture_output=True,text=True,timeout=180);(out/(str(i)+'.compile.log')).write_text(z.stdout+z.stderr);commands.append(cmd);assert z.returncode==0,z.stderr[-4000:]
  text=dep.read_text().replace('\\\n',' ');assert ':'in text;names=shlex.split(text.split(':',1)[1]);assert names and str(source.resolve())in[str(Path(n).resolve())for n in names]
  for name in names:
   f=Path(name).resolve();h=sha(f);assert initial[str(f)]==h;dependencies[str(f)]=h
  objects.append(str(obj))
 binary=out/'probe';cmd=[cc,*objects,'-Wl,-dead_strip'if darwin else'-Wl,--gc-sections','-pthread','-lm','-o',str(binary)]+(['-fsanitize=address,undefined,float-cast-overflow']+([]if darwin else['-no-pie'])if a.sanitize else[])
 z=subprocess.run(cmd,capture_output=True,text=True,timeout=60);(out/'link.log').write_text(z.stdout+z.stderr);commands.append(cmd);assert z.returncode==0,z.stderr[-4000:]
 with tempfile.TemporaryDirectory(prefix='source-filter-contract-')as directory:
  z=subprocess.run([str(binary)]+([directory]if data else[]),capture_output=True,text=True,timeout=60,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1:print_stacktrace=1'})
 (out/'run.log').write_text(z.stdout+z.stderr);assert z.returncode==0,z.stdout+z.stderr;assert all(sha(f)==h for f,h in dependencies.items())
 records.append({'group':group,'passed':True,'commands':commands,'dependency_sha256':dependencies,'binary_sha256':sha(binary),'output':z.stdout,'logs_sha256':{str(f.relative_to(O)):sha(f)for f in out.glob('*.log')}});print(group,'PASS',flush=True)
assert commit==git('rev-parse','HEAD')and tree==git('rev-parse','HEAD^{tree}')and not git('status','--porcelain')
value={'outcome':'passed','source_commit':commit,'source_tree':tree,'source_clean':True,'sanitizers':['address','undefined','float-cast-overflow']if a.sanitize else[],'compiler':str(Path(cc).resolve()),'compiler_version':subprocess.check_output([cc,'--version'],text=True).splitlines()[0],'fixture_files_sha256':{str(f.relative_to(R)):sha(f)for f in R.rglob('*')if f.is_file()},'groups':records,'generated_ui_sha256':sha(ui),'generated_ui_manifest_sha256':sha(ui.with_suffix('.manifest.json')),'scope':'Existing actual parser/filter/planner/worker/preparation/profile contracts and exact extracted Settings handler bodies with typed offline SDK/backend boundaries. No full UI, network, playback or device execution claim.'};(O/'receipt.json').write_text(json.dumps(value,indent=2)+'\n');print('PASS11existing source-filter contracts',flush=True)
