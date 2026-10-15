#!/usr/bin/env python3
"""Actual collection/profile/original-adapter contracts; local headless SDK boundaries."""
from pathlib import Path
import argparse,hashlib,json,os,platform,re,subprocess,sys
p=argparse.ArgumentParser()
for name in ('source','output'):p.add_argument('--'+name,type=Path,required=True)
p.add_argument('--cc',required=True);p.add_argument('--sanitize',action='store_true');p.add_argument('--private',action='store_true');a=p.parse_args()
here=Path(__file__).resolve().parent;s=a.source.resolve();out=a.output.resolve();assert not out.exists();out.mkdir(parents=True)
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
def git(*args):return subprocess.check_output(['git','-C',str(s),*args],text=True).strip()
commit,tree=git('rev-parse','HEAD'),git('rev-parse','HEAD^{tree}');assert not git('status','--porcelain')
assert 'ajustes_perfil_padrao();\n  ajustes_perfil_restaurar(depois);' in (s/'src/sync.c').read_text(), 'Require published493 independent defaults path'
assert 'int principal = perfilPrincipal()' not in (s/'src/sync.c').read_text()
origin=json.loads((here/'fixture-origin.json').read_text());assert sha(s/'tests/colecao_exibicao.c')==origin['original_fixture_sha256']
for collection in ['current_fixture_sha256','current_sdk_sha256']:
 for name,expected in origin[collection].items():assert sha(here/name)==expected,name
darwin=platform.system()=='Darwin';flags=['-std=gnu11','-O1','-g','-Wno-deprecated-declarations','-I'+str(here/'sdk'),'-I'+str(s/'src'),'-I'+str(here/'fixtures'),'-include',str(here/'fixtures/sdk-ticks-compat.h'),'-DNV_ANDROID=1','-DNV_TOUCH_UI=1']
flags+=['-DPLAYER_SOURCE='+json.dumps(str(s/'src/player.c'))]
flags+=['-D_DARWIN_C_SOURCE=1'] if darwin else ['-ffunction-sections','-fdata-sections','-fno-pie']
if a.private:flags+=['-DNV_TEST_LOCAL_LOGS=1']
san=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-sanitize-recover=all']if a.sanitize else[]
linkflags=['-Wl,-dead_strip']if darwin else['-Wl,--gc-sections','-no-pie']
env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1:print_stacktrace=1'}
groups={'profile':('profile-sync.c',['legenda2','dados','layout','linguas','fontefalha','fonteregra','regexjs','js','sync']),'views':('collection-views.c',['legenda2','dados','layout','ponteiro','ctxlista','perfis','home','ajustes']),'adapted-original':('original-collection.c',['legenda2','dados','layout','ponteiro','ctxlista','perfis','salvos','home','ajustes','fileiras','linguas','fonteregra','fontefalha','regexjs','js'])}
records=[]
producer=out/'actual-desc-filme-visto.c';discovery=(s/'src/descoberta.c').read_text();start=discovery.index('int desc_filme_visto(');end=discovery.index('\n}',start)+2;body=discovery[start:end];producer.write_text('#include "catalogo.h"\n#include <string.h>\n'+body+'\n');producer_origin={'source':'src/descoberta.c','source_sha256':sha(s/'src/descoberta.c'),'actual_function_sha256':hashlib.sha256(body.encode()).hexdigest(),'generated_sha256':sha(producer)}
def invoke(cmd,log):
 z=subprocess.run(cmd,text=True,capture_output=True,env=env,timeout=240);log.write_text(z.stdout+z.stderr)
 assert z.returncode==0,(str(log),z.returncode,(z.stdout+z.stderr)[-3000:]);return z
for group,(fixture,modules)in groups.items():
 d=out/group;d.mkdir();commands=[];objects=[];dependencies={}
 sources=[here/'fixtures'/fixture,here/'player-prefs-companion.c']+[s/'src'/(m+'.c')for m in modules]+([producer]if group!='profile'else[])
 for i,module in enumerate(sources):
  obj=d/(str(i)+'.o');dep=d/(str(i)+'.d');extra=[]
  if group=='profile':extra+=['-DPUBLISHED493_DEFAULTS=1','-DSETTINGS_SOURCE="'+str(s/'src/ajustes.c')+'"']
  else:extra+=['-DVERTUDO_SOURCE="'+str(s/'src/vertudo.c')+'"']
  if group!='profile'and module==s/'src/dados.c':extra+=['-Ddados_gravar=collection_real_dados_gravar']
  if group=='views'and module==s/'src/ajustes.c':
   # Preserve every existing controlled Settings boundary in this fixture;
   # link the remaining real Settings geometry getters needed by actual home.c.
   for name in ['ajustes_colecao_exibicao','ajustes_conteudo_x','ajustes_rail_largura_fixa','ajustes_tinta_foco','ajustes_tinta_foco2','ajustes_acento','ajustes_ocultar_filmes_vistos']:
    extra+=['-D'+name+'=collection_real_'+name]
  cmd=[a.cc,*flags,*san,*extra,'-MMD','-MF',str(dep),'-c',str(module),'-o',str(obj)];invoke(cmd,d/(str(i)+'-compile.log'));commands.append(cmd);objects.append(str(obj))
  raw=dep.read_text().replace('\\\n',' ');assert ':'in raw;paths=raw.split(':',1)[1].split();assert paths
  for f in paths:
   path=Path(f);assert path.is_file(),f;dependencies[str(path.resolve())]=sha(path)
 binary=d/'probe';link=[a.cc,*objects,*linkflags,*san,'-lm','-pthread','-o',str(binary)]
 first=subprocess.run(link,text=True,capture_output=True,env=env,timeout=240);(d/'initial-link.log').write_text(first.stdout+first.stderr)
 pattern=r'^\s*"([^"]+)", referenced from:\s*$'if darwin else r"undefined reference to [`']([^'`]+)['`]"
 names=sorted(set(re.findall(pattern,first.stderr,re.M)));assert not first.returncode or names,(group,first.stderr)
 assert all(re.fullmatch(r'[A-Za-z_][A-Za-z_0-9]*',n)for n in names)
 trap='#include <stdio.h>\n#include <stdlib.h>\n'
 for i,name in enumerate(names):trap+=f'void never_{i}(void) __asm__({json.dumps(name)});\nvoid never_{i}(void){{fputs("UNEXPECTED dependency: {name}\\n",stderr);abort();}}\n'
 (d/'traps.c').write_text(trap);cmd=[a.cc,*flags,*san,'-c',str(d/'traps.c'),'-o',str(d/'traps.o')];invoke(cmd,d/'traps-compile.log');commands.append(cmd);link.insert(1,str(d/'traps.o'));invoke(link,d/'link.log');commands.append(link)
 z=invoke([str(binary)],d/'run.log');assert 'UNEXPECTED dependency:'not in z.stderr
 expected='133 assertions, 17 profile changes'if group=='profile'else'8035 assertions'if group=='views'else'colecao_exibicao: ok';assert expected in z.stdout,(group,z.stdout)
 for f,h in dependencies.items():assert sha(f)==h,(group,f,'dependency mutated')
 receipt={'outcome':'passed','source_commit':commit,'tree':tree,'group':group,'private':a.private,'sanitized':a.sanitize,'commands':commands,'run_command':[str(binary)],'binary_sha256':sha(binary),'dependency_sha256':dependencies,'unreached_abort_traps':names,'abort_traps_fired':False,'logs':{f.name:sha(f)for f in d.glob('*.log')}}
 f=d/'receipt.json';f.write_text(json.dumps(receipt,indent=2)+'\n');records.append({'group':group,'receipt':str(f.relative_to(out)),'receipt_sha256':sha(f)});print(group,'PASS',flush=True)
assert git('rev-parse','HEAD')==commit and git('rev-parse','HEAD^{tree}')==tree and not git('status','--porcelain')
r={'outcome':'passed','source_commit':commit,'tree':tree,'private':a.private,'sanitized':a.sanitize,'records':records,'actual_producer_origin':producer_origin,'fixture_origin_sha256':sha(here/'fixture-origin.json'),'tooling_sha256':{str(f.relative_to(here)):sha(f)for f in here.rglob('*')if f.is_file()and'__pycache__'not in str(f)},'scope':'Actual complete collection/profile handlers and real local storage/home/Settings producers; typed offline account/catalog/text/GPU/SDL boundaries. Includes deliberately adapted original behavior, not an unchanged/full-source original-script claim. No app/SDK/device/network build or execution claim.'};(out/'receipt.json').write_text(json.dumps(r,indent=2)+'\n')
