#!/usr/bin/env python3
"""Cheap source-bound integrated player/router preflight; never builds an APK or launches CI."""
from pathlib import Path
import argparse,concurrent.futures,hashlib,json,os,subprocess,sys
p=argparse.ArgumentParser()
for key in ['source','manifest','output']:p.add_argument('--'+key,type=Path,required=True)
for key in ['commit','tree','manifest-sha256']:p.add_argument('--'+key,required=True)
p.add_argument('--manifest-root',type=Path)
p.add_argument('--layer',choices=['product','test'],required=True)
a=p.parse_args();s=a.source.resolve();o=a.output.resolve();here=Path(__file__).resolve().parent;f=here/'player496';sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
def git(*args):return subprocess.check_output(['git','-C',str(s),*args],text=True).strip()
assert sha(a.manifest)==a.manifest_sha256
manifest=json.loads(a.manifest.read_text())
assert manifest['source_frozen'] is True, 'Integrated source composition is not frozen'
entry=manifest['sources'][a.layer]
assert entry['layer']==a.layer and entry['source_clean'] is True
assert entry['private_logs'] is (a.layer=='test'), 'Layer/private opt-in scope mismatch'
assert entry['commit']==a.commit and entry['tree']==a.tree
assert isinstance(entry['path'],str) and entry['path'], 'Missing descriptive source origin path'
if entry.get('manifest'):
 assert sha((a.manifest_root.resolve() if a.manifest_root else a.manifest.resolve().parent)/entry['manifest'])==entry['manifest_sha256']
assert git('rev-parse','HEAD')==a.commit and git('rev-parse','HEAD^{tree}')==a.tree
assert not git('status','--porcelain'),'Refusing uncommitted product edits'
assert not o.exists(),'Use a fresh output directory for each frozen source/layer'
o.mkdir(parents=True)
for path,markers in {'src/player.c':['player_anterior_episodio','player_transporte_pedido_pendente','PLR_RECARREGAR'],'src/posplay.c':['posplay_troca_manual'],'src/app.c':['recarregarFonteVOD','player_transporte_pedido_pendente'],'src/video.h':['video_recarregar']}.items():
 text=(s/path).read_text();assert all(marker in text for marker in markers),(a.layer,path,'Composition lost approved APIs')
origin=json.loads((here/'fixture-origin.json').read_text());assert origin['frozen'] is True
for name,expected in origin['copied_without_modification'].items():
 actual=origin['adapter_delta']['paths'].get(name,expected);assert sha(f/name)==actual,name
private=['--private-logs']if a.layer=='test'else[]
base=[sys.executable];jobs=[]
for ui,flags in [('tv',['--render']),('touch',['--render','--touch']),('android',['--render','--touch','--android','--real-pointer'])]:
 for san in [False,True]:
  name='player-'+ui+('-asan'if san else'-normal');jobs.append((name,base+[str(f/'build-player.py'),'--source',str(s),'--output',str(o/name),'--episode-buttons',*flags,*(['--san']if san else[]),*private]))
for android in [False,True]:
 for san in [False,True]:
  name='app-'+('android'if android else'tv')+('-asan'if san else'-normal');jobs.append((name,base+[str(f/'run-app-route.py'),'--source',str(s),'--output',str(o/name),*(['--android']if android else[]),*(['--san']if san else[]),*private]))
jobs+=[('posplay',base+[str(f/'run-posplay.py'),'--source',str(s),'--output',str(o/'posplay'),*private]),('selectors',base+[str(f/'run-selectors.py'),'--source',str(s),'--output',str(o/'selectors'),'--previous'])]
def run(job):
 name,cmd=job;log=o/(name+'.log')
 try:
  z=subprocess.run(cmd,capture_output=True,text=True,timeout=180);log.write_text(z.stdout+z.stderr);result={'name':name,'command':cmd,'returncode':z.returncode,'log_sha256':sha(log)}
 except subprocess.TimeoutExpired as e:
  log.write_text('TIMEOUT after 180 seconds\n');result={'name':name,'command':cmd,'returncode':None,'timed_out':True,'log_sha256':sha(log)}
 print(name,'PASS'if result['returncode']==0 else'FAIL',flush=True);return result
with concurrent.futures.ThreadPoolExecutor(max_workers=3)as pool:records=list(pool.map(run,jobs))
assert git('rev-parse','HEAD')==a.commit and git('rev-parse','HEAD^{tree}')==a.tree and not git('status','--porcelain')
passed=all(r['returncode']==0 for r in records)
receipt={'outcome':'passed'if passed else'failed','source':str(s),'execution_source_path':str(s),'declared_origin_path':entry['path'],'layer':a.layer,'source_commit':a.commit,'tree':a.tree,'composition_manifest_sha256':a.manifest_sha256,'declared_layer_binding':entry,'fixture_origin_sha256':sha(here/'fixture-origin.json'),'private_macro_delta':['NV_TEST_LOCAL_LOGS=1']if private else[],'records':records,'complete_source_clean_after':True,'scope':'Cheap host actual player/event/renderer/pointer/profile/episode and exact app-helper routing/cancellation preflight with typed host service/GPU/text boundaries. Other changed-source/backend gates remain separately required. No real SDK app/decoder/CI/publication claim.'}
(o/'preflight-result.json').write_text(json.dumps(receipt,indent=2)+'\n')
raise SystemExit(0 if passed else 1)
