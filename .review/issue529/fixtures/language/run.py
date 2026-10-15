"""Focused actual source-language/Polish/account-reset and original language suites."""
from pathlib import Path
import argparse,hashlib,json,os,platform,re,shlex,shutil,subprocess
p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--cc',default='cc');p.add_argument('--platform',choices=['tv','android-touch'],required=True);p.add_argument('--sanitize',action='store_true');a=p.parse_args()
R=Path(__file__).resolve().parent;S=a.source.resolve();O=a.output.resolve();O.mkdir(parents=True,exist_ok=False);darwin=platform.system()=='Darwin';cc=shutil.which(a.cc);assert cc
sha=lambda f:hashlib.sha256(Path(f).read_bytes()).hexdigest()
def git(*args):return subprocess.check_output(['git','-C',str(S),*args],text=True).strip()
commit,tree=git('rev-parse','HEAD'),git('rev-parse','HEAD^{tree}');assert not git('status','--porcelain')
flags=['-std=gnu11','-O1','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-unused-parameter','-Wno-misleading-indentation','-I'+str(R/'sdk'),'-I'+str(S/'src'),'-ffunction-sections','-fdata-sections']
flags+=['-D_DARWIN_C_SOURCE=1','-DNV_GLES_NO_MAC=1']if darwin else['-Wno-format-truncation']
if a.platform=='android-touch':flags+=['-DNV_ANDROID=1','-DNV_TOUCH_UI=1']
if a.sanitize:flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-sanitize-recover=all']+([]if darwin else['-fno-pie'])
src=S/'src';records=[]
groups={
 'chip':([R/'language_chip.c']+[src/f for f in ['linguas.c','stream_parse.c','js.c','badges.c']],['-DNV_LANGUAGE_FIX','-DNV_STREAMS_SOURCE="'+str(src/'streams.c')+'"']),
 'account-reset':([R/'account_reset.c',src/'linguas.c',src/'js.c'],['-DNV_AJUSTES_SOURCE="'+str(src/'ajustes.c')+'"']),
 'subtitles':([R/'subtitle_groups.c',src/'linguas.c'],['-DNV_ADDONS_SOURCE="'+str(src/'addons.c')+'"']),
 'original-languages':([S/'tests/linguas.c',src/'linguas.c'],[]),
 'original-automatic':([S/'tests/idiomaauto.c'],[]),
 'original-i18n':([S/'tests/idioma.c',src/'idioma.c'],[]),
}
initial={str(f.resolve()):sha(f)for root in [S/'src',S/'tests',R]for f in root.rglob('*')if f.is_file()}
for group,(files,defs)in groups.items():
 out=O/group;out.mkdir();commands=[];objects=[];dependencies={}
 # The accepted reset and full-i18n recipes did not turn unrelated whole-TU
 # warnings into errors. Preserve their compiler policy rather than adding it.
 group_flags=[f for f in flags if f not in ['-Wall','-Wextra','-Werror']]if group in ['account-reset','original-i18n']else flags
 for i,source in enumerate(files):
  obj=out/(str(i)+'.o');dep=out/(str(i)+'.d');cmd=[cc,*group_flags,*defs,'-MMD','-MF',str(dep),'-c',str(source),'-o',str(obj)]
  result=subprocess.run(cmd,capture_output=True,text=True,timeout=180);(out/(str(i)+'.compile.log')).write_text(result.stdout+result.stderr);commands.append(cmd);assert result.returncode==0,result.stderr[-4000:]
  text=dep.read_text().replace('\\\n',' ');assert ':'in text
  names=shlex.split(text.split(':',1)[1]);assert names and str(source.resolve())in [str(Path(n).resolve())for n in names]
  for name in names:
   f=Path(name).resolve();h=sha(f);assert initial[str(f)]==h,'Dependency changed while compiling';dependencies[str(f)]=h
  objects.append(str(obj))
 binary=out/'probe';cmd=[cc,*objects,'-Wl,-dead_strip'if darwin else'-Wl,--gc-sections','-pthread','-lm','-o',str(binary)]
 if a.sanitize:cmd+=['-fsanitize=address,undefined']+([]if darwin else['-no-pie'])
 result=subprocess.run(cmd,capture_output=True,text=True,timeout=60);(out/'link.log').write_text(result.stdout+result.stderr);commands.append(cmd);assert result.returncode==0,result.stderr[-4000:]
 result=subprocess.run([str(binary)],capture_output=True,text=True,timeout=60,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1:print_stacktrace=1'});(out/'run.log').write_text(result.stdout+result.stderr);assert result.returncode==0,result.stdout+result.stderr
 if group=='chip':assert 'language-chip cases=70 failures=0'in result.stdout
 if group=='account-reset':assert 'account-reset actual Settings + language policy cases=21'in result.stdout
 assert all(sha(f)==h for f,h in dependencies.items())
 records.append({'group':group,'passed':True,'commands':commands,'dependency_sha256':dependencies,'binary_sha256':sha(binary),'output':result.stdout,'logs_sha256':{str(f.relative_to(O)):sha(f)for f in out.glob('*.log')}})
 print(group,'PASS',flush=True)
assert commit==git('rev-parse','HEAD')and tree==git('rev-parse','HEAD^{tree}')and not git('status','--porcelain')
value={'outcome':'passed','source_commit':commit,'source_tree':tree,'source_clean':True,'platform':a.platform,'sanitizers':['address','undefined']if a.sanitize else[],'compiler':str(Path(cc).resolve()),'compiler_version':subprocess.check_output([cc,'--version'],text=True).splitlines()[0],'fixture_files_sha256':{str(f.relative_to(R)):sha(f)for f in R.rglob('*')if f.is_file()},'groups':records,'scope':'Full actual production streams/Settings/addon language paths plus unchanged original language/i18n fixtures. Typed SDK/text boundary; no playback-track probing, GL/device or backend execution claim.'}
(O/'receipt.json').write_text(json.dumps(value,indent=2)+'\n');print('PASS6focused source-language groups',flush=True)
