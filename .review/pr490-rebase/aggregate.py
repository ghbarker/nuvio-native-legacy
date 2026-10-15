from pathlib import Path
import argparse,json,os
from common import HERE,approved,sha
p=argparse.ArgumentParser();p.add_argument('--input',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();plan=approved();review=os.environ['GITHUB_SHA'];needs=json.loads(os.environ['REQUIRED_JOB_RESULTS']);names=['native','fontepref','fontepref_file','player_retido','android'];assert set(needs)==set(names)and all(needs[n]['result']=='success'for n in names)
inputs={};groups={}
def load(path):
 assert path.is_file(),path;inputs[str(path.relative_to(a.input))]=sha(path);return json.loads(path.read_text())
def binding(v):
 assert v['source_commit']==plan['source_commit']and v['tree']==plan['source_tree']and v['review_commit']==review and v['manifest_sha256']==plan['source_manifest_sha256']
for name in names:
 d=a.input/('pr490-'+name+'-'+review);v=load(d/'source-verified.json');binding(v);assert v['verified']
 if name=='native':
  q=load(d/'native/native-verified.json');assert q['outcome']=='passed';binding(q['source_binding']);assert q['plan_sha256']==sha(HERE/'plan.json')
  assert {x['ui']for x in q['full_apps']}=={'tv','touch'}and len(q['full_apps'])==2
  expected={'sdk-compile','sdk-run','languages','source-invariants','link-tv','link-touch','geometry-compile','geometry-link','geometry-run','watched-normal','watched-asan','extra-normal','extra-asan'}
  assert {x['name']for x in q['gates']}==expected and len(q['gates'])==len(expected)
  for g in q['gates']:assert g['returncode']==0 and sha(d/'native'/g['log'])==g['log_sha256']
  assert len(q['contracts'])==4 and {(c['group'],c['mode'])for c in q['contracts']}=={(group,mode)for group in ['watched','extra']for mode in ['normal','asan']}
  expected_suites={'discovery','colfiltro-existing','history-existing','homepos-existing','home-tv','home-touch','catalog-ui-tv','catalog-ui-touch','settings-tv','settings-touch'}
  for c in q['contracts']:
   relative=Path(c['directory']);assert not relative.is_absolute()and'..'not in relative.parts
   path=d/'native'/relative
   if c['group']=='watched':
    assert set(c['suites'])==set(c['logs_sha256'])==expected_suites and len(c['suites'])==10
    for label,h in c['logs_sha256'].items():assert sha(path/(label+'.log'))==h and(path/(label+'.log')).stat().st_size
   else:
    r=load(path/'verified.json');assert sha(path/'verified.json')==c['receipt_sha256']
    assert r['source_commit']==plan['source_commit']and r['sanitized']==(c['mode']=='asan')and r['sdk_mode']=='real macOS/SDL SDK'
    assert len(r['records'])==3 and {x['name']for x in r['records']}=={'profile-session','all-eight-tv','all-eight-touch'}
    for x in r['records']:
     assert x['returncode']==0 and not x['expected_baseline_failure']
     assert x['passed_lines']=={'profile-session':1,'all-eight-tv':8,'all-eight-touch':16}[x['name']]
     assert sha(path/(x['name']+'.log'))==x['log_sha256']
  fixture=HERE/'fixtures/watched'
  assert q['fixture_files_sha256']=={str(x.relative_to(fixture)):sha(x)for x in fixture.rglob('*')if x.is_file()and'__pycache__'not in x.parts}
  assert 'colecao_exibicao: ok'in(d/'native/geometry-run.log').read_text()
  for app in q['full_apps']:
   assert app['translation_units']>100 and sha(d/'native'/('app-'+app['ui']))==app['sha256']
   for c in app['compile_records']:assert c['returncode']==0 and sha(d/'native'/('objects-'+app['ui'])/c['log'])==c['log_sha256']
  groups[name]={'full_apps':2,'watched_suites':20,'extra_groups':6,'all_eight_combinations':48,'profile_epoch_modes':2,'geometry':'passed','translation_alignment':'passed'}
 elif name=='android':
  q=load(d/'android/android-verified.json');assert q['outcome']=='passed';binding(q['source_binding']);assert q['plan_sha256']==sha(HERE/'plan.json')
  assert q['package']=='space.nuvio.nativelegacy'and q['private_logging_absent']and q['native_16kb_pages']=='passed'
  assert {x['abi']for x in q['actual_arm_compiles']}=={'arm64-v8a','armeabi-v7a'}and sha(d/'android/standard-validation.apk')==q['apk_sha256']
  for abi in q['actual_arm_compiles']:
   for db in abi['compile_databases']:assert sha(d/'android'/db['file'])==db['sha256']
  groups[name]={'abis':2,'apk_sha256':q['apk_sha256']}
 else:
  for mode in ['normal','asan']:
   q=load(d/name/mode/('original-'+mode+'-verified.json'));assert q['outcome']=='passed'and q['suite']==name and q['mode']==mode;binding(q['source_binding']);assert q['plan_sha256']==sha(HERE/'plan.json')
   assert q['script_unchanged']and q['original_flags_unchanged']and q['full_source_files_compiled']>100
   for c in q['compile_proofs']:assert sha(d/name/mode/c['file'])==c['sha256']
   assert sha(d/name/mode/(name+'.log'))==q['log_sha256']
  groups[name]={'modes':['normal','asan']}
result={'outcome':'passed','review_commit':review,'run_id':os.environ['GITHUB_RUN_ID'],'run_attempt':int(os.environ['GITHUB_RUN_ATTEMPT']),'source_commit':plan['source_commit'],'source_tree':plan['source_tree'],'source_manifest_sha256':plan['source_manifest_sha256'],'plan_sha256':sha(HERE/'plan.json'),'required_jobs':needs,'verified_groups':groups,'artifact_inputs_sha256':inputs,'release_or_pr_created':False}
a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(result,indent=2)+'\n');print('PASS5fresh exact-source producers and watched regression proofs')
