from pathlib import Path
import argparse,hashlib,json,os
from common import HERE,approved,sha
p=argparse.ArgumentParser();p.add_argument('--input',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();plan=approved();review=os.environ['GITHUB_SHA'];needs=json.loads(os.environ['REQUIRED_JOB_RESULTS'])
names=['native','fontepref','fontepref_file','player_retido','android','features'];assert set(needs)==set(names) and all(needs[n]['result']=='success'for n in names)
inputs={};groups={}
def load(path):
 assert path.is_file(),path;inputs[str(path.relative_to(a.input))]=sha(path);return json.loads(path.read_text())
def binding(v):
 assert v['source_commit']==plan['source_commit']and v['tree']==plan['source_tree']and v['review_commit']==review
 assert v['manifest_sha256']==plan['source_manifest_sha256']
for name in names:
 d=a.input/('subtitle475-'+name+'-'+review);v=load(d/'source-verified.json');binding(v);assert v['verified']
 if name=='native':
  q=load(d/'native/native-verified.json');assert q['outcome']=='passed';binding(q['source_binding']);assert q['plan_sha256']==sha(HERE/'plan.json')
  assert {x['ui']for x in q['full_apps']}=={'tv','touch'}and len(q['full_apps'])==2
  expected={'sdk-compile','sdk-run','languages','baseline-defaults','link-tv','link-touch'}
  assert {x['name']for x in q['gates']}==expected and len(q['gates'])==len(expected)
  for g in q['gates']:assert g['returncode']==0 and sha(d/'native'/g['log'])==g['log_sha256']
  for app in q['full_apps']:
   assert app['translation_units']>100 and sha(d/'native'/('app-'+app['ui']))==app['sha256']
   for c in app['compile_records']:
    assert c['returncode']==0 and sha(d/'native'/('objects-'+app['ui'])/c['log'])==c['log_sha256']
  groups[name]={'full_apps':2,'focused_gates':len(expected)}
 elif name=='features':
  q=load(d/'features/features-verified.json');assert q['outcome']=='passed';binding(q['source_binding']);assert q['plan_sha256']==sha(HERE/'plan.json') and q['all_runtime_checks_fresh']
  expected={'settings-normal','settings-asan','secondary','outline-tv-normal','outline-tv-asan','outline-mobile-normal','outline-mobile-asan','style-normal','style-asan'}
  assert q['group_count']==9 and len(q['groups'])==9 and {g['name']for g in q['groups']}==expected
  for relative,h in q['artifact_inputs_sha256'].items():assert sha(d/'features'/relative)==h,relative
  for g in q['groups']:
   assert g['execution']['returncode']==0 and sha(d/'features'/g['execution']['log'])==g['execution']['log_sha256']
   assert sha(d/'features'/g['receipt'])==g['receipt_sha256'];raw=load(d/'features'/g['receipt']);assert raw['passed']
   if g['kind']=='settings':
    inner=load(d/'features'/g['name']/'menu/receipt.json');assert inner['source_commit']==plan['source_commit'] and inner['source_tree']==plan['source_tree'] and not inner['private'];assert len(inner['cases'])==6 and not inner['abort_traps_fired']and all(c['returncode']==0 for c in inner['cases'])
   else:
    assert raw['source_commit']==plan['source_commit'] and raw['source_tree']==plan['source_tree']
    if g['kind']=='secondary':assert raw['legacy_trace_cases']==16 and raw['legacy_traces_equal']and raw['independent_position_cases']==256 and raw['bound_delay_normal_and_asan']and raw['original_unbound_normal_and_asan']and all(c['returncode']==0 for c in raw['commands'])
    else:
     assert raw['sanitizers']==(['address','undefined']if g['name'].endswith('asan')else[])and not raw['private']and all(x['passed']for x in raw['groups'])
     if g['kind']=='outline':assert {x['group']for x in raw['groups']}=={'player','secondary','ass','palette','grid-font'}and raw['libass']['sdk_mode']=='Actual installed pkg-config SDK/library pair'
     if g['kind']=='style':assert {x['platform']for x in raw['groups']}=={'tv','mobile'}and all(x['returncode']==0 and not x['traps_fired']and x['stdout'].strip()=='PASS actual style controls 274 checks'for x in raw['groups'])
  groups[name]={'fresh_invocations':9,'settings_runtime_configs':12,'legacy_secondary_traces':16,'independent_secondary_positions_per_mode':256,'outline_group_runs':20,'style_platform_runs':4,'actual_libass':True,'original_unbound_script_normal_asan':True}
 elif name=='android':
  q=load(d/'android/android-verified.json');assert q['outcome']=='passed';binding(q['source_binding']);assert q['plan_sha256']==sha(HERE/'plan.json')
  assert q['package']=='space.nuvio.nativelegacy'and q['private_logging_absent']and q['native_16kb_pages']=='passed'
  assert {x['abi']for x in q['actual_arm_compiles']}=={'arm64-v8a','armeabi-v7a'}
  assert sha(d/'android/standard-validation.apk')==q['apk_sha256']
  for abi in q['actual_arm_compiles']:
   for db in abi['compile_databases']:assert sha(d/'android'/db['file'])==db['sha256']
  before=load(d/'android-workspace/before.json');workspace=load(d/'android-workspace/after.json');assert before['outcome']=='passed'and before['strict_clean_after']and before['source_commit']==plan['source_commit']and before['source_tree']==plan['source_tree']and before['cache_before_cleanup']['present']is False
  assert workspace['outcome']=='passed'and workspace['strict_clean_after']and workspace['source_commit']==plan['source_commit']and workspace['source_tree']==plan['source_tree']
  assert workspace['script_sha256']==before['script_sha256']==sha(HERE/'android_workspace.py')and workspace['before_sha256']==sha(d/'android-workspace/before.json')
  groups[name]={'abis':2,'apk_sha256':q['apk_sha256']}
 else:
  modes=[]
  for mode in ['normal','asan']:
   q=load(d/name/mode/('original-'+mode+'-verified.json'));assert q['outcome']=='passed'and q['suite']==name and q['mode']==mode;binding(q['source_binding']);assert q['plan_sha256']==sha(HERE/'plan.json')
   assert q['script_unchanged']and q['original_flags_unchanged']and q['full_source_files_compiled']>100
   for c in q['compile_proofs']:assert sha(d/name/mode/c['file'])==c['sha256']
   assert sha(d/name/mode/(name+'.log'))==q['log_sha256'];modes.append(mode)
  groups[name]={'modes':modes}
result={'outcome':'passed','review_commit':review,'run_id':os.environ['GITHUB_RUN_ID'],'run_attempt':int(os.environ['GITHUB_RUN_ATTEMPT']),'source_commit':plan['source_commit'],'source_tree':plan['source_tree'],'source_manifest_sha256':plan['source_manifest_sha256'],'plan_sha256':sha(HERE/'plan.json'),'required_jobs':needs,'verified_groups':groups,'artifact_inputs_sha256':inputs,'release_or_pr_created':False}
a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(result,indent=2)+'\n');print('PASS all6 actual producer jobs and source-bound checks')
