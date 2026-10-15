"""Bind successful same-source groups and the isolated clock-boundary retry; never hide failed evidence."""
from pathlib import Path
import hashlib,json,subprocess
r=Path(__file__).resolve().parent
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
manifest=r.parent/'composition-manifest.json';m=json.loads(manifest.read_text());layers={}
for layer in ['product','test']:
 oldpath=r/('results-'+layer)/'preflight-result.json';old=json.loads(oldpath.read_text());e=m['sources'][layer];s=Path(old['execution_source_path']);assert old['source_commit']==e['commit'] and old['tree']==e['tree'] and old['composition_manifest_sha256']==sha(manifest)
 assert subprocess.check_output(['git','-C',str(s),'rev-parse','HEAD'],text=True).strip()==e['commit']
 assert subprocess.check_output(['git','-C',str(s),'rev-parse','HEAD^{tree}'],text=True).strip()==e['tree']
 assert not subprocess.check_output(['git','-C',str(s),'status','--porcelain'],text=True).strip()
 kept=[x for x in old['records']if x['name']!='posplay'];assert len(kept)==11 and all(x['returncode']==0 for x in kept)
 for group in kept:
  resultpath=r/('results-'+layer)/group['name']/'result.json'
  if group['name']=='selectors':resultpath=r/('results-'+layer)/'selectors/result.json'
  j=json.loads(resultpath.read_text());assert j['outcome']=='passed'
  if group['name'].startswith('player-'):
   assert all(sha(p)==h for p,h in j['dependency_closure'].items())
   assert all(sha(r/'player496'/n)==h for n,h in j['fixture_sha256'].items())
  if group['name'].startswith('app-'):assert j['app_sha256']==sha(s/'src/app.c') and j['fixture_sha256']==sha(r/'player496/app-route.c')
  if group['name']=='selectors':assert all(sha(s/p)==h for p,h in j['source_hashes'].items())
  group['result_sha256']=sha(resultpath)
 retry=r/('results-'+layer+'-posplay-clock/result.json');j=json.loads(retry.read_text());assert j['outcome']=='passed' and j['source_sha256']==sha(s/'src/posplay.c') and j['fixture_sha256']==sha(r/'player496/posplay-manual.c')
 assert all(x['returncode']==0 for x in j['records'])
 if layer=='test':assert all('-DNV_TEST_LOCAL_LOGS=1'in x['command']for x in j['records']if x['label']=='build')
 layers[layer]={'outcome':'passed','source_commit':e['commit'],'tree':e['tree'],'original_batch_receipt_sha256':sha(oldpath),'original_batch_outcome':old['outcome'],'unchanged_successful_groups':kept,'posplay_final_receipt':str(retry.relative_to(r)),'posplay_final_receipt_sha256':sha(retry),'scope':'All12groups pass on exact same frozen source; isolated posplay clock-boundary replay supplies the final twelfth group.'}
receipt={'outcome':'passed','composition_manifest_sha256':sha(manifest),'source_frozen':True,'layers':layers,'scope':'Cheap actual player/events/renderer/pointer/profile/episode and app routing/posplay host preflight, normal+ASan/UBSan. Typed graphics/text/media boundaries; genuine sinopse.c producer linked. Not real SDK/app/decoder or macOS runtime passes.','failure_history':{'test_initial_group':'posplay','initial_result':'link failed: private diagnostic timestamp required SDL_GetTicks','change':'Only typed deterministic fixture SDL clock added; C assertions/product source untouched.','failed_batch_preserved':'results-test/preflight-result.json','final_passes':'posplay normal/ASan replayed for both layers; other11group source/fixture/dependency hashes rechecked unchanged.'},'CI_or_product_edit_by_reviewer':False}
(r/'local-preflight-verified.json').write_text(json.dumps(receipt,indent=2)+'\n');print(sha(r/'local-preflight-verified.json'))
