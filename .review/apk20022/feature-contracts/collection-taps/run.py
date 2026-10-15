#!/usr/bin/env python3
"""Run all three frozen collection pointer producers; never accept a subset."""
from pathlib import Path
import argparse,hashlib,json,subprocess,sys
HERE=Path(__file__).resolve().parent
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
p=argparse.ArgumentParser(description=__doc__)
for name in ('source','output'):p.add_argument('--'+name,type=Path,required=True)
p.add_argument('--cc',required=True);p.add_argument('--private',action='store_true');p.add_argument('--sanitize',action='store_true');a=p.parse_args()
source=a.source.resolve();out=a.output.resolve();assert not out.exists()
git=lambda *args:subprocess.check_output(['git','-C',str(source),*args],text=True).strip()
start={'commit':git('rev-parse','HEAD'),'tree':git('rev-parse','HEAD^{tree}')};assert not git('status','--porcelain')
# Accepted source has this real predicate; the old runner's compatibility fallback is forbidden.
assert 'int desc_filme_visto(' in (source/'src/descoberta.c').read_text(), 'Actual watched-state producer is required'
freezes={}
for name,root,manifest in [('gesture',HERE/'fixture',HERE/'fixture-freeze.json'),('reachability',HERE/'reachability',HERE/'reachability-freeze.json'),('async',HERE/'async',HERE/'async-freeze.json')]:
 freeze=json.loads(manifest.read_text())
 files=freeze['fixture_files_sha256']if name=='async'else freeze['files'];assert files,name
 for rel,expected in files.items():assert sha(root/rel)==expected,(name,rel)
 freeze=dict(freeze,files=files)
 freezes[name]=(root,manifest,freeze)
out.mkdir(parents=True)
records={}
for name,expected_cases in [('gesture',75),('reachability',36),('async',13)]:
 root,manifest,freeze=freezes[name]
 cmd=[sys.executable,str(root/'run.py'),'--source',str(source),'--output',str(out/name),'--cc',a.cc]+(['--private']if a.private else[])+(['--sanitize']if a.sanitize else[])
 result=subprocess.run(cmd,text=True,capture_output=True,timeout=840)
 logfile=out/(name+'.log');logfile.write_text(result.stdout+result.stderr)
 assert result.returncode==0,(name,result.stdout+result.stderr)
 rawpath=out/name/'receipt.json';raw=json.loads(rawpath.read_text())
 assert raw['source_commit']==start['commit']
 assert raw['private']is a.private and raw['sanitized']is a.sanitize
 if name in ('gesture','async'):
  assert raw['source_status']=='' and raw['exit_code']==0 and raw['abort_traps_fired']is False
  assert raw['actual_watched_producer_origin']['actual_function_present']is True
  runs=raw['runs'];assert len(runs)==expected_cases and len({item['case']for item in runs})==expected_cases
  assert all(item['exit_code']==0 and item['abort_traps_fired']is False for item in runs)
  required=['stale-filter','stale-reopen','stale-source','stale-session','stale-mode','stale-closed','stale-layout','stale-key','drag-x','drag-y','drag-diag-x','drag-diag-y','resume-x','resume-y','transition-x','transition-y','clip-top','multi','hold','tmdb','decades-clip']if name=='gesture'else['late-y','late-x','late-xy','active-y','active-x','regroup-xy','regroup-active-x','reset-active','stale-load-before-redraw','stale-load-after-redraw','stale-reorder-before-redraw','stale-reorder-after-redraw','tv-follow']
  assert all(any(item['case'].endswith('-'+case)for item in runs)for case in required)
 else:
  # This exact schema is completed against the frozen owner receipt before approval.
  assert raw['passed']is True and raw['outcome']=='passed'
  assert raw['tree']==start['tree']and raw['source_status']==''
  assert raw['cases']==expected_cases and raw['title_activations']==2160
  assert raw['abort_traps_fired']is False and raw['exit_code']==0
  expected={(v,o,z)for v in range(1,7)for o in range(2)for z in range(3)}
  assert len(raw['runs'])==36 and {(x['view'],x['orientation'],x['zoom'])for x in raw['runs']}==expected
  assert all(x['exit_code']==0 and x['abort_traps_fired']is False for x in raw['runs'])
  assert len(raw['case_summaries'])==36 and {(x['view'],x['orientation'],x['zoom'])for x in raw['case_summaries']}==expected
  assert all(x['titles']==60 and x['real_drags']>1 and x['usable48dp_targets']>0 and x['checks']>0 for x in raw['case_summaries'])
  assert raw['actual_producer_origin']['source']=='src/descoberta.c'
  assert raw['actual_producer_origin']['source_sha256']==sha(source/'src/descoberta.c')
  for case in raw['runs']:
   log=out/name/('run-view%d-orientation%d-zoom%d.log'%(case['view'],case['orientation'],case['zoom']))
   assert sha(log)==case['log_sha256']
 for file,expected in raw['dependency_sha256'].items():assert sha(file)==expected,(name,file)
 assert raw['commands'] and all(isinstance(command,list)and command for command in raw['commands'])
 for rel,expected in raw['tooling_sha256'].items():assert sha(root/rel)==expected,(name,rel)
 compile_commands=[command for command in raw['commands']if '-c'in command]
 assert compile_commands and all(('-DNV_TEST_LOCAL_LOGS=1'in command)is a.private for command in compile_commands)
 assert all(('-fsanitize=address,undefined'in command)is a.sanitize for command in compile_commands)
 assert all('-DNV_ANDROID=1'in command and '-DNV_TOUCH_UI=1'in command for command in compile_commands)
 for rel,expected in freeze['files'].items():assert sha(root/rel)==expected,(name,rel)
 records[name]={'cases':expected_cases,'raw_receipt_relative':name+'/receipt.json','raw_receipt_sha256':sha(rawpath),'producer_log_relative':name+'.log','producer_log_sha256':sha(logfile),'fixture_freeze_sha256':sha(manifest),'command':cmd}
 records[name]['source_inputs_sha256']={str(Path(file).relative_to(source)):expected for file,expected in raw['dependency_sha256'].items()if Path(file).is_relative_to(source)}
 records[name]['actual_watched_source_sha256']=sha(source/'src/descoberta.c')
 if name=='reachability':records[name]['title_activations']=2160
assert not git('status','--porcelain')and git('rev-parse','HEAD')==start['commit']and git('rev-parse','HEAD^{tree}')==start['tree']
receipt={'passed':True,'source_commit':start['commit'],'source_tree':start['tree'],'source_clean':True,'private':a.private,'sanitized':a.sanitize,'subproducers':records,'subcases':{'gesture':75,'reachability':36,'async':13},'reachability_title_activations':2160,'actual_watched_producer':True,'abort_traps_fired':False,'scope':'All three complete source-bound real-render/pointer producers. Gesture75, reachability36/all2160title activations and asynchronous row-load13 remain distinct; typed headless boundaries, no physical-device claim.'}
(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
print('PASS gesture75 + reachability36 (2160 title activations) + async13')
