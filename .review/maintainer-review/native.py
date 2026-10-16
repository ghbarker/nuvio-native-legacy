from pathlib import Path
import argparse,concurrent.futures,json,os,platform,shlex,subprocess,sys
from common import HERE,sha,git,source_binding,record
p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);p.add_argument('--proof',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();s=a.source.resolve();o=a.output.resolve();o.mkdir(parents=True,exist_ok=True)
plan,v,m=source_binding(s,a.proof);assert platform.system()=='Darwin';cc='/usr/bin/cc';prefix=Path(subprocess.check_output(['brew','--prefix'],text=True).strip());rows=[];contracts=[]
base=['-std=gnu11','-O1','-g','-Wno-deprecated-declarations','-I'+str(prefix/'include'),'-I'+str(prefix/'include/SDL2')]
base+=['-D'+name+'=""'for name in ['NV_SUPABASE_URL','NV_SUPABASE_ANON_KEY','NV_TV_LOGIN_BASE','NV_TRAKT_CLIENT_ID','NV_TRAKT_CLIENT_SECRET','NV_SIMKL_CLIENT_ID','NV_SIMKL_APP','NV_TMDB_API_KEY','NV_SEEKR_API_KEY','NV_REC_URL','NV_DISCORD_CLIENT_ID']]
base+=['-DNV_VERSAO='+json.dumps(json.loads((s/'deploy/app/appinfo.json').read_text())['version'])]
libs=['-L'+str(prefix/'lib'),'-Wl,-rpath,'+str(prefix/'lib'),'-lSDL2','-lSDL2_image','-lSDL2_ttf','-lz','-framework','OpenGL']
# libass must be a real compiled backend, never the no-backend branch.
font_rows=m['runtime_font_inputs'];assert font_rows, 'Frozen runtime font inputs are required'
def bind_runtime_fonts():
 for e in font_rows:
  assert e['path'].startswith('deploy/app/fonts/') and e['path'].endswith(('.ttf','.otf'))
  assert git(s,'rev-parse','HEAD:'+e['path'])==e['blob'] and sha(s/e['path'])==e['sha256'], e['path']
bind_runtime_fonts()
for label,args in [('font-libraries-exist',['--exists','sdl2','SDL2_ttf','libass']),
                   ('font-library-versions',['--modversion','sdl2','SDL2_ttf','libass']),
                   ('libass-cflags',['--cflags','libass']),('libass-libs',['--libs','libass'])]:
 rows.append(record(o,label,['pkg-config',*args],timeout=60))
ass_cflags=shlex.split((o/'libass-cflags.log').read_text().strip())
ass_libs=shlex.split((o/'libass-libs.log').read_text().strip())
assert '-lass' in ass_libs, 'Actual libass link flag missing from pkg-config'
base+=['-DNV_ASS_LIBASS',*ass_cflags];libs+=ass_libs
font_backend={'enabled':True,'define':'-DNV_ASS_LIBASS','pkg_config_cflags':ass_cflags,'pkg_config_libs':ass_libs,
              'runtime_font_inputs':font_rows,'versions_log_sha256':sha(o/'font-library-versions.log')}
sanity=o/'sdk.c';sanity.write_text('#include <stdio.h>\n#include <AvailabilityMacros.h>\n#include <SDL2/SDL.h>\n#include <SDL2/SDL_ttf.h>\n#include <ass/ass.h>\n#include <OpenGL/gl3.h>\nint main(void){const SDL_version *v=TTF_Linked_Version();if(!v||!ass_library_version()||TTF_Init())return 1;ASS_Library *a=ass_library_init();if(!a)return 2;ass_library_done(a);TTF_Quit();puts("SDK ready: actual SDL_ttf and libass");return 0;}\n')
rows.append(record(o,'sdk-compile',[cc,*base,str(sanity),*libs,'-o',str(o/'sdk')],timeout=60));rows.append(record(o,'sdk-run',[str(o/'sdk')],timeout=20))
rows.append(record(o,'languages',[sys.executable,str(s/'tools/idiomas.py')]))
rows.append(record(o,'public-tests-normal-asan',[sys.executable,str(HERE/'public_tests.py'),'--source',str(s),'--proof',str(a.proof.resolve()),'--output',str(o/'public-tests')],timeout=3600))
rows.append(record(o,'rust-mkvcore-build',['bash','tools/rust-mkvcore.sh'],cwd=s,timeout=600))
rustarchive=s/'build/macos-rust-target/release/libmkvcore.a';assert rustarchive.is_file() and rustarchive.stat().st_size>0
assert (o/'rust-mkvcore-build.log').read_text().splitlines()[-1]==str(rustarchive)
rusttool=o/'rust-toolchain.log';assert rusttool.is_file() and 'rustc ' in rusttool.read_text() and 'cargo ' in rusttool.read_text()
rust={'enabled':True,'staticlib_sha256':sha(rustarchive),'staticlib_bytes':rustarchive.stat().st_size,'toolchain_log_sha256':sha(rusttool),'source_inputs_sha256':{e['path']:e['sha256'] for e in m['rust_inputs']}}
base+=['-DNV_RUST_MKVCORE'];libs+=[str(rustarchive)]
receipt=o/'public-tests/receipt.json';q=json.loads(receipt.read_text());assert q['passed'] and q['source_binding']==v
focused={'receipt_relative':str(receipt.relative_to(o)),'receipt_sha256':sha(receipt)}
units=sorted((s/'src').glob('*.c'))+sorted((s/'src/dts').glob('*.c'));apps=[]
assert sorted(str(x.relative_to(s)) for x in units)==m['native_translation_units'], 'Exact current-upstream native graph mismatch'
assert all(s/'src'/name in units for name in ['main.c','descoberta.c','sync.c','fontenota.c','sinopse.c','fontefiltro.c'])
if m['fresh_sdk_required']:
 for ui in ['tv','touch']:
  d=o/('objects-'+ui);d.mkdir();flags=base+(['-DNV_TOUCH_UI=1']if ui=='touch'else[])
  def compile_one(item):
   i,path=item;obj=d/(str(i)+'-'+path.stem+'.o');label=str(i)+'-'+path.stem
   r=record(d,label,[cc,*flags,'-I'+str(s/'src'),'-c',str(path),'-o',str(obj)])
   return str(obj),r
  with concurrent.futures.ThreadPoolExecutor(max_workers=3)as pool:objects=list(pool.map(compile_one,enumerate(units)))
  binary=o/('app-'+ui);rows.append(record(o,'link-'+ui,[cc,*[x[0]for x in objects],*libs,'-o',str(binary)]))
  rows.append(record(o,'linked-libraries-'+ui,['otool','-L',str(binary)]))
  linked_libraries=(o/('linked-libraries-'+ui+'.log')).read_text()
  assert 'libass.' in linked_libraries and 'libSDL2_ttf' in linked_libraries, 'Full app omitted a required font backend'
  apps.append({'ui':ui,'sha256':sha(binary),'bytes':binary.stat().st_size,'translation_units':len(units),'compile_records':[x[1]for x in objects]})
source_binding(s,a.proof)
bind_runtime_fonts()
result={'outcome':'passed','source_binding':v,'plan_sha256':sha(HERE/'plan.json'),'gates':rows,'contracts':contracts,'candidate_pr':m['pr'],'focused':focused,'full_apps':apps,'rust_mkvcore':rust,'font_backend':font_backend,'compiler':subprocess.check_output([cc,'--version'],text=True).splitlines()[0],'fresh_sdk_required':m['fresh_sdk_required'],'scope':'Exact frozen public font suites in their declared modes on actual macOS, with required SDL_ttf and libass. Full TV/touch links only where source requires fresh SDK validation; each full C application is freshly compiled and linked from the exact current-upstream graph.'}
(o/'native-verified.json').write_text(json.dumps(result,indent=2)+'\n');print('PASS complete exact-candidate native validation',flush=True)
