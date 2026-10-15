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
baseline=s.parent/'source-baseline';subprocess.run(['git','worktree','add','--detach',str(baseline),m['base']],cwd=s,check=True)
rows.append(record(o,'baseline-defaults',[sys.executable,str(HERE/'fixtures/defaults.py'),'--baseline',str(baseline),'--source',str(s),'--output',str(o/'defaults'),'--cc',cc]))
contract_cc=o/'contract-cc';contract_cc.write_text('#!/bin/sh\nexec /usr/bin/cc -D_DARWIN_C_SOURCE=1 -DNV_GLES_NO_MAC=1 "$@"\n');contract_cc.chmod(0o755)
# Compile and run every cheap affected actual-source closure before full app links.
for group in ['taps','reachability','async','settings','collection-state']:
 for sanitized in [False,True]:
  mode='asan'if sanitized else'normal';label=group+'-'+mode;directory=o/label
  cmd=[sys.executable,str(HERE/'fixtures'/group/'run.py'),'--source',str(s),'--output',str(directory),'--cc',str(contract_cc)]+(['--sanitize']if sanitized else[])
  rows.append(record(o,label,cmd));receipt=directory/'receipt.json';assert receipt.is_file()
  q=json.loads(receipt.read_text());assert q['source_commit']==m['head'] and q.get('tree',q.get('source_tree',m['tree']))==m['tree']
  contracts.append({'group':group,'mode':mode,'label':label,'receipt':str(receipt.relative_to(o)),'sha256':sha(receipt)})
units=sorted((s/'src').glob('*.c'))+sorted((s/'src/dts').glob('*.c'));apps=[]
assert s/'src/vertudo.c'in units and s/'src/ajustes.c'in units and s/'src/perfis.c'in units
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
result={'outcome':'passed','source_binding':v,'plan_sha256':sha(HERE/'plan.json'),'gates':rows,'contracts':contracts,'contract_compiler_sha256':sha(contract_cc),'full_apps':apps,'compiler':subprocess.check_output([cc,'--version'],text=True).splitlines()[0],'scope':'Fresh full macOS TV/touch links and actual collection Settings/profile/view/touch/async row contracts, normal+ASan/UBSan, before full links. No held source/subtitle/cache features or private overlay.'}
(o/'native-verified.json').write_text(json.dumps(result,indent=2)+'\n');print('PASS complete native collection validation',flush=True)
