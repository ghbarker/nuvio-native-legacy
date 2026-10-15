from pathlib import Path
import argparse,concurrent.futures,json,os,platform,subprocess,sys
from common import HERE,sha,source_binding,record
p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);p.add_argument('--proof',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();s=a.source.resolve();o=a.output.resolve();o.mkdir(parents=True,exist_ok=True)
plan,v,m=source_binding(s,a.proof);assert platform.system()=='Darwin';cc='/usr/bin/cc';prefix=Path(subprocess.check_output(['brew','--prefix'],text=True).strip());rows=[]
base=['-std=gnu11','-O1','-g','-Wno-deprecated-declarations','-I'+str(prefix/'include'),'-I'+str(prefix/'include/SDL2')]
base+=['-D'+name+'=""'for name in ['NV_SUPABASE_URL','NV_SUPABASE_ANON_KEY','NV_TV_LOGIN_BASE','NV_TRAKT_CLIENT_ID','NV_TRAKT_CLIENT_SECRET','NV_SIMKL_CLIENT_ID','NV_SIMKL_APP','NV_TMDB_API_KEY']]
libs=['-L'+str(prefix/'lib'),'-Wl,-rpath,'+str(prefix/'lib'),'-lSDL2','-lSDL2_image','-lSDL2_ttf','-lz','-framework','OpenGL']
sanity=o/'sdk.c';sanity.write_text('#include <stdio.h>\n#include <AvailabilityMacros.h>\n#include <SDL2/SDL.h>\n#include <OpenGL/gl3.h>\nint main(void){puts("SDK ready");return 0;}\n')
rows.append(record(o,'sdk-compile',[cc,*base,str(sanity),*libs,'-o',str(o/'sdk')],timeout=60));rows.append(record(o,'sdk-run',[str(o/'sdk')],timeout=20))
rows.append(record(o,'languages',[sys.executable,str(s/'tools/idiomas.py')]))
baseline=s.parent/'source-baseline';subprocess.run(['git','worktree','add','--detach',str(baseline),m['base']],cwd=s,check=True)
rows.append(record(o,'baseline-defaults',[sys.executable,str(HERE/'fixtures/defaults.py'),'--baseline',str(baseline),'--source',str(s),'--output',str(o/'defaults'),'--cc',cc]))
contract_cc=o/'contract-cc';contract_cc.write_text('#!/bin/sh\nexec /usr/bin/cc -D_DARWIN_C_SOURCE=1 "$@"\n');contract_cc.chmod(0o755)
for ui in ['tv','mobile']:
 for sanitized in [False,True]:
  suffix=ui+('-asan'if sanitized else'-normal');extra=['--sanitize']if sanitized else[]
  # Settings is the same actual Mobile route; avoid a duplicate TV-labelled replay.
  modes=['close','preview','route']+(['settings']if ui=='mobile'else[])
  for mode in modes:
   rows.append(record(o,suffix+'-'+mode,[sys.executable,str(HERE/'fixtures/synopsis477/portable.py'),'--source',str(s),'--output',str(o/(suffix+'-'+mode)),'--cc',str(contract_cc),'--platform',ui,'--mode',mode,*extra]))
  env=dict(os.environ,SOURCE=str(s),PROOF_OUTPUT=str(o/(suffix+'-reader')),REVIEW_CC=str(contract_cc))
  rows.append(record(o,suffix+'-reader',[sys.executable,str(HERE/'fixtures/synopsis477/run.py'),*(['--tv']if ui=='tv'else[]),*(['--san']if sanitized else[])],env=env))
units=sorted((s/'src').glob('*.c'))+sorted((s/'src/dts').glob('*.c'));apps=[]
assert s/'src/sinopse.c'in units
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
result={'outcome':'passed','source_binding':v,'plan_sha256':sha(HERE/'plan.json'),'gates':rows,'full_apps':apps,'compiler':subprocess.check_output([cc,'--version'],text=True).splitlines()[0],'scope':'Fresh full macOS TV/touch links and actual synopsis/Settings/reader normal+ASan/UBSan closures; baseline defaults compatibility.'}
(o/'native-verified.json').write_text(json.dumps(result,indent=2)+'\n');print('PASS complete native integration',flush=True)
