from pathlib import Path
import argparse,concurrent.futures,json,os,platform,subprocess,sys
from common import HERE,sha,source_binding,record
p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);p.add_argument('--proof',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();s=a.source.resolve();o=a.output.resolve();o.mkdir(parents=True,exist_ok=True)
plan,v,m=source_binding(s,a.proof);assert platform.system()=='Darwin';cc='/usr/bin/cc';prefix=Path(subprocess.check_output(['brew','--prefix'],text=True).strip());rows=[];contracts=[]
base=['-std=gnu11','-O1','-g','-Wno-deprecated-declarations','-I'+str(prefix/'include'),'-I'+str(prefix/'include/SDL2')]
base+=['-D'+name+'=""'for name in ['NV_SUPABASE_URL','NV_SUPABASE_ANON_KEY','NV_TV_LOGIN_BASE','NV_TRAKT_CLIENT_ID','NV_TRAKT_CLIENT_SECRET','NV_SIMKL_CLIENT_ID','NV_SIMKL_APP','NV_TMDB_API_KEY']]
libs=['-L'+str(prefix/'lib'),'-Wl,-rpath,'+str(prefix/'lib'),'-lSDL2','-lSDL2_image','-lSDL2_ttf','-lz','-framework','OpenGL']
sanity=o/'sdk.c';sanity.write_text('#include <stdio.h>\n#include <AvailabilityMacros.h>\n#include <SDL2/SDL.h>\n#include <OpenGL/gl3.h>\nint main(void){puts("SDK ready");return 0;}\n')
rows.append(record(o,'sdk-compile',[cc,*base,str(sanity),*libs,'-o',str(o/'sdk')],timeout=60));rows.append(record(o,'sdk-run',[str(o/'sdk')],timeout=20))
rows.append(record(o,'languages',[sys.executable,str(s/'tools/idiomas.py')]))
# Cheap affected actual-source suites run before the full application links.
for platform in ['tv','android-touch']:
 for sanitized in [False,True]:
  mode='asan'if sanitized else'normal';label='language-'+platform+'-'+mode;directory=o/label
  cmd=[sys.executable,str(HERE/'fixtures/language/run.py'),'--source',str(s),'--output',str(directory),'--cc',cc,'--platform',platform]+(['--sanitize']if sanitized else[])
  rows.append(record(o,label,cmd));receipt=directory/'receipt.json';q=json.loads(receipt.read_text())
  assert q['outcome']=='passed'and q['source_commit']==m['head']and q['source_tree']==m['tree']and q['source_clean']
  assert q['platform']==platform and q['sanitizers']==(['address','undefined']if sanitized else[])
  assert len(q['groups'])==6 and all(x['passed']for x in q['groups'])
  contracts.append({'platform':platform,'mode':mode,'label':label,'receipt':str(receipt.relative_to(o)),'sha256':sha(receipt)})
units=sorted((s/'src').glob('*.c'))+sorted((s/'src/dts').glob('*.c'));apps=[]
assert s/'src/streams.c'in units and s/'src/linguas.c'in units
for ui in ['tv','touch']:
 d=o/('objects-'+ui);d.mkdir();flags=base+(['-DNV_TOUCH_UI=1']if ui=='touch'else[])
 def compile_one(item):
  i,path=item;obj=d/(str(i)+'-'+path.stem+'.o');label=str(i)+'-'+path.stem
  r=record(d,label,[cc,*flags,'-I'+str(s/'src'),'-c',str(path),'-o',str(obj)])
  return str(obj),r
 with concurrent.futures.ThreadPoolExecutor(max_workers=3)as pool:objects=list(pool.map(compile_one,enumerate(units)))
 binary=o/('app-'+ui);rows.append(record(o,'link-'+ui,[cc,*[x[0]for x in objects],*libs,'-o',str(binary)]))
 apps.append({'ui':ui,'sha256':sha(binary),'bytes':binary.stat().st_size,'translation_units':len(units),'compile_records':[x[1]for x in objects]})
source_binding(s,a.proof)
result={'outcome':'passed','source_binding':v,'plan_sha256':sha(HERE/'plan.json'),'gates':rows,'contracts':contracts,'full_apps':apps,'compiler':subprocess.check_output([cc,'--version'],text=True).splitlines()[0],'scope':'Fresh full macOS TV/touch links,24focused language/Polish/account-reset/original language/i18n groups and full translation alignment. No private overlay, device or track-probing claim.'}
(o/'native-verified.json').write_text(json.dumps(result,indent=2)+'\n');print('PASS complete native source-language validation',flush=True)
