from pathlib import Path
import argparse,concurrent.futures,json,os,platform,subprocess,sys
from common import HERE,sha,source_binding,record
p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);p.add_argument('--proof',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();s=a.source.resolve();o=a.output.resolve();o.mkdir(parents=True,exist_ok=True)
plan,v,m=source_binding(s,a.proof);assert platform.system()=='Darwin';cc='/usr/bin/cc';prefix=Path(subprocess.check_output(['brew','--prefix'],text=True).strip());rows=[];contracts=[];fixture=HERE/'fixtures/watched'
base=['-std=gnu11','-O1','-g','-Wno-deprecated-declarations','-I'+str(prefix/'include'),'-I'+str(prefix/'include/SDL2')]
base+=['-D'+name+'=""'for name in ['NV_SUPABASE_URL','NV_SUPABASE_ANON_KEY','NV_TV_LOGIN_BASE','NV_TRAKT_CLIENT_ID','NV_TRAKT_CLIENT_SECRET','NV_SIMKL_CLIENT_ID','NV_SIMKL_APP','NV_TMDB_API_KEY']]
libs=['-L'+str(prefix/'lib'),'-Wl,-rpath,'+str(prefix/'lib'),'-lSDL2','-lSDL2_image','-lSDL2_ttf','-lz','-framework','OpenGL']
sanity=o/'sdk.c';sanity.write_text('#include <stdio.h>\n#include <AvailabilityMacros.h>\n#include <SDL2/SDL.h>\n#include <OpenGL/gl3.h>\nint main(void){puts("SDK ready");return 0;}\n')
rows.append(record(o,'sdk-compile',[cc,*base,str(sanity),*libs,'-o',str(o/'sdk')],timeout=60));rows.append(record(o,'sdk-run',[str(o/'sdk')],timeout=20))
rows.append(record(o,'languages',[sys.executable,str(s/'tools/idiomas.py')]))
rows.append(record(o,'source-invariants',[sys.executable,str(fixture/'verify_source.py'),'--root',str(s)]))
expected=['discovery','colfiltro-existing','history-existing','homepos-existing','home-tv','home-touch','catalog-ui-tv','catalog-ui-touch','settings-tv','settings-touch']
for sanitized in [False,True]:
 mode='asan'if sanitized else'normal';directory=o/('watched-'+mode)
 env=dict(os.environ,NUVIO_WATCHED_REVIEW=str(fixture),NUVIO_WATCHED_RESULTS=str(directory),SANITIZE='1'if sanitized else'0')
 rows.append(record(o,'watched-'+mode,['/bin/bash','-x',str(fixture/'run_ci.sh')],cwd=s,env=env))
 logs={label:sha(directory/(label+'.log'))for label in expected};assert all((directory/(label+'.log')).stat().st_size for label in expected)
 contracts.append({'group':'watched','mode':mode,'directory':directory.name,'logs_sha256':logs,'suites':expected})
 directory=o/('extra-'+mode)
 rows.append(record(o,'extra-'+mode,[sys.executable,str(fixture/'run_extra.py'),'--source',str(s),'--output',str(directory)]+(['--sanitize']if sanitized else[])))
 q=json.loads((directory/'verified.json').read_text());assert q['source_commit']==m['head']and q['sanitized']==sanitized and q['sdk_mode']=='real macOS/SDL SDK'
 assert {x['name']for x in q['records']}=={'profile-session','all-eight-tv','all-eight-touch'}and len(q['records'])==3
 for r in q['records']:
  assert not r['expected_baseline_failure']and r['returncode']==0
  assert r['passed_lines']=={'profile-session':1,'all-eight-tv':8,'all-eight-touch':16}[r['name']]
 contracts.append({'group':'extra','mode':mode,'directory':directory.name,'receipt_sha256':sha(directory/'verified.json')})
units=sorted((s/'src').glob('*.c'))+sorted((s/'src/dts').glob('*.c'));apps=[]
assert {s/'src/vertudo.c',s/'src/descoberta.c',s/'src/ajustes.c',s/'src/main.c'}<=set(units)
for ui in ['tv','touch']:
 d=o/('objects-'+ui);d.mkdir();flags=base+(['-DNV_TOUCH_UI=1']if ui=='touch'else[])
 def compile_one(item):
  i,path=item;obj=d/(str(i)+'-'+path.stem+'.o');label=str(i)+'-'+path.stem
  r=record(d,label,[cc,*flags,'-I'+str(s/'src'),'-c',str(path),'-o',str(obj)])
  return str(obj),r,path.name
 with concurrent.futures.ThreadPoolExecutor(max_workers=3)as pool:objects=list(pool.map(compile_one,enumerate(units)))
 binary=o/('app-'+ui);rows.append(record(o,'link-'+ui,[cc,*[x[0]for x in objects],*libs,'-o',str(binary)]))
 apps.append({'ui':ui,'sha256':sha(binary),'bytes':binary.stat().st_size,'translation_units':len(units),'compile_records':[x[1]for x in objects]})
 if ui=='tv':
  # The unchanged geometry fixture includes vertudo.c. Reuse the exact TV objects
  # for every other production module instead of compiling the app a second time.
  obj=o/'geometry.o';rows.append(record(o,'geometry-compile',[cc,*flags,'-I'+str(s/'src'),'-c',str(fixture/'tests/colecao_exibicao_compat.c'),'-o',str(obj)]))
  binary=o/'geometry';rows.append(record(o,'geometry-link',[cc,*[x[0]for x in objects if x[2]not in ['main.c','vertudo.c']],str(obj),*libs,'-o',str(binary)]))
  rows.append(record(o,'geometry-run',[str(binary)],cwd=s,timeout=60));assert 'colecao_exibicao: ok'in(o/'geometry-run.log').read_text()
source_binding(s,a.proof)
result={'outcome':'passed','source_binding':v,'plan_sha256':sha(HERE/'plan.json'),'gates':rows,'contracts':contracts,'full_apps':apps,'fixture_files_sha256':{str(x.relative_to(fixture)):sha(x)for x in sorted(fixture.rglob('*'))if x.is_file()and'__pycache__'not in x.parts},'compiler':subprocess.check_output([cc,'--version'],text=True).splitlines()[0],'scope':'Fresh full macOS TV/touch links; existing ten watched/discovery/history/Home/Settings suites and three profile-epoch/all-eight suites in normal + ASan/UBSan; original real-source geometry contract using exact TV objects; translation alignment. No private overlay or device/rendering claim.'}
(o/'native-verified.json').write_text(json.dumps(result,indent=2)+'\n');print('PASS exact-source watched native validation',flush=True)
