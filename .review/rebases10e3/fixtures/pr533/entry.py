"""Nine reviewed actual-source filter/planner/profile closures on Linux."""
from pathlib import Path
import argparse,json,os,platform,subprocess,sys,tempfile
HERE=Path(__file__).resolve().parent;sys.path.insert(0,str(HERE.parents[1]))
from common import source_binding,record,sha
p=argparse.ArgumentParser()
for key in ['source','proof','output','cc']:p.add_argument('--'+key,type=Path,required=True)
a=p.parse_args();s=a.source.resolve();out=a.output.resolve();assert not out.exists();out.mkdir(parents=True)
plan,v,m=source_binding(s,a.proof);assert m['pr']==533 and platform.system()=='Linux'
origin=json.loads((HERE/'fixture-origin.json').read_text())
for relative,digest in origin['inputs'].items():assert sha(HERE/relative)==digest,relative
ui=out/'current-ui.c';record(out,'extract-current-ui',[sys.executable,str(HERE/'prepare_ui.py'),str(s),str(ui)])
groups={
 'core':([HERE/'reviewed-fontefiltro.c',s/'src/fontefiltro.c'],False),
 'parser':([HERE/'reviewed-fontefiltro_parse.c',s/'src/stream_parse.c',s/'src/fontefiltro.c',s/'src/js.c',s/'src/badges.c'],False),
 'planner':([HERE/'reviewed-fontefalha.c',s/'src/fontefalha.c',s/'src/fontefiltro.c'],True),
 'worker':([HERE/'reviewed-fontefalha_worker.c',s/'src/fontefalha.c',s/'src/fontefiltro.c',s/'src/fonteauto.c'],True),
 'ui-handlers':([ui,s/'src/fontefalha.c',s/'src/fontefiltro.c'],True),
 'preparation':([HERE/'preparation_filters.c',s/'src/fontefalha.c',s/'src/fontefiltro.c',s/'src/fonteauto.c'],True),
 'concurrency':([HERE/'config_concurrency.c',s/'src/fontefalha.c',s/'src/fontefiltro.c'],True),
 'profile-bank':([HERE/'profile-bank-filters.c',s/'src/fontefalha.c',s/'src/fontefiltro.c'],False),
 'profile-scope':([HERE/'profile-scope-filters.c',s/'src/debrid.c',s/'src/fontefalha.c',s/'src/fontefiltro.c'],False)}
rows=[]
for mode in ['normal','asan']:
 for name,(inputs,needs_data)in groups.items():
  d=out/(name+'-'+mode);d.mkdir();binary=d/'fixture'
  flags=['-std=gnu11','-O1','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-unused-parameter','-Wno-unused-but-set-variable','-Wno-misleading-indentation','-Wno-format-truncation','-Wno-restrict','-I'+str(HERE/'sdk'),'-I'+str(s/'src'),'-DNV_STREAMS_SOURCE="'+str(s/'src/streams.c')+'"','-ffunction-sections','-fdata-sections','-Wl,--gc-sections']
  if mode=='asan':flags+=['-fsanitize=address,undefined,float-cast-overflow','-fno-omit-frame-pointer','-fno-sanitize-recover=all','-fno-pie','-no-pie']
  compiled=record(d,'compile',[str(a.cc),*flags,*map(str,inputs),'-pthread','-lm','-o',str(binary)],timeout=180)
  env=dict(os.environ,ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
  with tempfile.TemporaryDirectory(prefix='filters-')as data:
   ran=record(d,'run',[str(binary)]+([data]if needs_data else[]),env=env,timeout=120)
  text=(d/'run.log').read_text();assert not any(x in text for x in ['ERROR: AddressSanitizer','runtime error:'])
  rows.append({'name':name,'mode':mode,'compile':compiled,'run':ran,'directory':str(d.relative_to(out)),'binary_sha256':sha(binary)})
source_binding(s,a.proof)
r={'passed':True,'source_binding':v,'source_commit':m['head'],'source_tree':m['tree'],'groups':rows,'group_count':18,'generated_ui_sha256':sha(ui),'ui_manifest_sha256':sha(ui.with_suffix('.manifest.json')),'fixture_origin_sha256':sha(HERE/'fixture-origin.json'),'scope':'Fresh actual filter core/parser/planner/worker/current Settings/preparation/concurrency/profile bank/profile scope normal+ASan/UBSan; unchanged reviewed assertions.'}
(out/'receipt.json').write_text(json.dumps(r,indent=2)+'\n');print('PASS PR533 nine relevant groups normal+ASan',flush=True)
