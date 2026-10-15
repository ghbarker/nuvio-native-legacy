"""Frozen original #362 Settings UI and profile producers against an exact source."""
from pathlib import Path
import argparse,subprocess,json,re,os,hashlib,platform,shlex
p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--cc',default='cc');p.add_argument('--group',choices=['all','ui','profile'],default='all');p.add_argument('--platform',choices=['all','mobile','tv'],default='all');p.add_argument('--sanitize',action='store_true');p.add_argument('--private',action='store_true');a=p.parse_args()
s=a.source.resolve();r=Path(__file__).resolve().parent;o=a.output.resolve();o.mkdir(parents=True,exist_ok=False);sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest();darwin=platform.system()=='Darwin';origin=json.loads((r/'fixture-provenance.json').read_text())
def git(*args):return subprocess.check_output(['git','-C',str(s),*args],text=True).strip()
head,tree=git('rev-parse','HEAD'),git('rev-parse','HEAD^{tree}');assert not git('status','--porcelain'),'Require clean committed source'
for name,file in [('ui','collection-choice.c'),('profile','profile-sync.c')]:assert sha(r/'fixtures'/file)==origin[name]['sha256'],name+' assertions changed'
for name,h in origin['sdk_sha256'].items():assert sha(r/name)==h,name
source_paths=[s/x for x in git('ls-files','src').splitlines()];recipe_paths=[x for x in r.rglob('*')if x.is_file()];initial={str(x.resolve()):sha(x)for x in source_paths+recipe_paths if x.is_file()}
flags=['-std=gnu11','-O1','-g1','-DNV_ANDROID=1','-DNV_TOUCH_UI=1','-DNV_PERFIL_SESSAO_FORTE=1','-I'+str(r/'sdk'),'-I'+str(s/'src'),'-DSETTINGS_SOURCE="'+str(s/'src/ajustes.c')+'"']
flags+=['-D_DARWIN_C_SOURCE=1','-DNV_GLES_NO_MAC=1']if darwin else['-ffunction-sections','-fdata-sections','-fno-pie']
if a.private:flags+=['-DNV_TEST_LOCAL_LOGS=1']
san=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-sanitize-recover=all']if a.sanitize else[];linkflags=['-Wl,-dead_strip']if darwin else['-Wl,--gc-sections','-no-pie'];env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1:print_stacktrace=1'}
groups={'ui':('collection-choice.c',['layout','ponteiro','fileiras','dados','linguas','fontefalha','fonteregra','regexjs','js','perfis']),'profile':('profile-sync.c',['dados','layout','linguas','fontefalha','fonteregra','regexjs','js','sync','perfis'])};records=[]
for group,(fixture,modules)in groups.items():
 if a.group!='all'and group!=a.group:continue
 d=o/group;d.mkdir();commands=[];deps={};objects=[]
 def run(cmd,name):
  commands.append(cmd);z=subprocess.run(cmd,text=True,capture_output=True,env=env,timeout=240);(d/name).write_text(z.stdout+z.stderr);return z
 for i,module in enumerate([r/'fixtures'/fixture]+([r/'offline-account.c']if group=='ui'else[])+[s/'src'/(x+'.c')for x in modules]):
  extra=['-DPUBLISHED493_DEFAULTS=1']if group=='profile'else[]
  if module.name=='perfis.c':extra+=['-D'+name+'=collection_real_'+name for name in ['perfis_ativo','perfis_n','perfis_item','perfis_ativo_addons']]
  obj=d/(str(i)+'.o');dep=d/(str(i)+'.d');z=run([a.cc,*flags,*san,*extra,'-MMD','-MF',str(dep),'-c',str(module),'-o',str(obj)],str(i)+'-compile.log')
  assert not z.returncode,z.stdout+z.stderr
  for name in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
   f=Path(name).resolve();assert str(f)in initial and sha(f)==initial[str(f)],(str(f),'dependency changed during compile');deps[str(f)]=sha(f)
  objects.append(str(obj))
 link=[a.cc,*objects,*linkflags,*san,'-lm','-pthread','-o',str(d/'probe')];z=run(link,'initial-link.log');pattern=r'^\s*"([^"]+)", referenced from:\s*$'if darwin else r"undefined reference to [`']([^'`]+)['`]";names=sorted(set(re.findall(pattern,z.stderr,re.M)));assert not z.returncode or names,z.stderr;assert not any('perfis_sessao_'in name for name in names),'Real lifecycle missing'
 traps='#include <stdio.h>\n#include <stdlib.h>\n'
 for i,name in enumerate(names):
  assert re.fullmatch(r'[A-Za-z_][A-Za-z_0-9]*',name),name
  traps+=f'void never_{i}(void) __asm__({json.dumps(name)});void never_{i}(void){{fputs("UNEXPECTED dependency: {name}\\n",stderr);abort();}}\n'
 (d/'traps.c').write_text(traps);z=run([a.cc,*flags,*san,'-c',str(d/'traps.c'),'-o',str(d/'traps.o')],'traps-compile.log');assert not z.returncode,z.stderr;link.insert(1,str(d/'traps.o'));z=run(link,'link.log');assert not z.returncode,z.stderr
 cases=[]
 if group=='profile':case_args=[[]]
 else:
  case_args=[[str(portrait),str(size),str(ui),str(mobile)]for mobile in([1,0]if a.platform=='all'else[1 if a.platform=='mobile'else 0])for portrait in([1,0]if mobile else[0])for size in[0,1,2]for ui in[1,1.2,1.3,1.5]]
 for args in case_args:
  z=run([str(d/'probe'),*args],('run-'+('-'.join(args)if args else'profile')+'.log'));assert not z.returncode,z.stdout+z.stderr;assert 'UNEXPECTED dependency:'not in z.stderr
  expected='133 assertions, 17 profile changes'if group=='profile'else'PASS global8choice checks=235';assert expected in z.stdout,(group,z.stdout)
  cases.append({'args':args,'returncode':0,'stdout_last':z.stdout.splitlines()[-1]})
 for name,h in deps.items():assert sha(name)==h,(name,'dependency changed after run')
 result={'passed':True,'source_commit':head,'source_tree':tree,'source_clean':True,'group':group,'private':a.private,'sanitizers':['address','undefined']if a.sanitize else[],'fixture_sha256':sha(r/'fixtures'/fixture),'fixture_assertions_unchanged':True,'real_profile_lifecycle':'perfis.c linked with NV_PERFIL_SESSAO_FORTE=1; only unrelated identity/list getters renamed within that object to coexist with fixed offline fixture identities','cases':cases,'unreached_abort_traps':names,'abort_traps_fired':False,'dependency_sha256':deps,'commands':commands,'binary_sha256':sha(d/'probe'),'logs_sha256':{x.name:sha(x)for x in d.glob('*.log')}};(d/'receipt.json').write_text(json.dumps(result,indent=2)+'\n');records.append({'group':group,'case_count':len(cases),'receipt':str(d/'receipt.json'),'receipt_sha256':sha(d/'receipt.json')});print(group,len(cases),'PASS',flush=True)
assert git('rev-parse','HEAD')==head and git('rev-parse','HEAD^{tree}')==tree and not git('status','--porcelain')
for name,h in initial.items():assert sha(name)==h,(name,'source or recipe mutated')
final={'passed':True,'source_commit':head,'source_tree':tree,'source_parent':git('rev-parse','HEAD^'),'source_clean':True,'records':records,'fixture_provenance_sha256':sha(r/'fixture-provenance.json'),'recipe_sha256':{str(x.relative_to(r)):sha(x)for x in recipe_paths},'scope':origin['scope']};(o/'receipt.json').write_text(json.dumps(final,indent=2)+'\n')
