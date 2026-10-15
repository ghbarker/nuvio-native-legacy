#!/usr/bin/env python3
"""Full macOS builds and focused actual-dispatch/target-owner checks."""
import argparse,hashlib,json,os,subprocess,sys
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);p.add_argument('--source-proof',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
source=a.source.resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True);here=Path(__file__).resolve().parent
proof=json.loads(a.source_proof.read_text());assert proof['verified'] and proof['product_delta_only']
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
commands=[]
def execute(name,cmd,cwd=source):
 with (out/(name+'.log')).open('w') as f:result=subprocess.run(cmd,cwd=cwd,stdout=f,stderr=subprocess.STDOUT)
 commands.append({'name':name,'arguments':cmd,'returncode':result.returncode,'log_sha256':sha(out/(name+'.log'))})
 assert result.returncode==0,(name,result.returncode)
 print('PASS',name,flush=True)
flags=['-O1','-DNV_SUPABASE_URL=""','-DNV_SUPABASE_ANON_KEY=""','-DNV_TV_LOGIN_BASE=""','-DNV_TRAKT_CLIENT_ID=""','-DNV_TRAKT_CLIENT_SECRET=""','-DNV_SIMKL_CLIENT_ID=""','-DNV_SIMKL_APP=""','-DNV_TMDB_API_KEY=""','-I/opt/homebrew/include','-I/opt/homebrew/include/SDL2','-L/opt/homebrew/lib','-lSDL2','-lSDL2_image','-lSDL2_ttf','-lz','-framework','OpenGL','-Wno-deprecated-declarations']
sources=[str(p) for p in sorted((source/'src').glob('*.c'))]+[str(p) for p in sorted((source/'src/dts').glob('*.c'))]
for ui in ('tv','touch'):
 binary=out/('app-'+ui);execute('full-source-'+ui,['cc',*sources,*flags,*(['-DNV_TOUCH_UI=1'] if ui=='touch' else []),'-o',str(binary)])
for fixture in ('dispatch','owner'):
 execute('actual-'+fixture,[sys.executable,str(here/fixture/'run.py'),'--source',str(source),'--output',str(out/fixture),'--mode','both'])
records={}
for name,count in [('dispatch',50),('owner',18)]:
 report=json.loads((out/name/'report.json').read_text());assert report['outcome']=='passed' and report['source_unchanged']
 assert {(r['mode'],r['ui']) for r in report['records']}=={(m,u) for m in ('normal','asan') for u in ('tv','mobile')}
 assert len(report['records'])==4
 for record in report['records']:
  assert record['assertions']==count and record['returncode']==0 and not record['traps_fired']
  for command in record['commands']:assert command['returncode']==0 or command['expected_link_probe']
  for path,digest in record['source_sha256'].items():assert sha(source/path)==digest
  assert any('-DNV_TPK=1' in c['arguments'] for c in record['commands']) if record['ui']=='tv' else any(all(d in c['arguments'] for d in ['-DNV_ANDROID=1','-DNV_TOUCH_UI=1']) for c in record['commands'])
  if record['mode']=='asan':assert '-fsanitize=address,undefined' in record['sanitizer_flags']
 records[name]=report
assert subprocess.run(['git','-C',str(source),'diff','--exit-code','HEAD','--'],capture_output=True).returncode==0
receipt={'outcome':'passed','case':proof['case'],'source_commit':proof['source_commit'],'review_commit':os.environ['GITHUB_SHA'],'source_scope':proof['scope'],
 'actual_source_fixtures':records,'assertions_passed':272,'fixture_executions':8,
 'full_source_compiles':{ui:{'sha256':sha(out/('app-'+ui)),'bytes':(out/('app-'+ui)).stat().st_size} for ui in ('tv','touch')},
 'commands':commands,'source_unchanged':True,'tests_tree_unchanged':proof['original_tests_tree_unchanged'],
 'scope_note':'Native control flow and source ownership tests, including active Mobile platform macros; no physical Tizen or phone execution claimed.'}
(out/'native-gates-verified.json').write_text(json.dumps(receipt,indent=2)+'\n')
print('Verified 272 actual-source assertions and complete TV/touch compiles for',proof['case'])
