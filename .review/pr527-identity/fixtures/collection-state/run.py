"""Original raw-row #362 collection views and deliberately adapted original test."""
from pathlib import Path
import argparse,subprocess,json,re,os,hashlib,platform,shlex
p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--cc',default='cc');p.add_argument('--sanitize',action='store_true');p.add_argument('--private',action='store_true');a=p.parse_args();s=a.source.resolve();r=Path(__file__).resolve().parent;o=a.output.resolve();o.mkdir(parents=True,exist_ok=False);sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest();darwin=platform.system()=='Darwin';origin=json.loads((r/'fixture-provenance.json').read_text())
def git(*args):return subprocess.check_output(['git','-C',str(s),*args],text=True).strip()
head,tree=git('rev-parse','HEAD'),git('rev-parse','HEAD^{tree}');assert not git('status','--porcelain'),'Require clean committed source';assert sha(s/'tests/colecao_exibicao.c')==origin['current_product_test_sha256']
for name,entry in origin['fixtures'].items():assert sha(r/'fixtures'/name)==entry['current_sha256'],name+' fixture changed'
source_paths=[s/x for x in git('ls-files','src').splitlines()];recipe_paths=[x for x in r.rglob('*')if x.is_file()];initial={str(x.resolve()):sha(x)for x in source_paths+recipe_paths if x.is_file()}
discovery=(s/'src/descoberta.c').read_text();start=discovery.index('int desc_filme_visto(');end=discovery.index('\n}',start)+2;body=discovery[start:end]
producer=o/'actual-desc-filme-visto.c';producer.write_text('#include "catalogo.h"\n#include <string.h>\n'+body+'\n');initial[str(producer.resolve())]=sha(producer)
producer_origin={'source':'src/descoberta.c','source_sha256':sha(s/'src/descoberta.c'),'actual_function_sha256':hashlib.sha256(body.encode()).hexdigest(),'generated_sha256':sha(producer)}
flags=['-std=gnu11','-O1','-g1','-DNV_ANDROID=1','-DNV_TOUCH_UI=1','-DNV_PERFIL_SESSAO_FORTE=1','-Wno-deprecated-declarations','-I'+str(r/'sdk'),'-I'+str(s/'src'),'-I'+str(r/'fixtures'),'-include',str(r/'fixtures/sdk-ticks-compat.h'),'-DVERTUDO_SOURCE="'+str(s/'src/vertudo.c')+'"'];flags+=['-D_DARWIN_C_SOURCE=1','-DNV_GLES_NO_MAC=1']if darwin else['-ffunction-sections','-fdata-sections','-fno-pie']
if a.private:flags+=['-DNV_TEST_LOCAL_LOGS=1']
san=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-sanitize-recover=all']if a.sanitize else[];linkflags=['-Wl,-dead_strip']if darwin else['-Wl,--gc-sections','-no-pie'];env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1:print_stacktrace=1'}
groups={'views':('collection-views.c',['dados','layout','ponteiro','ctxlista','perfis','home','ajustes']),'adapted-original':('original-collection.c',['dados','layout','ponteiro','ctxlista','perfis','salvos','home','ajustes','fileiras','linguas','fonteregra','fontefalha','regexjs','js'])};plans=[];records=[]
# Preflight every authentic closure through the compiler and linker before any test runs.
for group,(fixture,modules)in groups.items():
 d=o/group;d.mkdir();commands=[];deps={};objects=[]
 def run(cmd,name):
  commands.append(cmd);z=subprocess.run(cmd,text=True,capture_output=True,env=env,timeout=240);(d/name).write_text(z.stdout+z.stderr);return z
 for i,module in enumerate([r/'fixtures'/fixture]+[s/'src'/(x+'.c')for x in modules]+[producer]):
  extra=['-Ddados_gravar=collection_real_dados_gravar']if module.name=='dados.c'else[]
  if group=='views'and module.name=='ajustes.c':extra+=['-D'+name+'=collection_real_'+name for name in ['ajustes_colecao_exibicao','ajustes_conteudo_x','ajustes_rail_largura_fixa','ajustes_tinta_foco','ajustes_tinta_foco2','ajustes_acento','ajustes_ocultar_filmes_vistos']]
  obj=d/(str(i)+'.o');dep=d/(str(i)+'.d');z=run([a.cc,*flags,*san,*extra,'-MMD','-MF',str(dep),'-c',str(module),'-o',str(obj)],str(i)+'-compile.log');assert not z.returncode,z.stdout+z.stderr
  for name in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
   f=Path(name).resolve();assert str(f)in initial and sha(f)==initial[str(f)],(str(f),'dependency changed during compile');deps[str(f)]=sha(f)
  objects.append(str(obj))
 link=[a.cc,*objects,*linkflags,*san,'-lm','-pthread','-o',str(d/'probe')];z=run(link,'initial-link.log');pattern=r'^\s*"([^"]+)", referenced from:\s*$'if darwin else r"undefined reference to [`']([^'`]+)['`]";names=sorted(set(re.findall(pattern,z.stderr,re.M)));assert not z.returncode or names,z.stderr;assert not any('perfis_sessao_'in name for name in names),'Real profile lifecycle must be linked'
 traps='#include <stdio.h>\n#include <stdlib.h>\n'
 for i,name in enumerate(names):
  assert re.fullmatch(r'[A-Za-z_][A-Za-z_0-9]*',name),name
  traps+=f'void never_{i}(void) __asm__({json.dumps(name)});void never_{i}(void){{fputs("UNEXPECTED dependency: {name}\\n",stderr);abort();}}\n'
 (d/'traps.c').write_text(traps);z=run([a.cc,*flags,*san,'-c',str(d/'traps.c'),'-o',str(d/'traps.o')],'traps-compile.log');assert not z.returncode,z.stderr;link.insert(1,str(d/'traps.o'));z=run(link,'link.log');assert not z.returncode,z.stderr
 plans.append({'group':group,'directory':d,'fixture':fixture,'commands':commands,'dependencies':deps,'unreached_abort_traps':names,'modules':modules});print('preflight',group,'compiled and linked',flush=True)
(o/'preflight.json').write_text(json.dumps({'passed':True,'source_commit':head,'source_tree':tree,'groups':[{'group':p['group'],'real_modules':p['modules'],'binary_sha256':sha(p['directory']/'probe')}for p in plans],'all_closures_compiled_and_linked_before_runs':True},indent=2)+'\n')
for plan in plans:
 group=plan['group'];d=plan['directory'];z=subprocess.run([str(d/'probe')],text=True,capture_output=True,env=env,timeout=240);(d/'run.log').write_text(z.stdout+z.stderr);assert not z.returncode,z.stdout+z.stderr;assert 'UNEXPECTED dependency:'not in z.stderr;expected='8287 assertions'if group=='views'else'colecao_exibicao: ok';assert expected in z.stdout,z.stdout
 for name,h in plan['dependencies'].items():assert sha(name)==h,(name,'dependency mutated after run')
 result={'passed':True,'source_commit':head,'source_tree':tree,'source_clean':True,'group':group,'private':a.private,'sanitizers':['address','undefined']if a.sanitize else[],'fixture_sha256':sha(r/'fixtures'/plan['fixture']),'assertions_preserved':True,'views_assertions':8287 if group=='views'else None,'original_assertion_sites_preserved':47 if group=='adapted-original'else None,'added_grid_assertion_sites':1 if group=='adapted-original'else None,'real_modules':plan['modules'],'actual_watched_producer_origin':producer_origin,'real_profile_lifecycle':'perfis.c compiled and linked with NV_PERFIL_SESSAO_FORTE=1','unreached_abort_traps':plan['unreached_abort_traps'],'abort_traps_fired':False,'dependency_sha256':plan['dependencies'],'commands':plan['commands'],'run_command':[str(d/'probe')],'stdout':z.stdout,'binary_sha256':sha(d/'probe'),'logs_sha256':{p.name:sha(p)for p in d.glob('*.log')}};(d/'receipt.json').write_text(json.dumps(result,indent=2)+'\n');records.append({'group':group,'receipt':str(d/'receipt.json'),'receipt_sha256':sha(d/'receipt.json')});print(group,'PASS',expected,flush=True)
assert git('rev-parse','HEAD')==head and git('rev-parse','HEAD^{tree}')==tree and not git('status','--porcelain')
for name,h in initial.items():assert sha(name)==h,(name,'source or recipe mutated')
final={'passed':True,'source_commit':head,'source_tree':tree,'source_parent':git('rev-parse','HEAD^'),'source_clean':True,'records':records,'fixture_provenance_sha256':sha(r/'fixture-provenance.json'),'recipe_sha256':{str(p.relative_to(r)):sha(p)for p in recipe_paths},'scope':'Actual watched-projection collection handlers and real storage/layout/pointer/context/profile/home/Settings/saved/fileiras/language/failure/rule producers; original typed offline catalog/text/GPU/SDL boundaries. All 8287 view assertions and 47 original sites plus added Grid site retained. Deliberately adapted original behavior, not a full unchanged original-script claim. Exact current desc_filme_visto predicate compiled; fixed raw/visible offline catalog boundary with filtering Off. Historical47originalassertsites are not a latest-upstream test-script claim. No profile test duplication, application/device build or network claim.'};(o/'receipt.json').write_text(json.dumps(final,indent=2)+'\n')
