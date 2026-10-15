from pathlib import Path
import argparse,hashlib,json,os
from common import HERE,approved,sha
p=argparse.ArgumentParser();p.add_argument('--input',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();plan=approved();review=os.environ['GITHUB_SHA'];needs=json.loads(os.environ['REQUIRED_JOB_RESULTS'])
names=['native','fontepref','fontepref_file','player_retido','android'];assert set(needs)==set(names) and all(needs[n]['result']=='success'for n in names)
inputs={};groups={}
def load(path):
 assert path.is_file(),path;inputs[str(path.relative_to(a.input))]=sha(path);return json.loads(path.read_text())
def binding(v):
 assert v['source_commit']==plan['source_commit']and v['tree']==plan['source_tree']and v['review_commit']==review
 assert v['manifest_sha256']==plan['source_manifest_sha256']
for name in names:
 d=a.input/('pr520-'+name+'-'+review);v=load(d/'source-verified.json');binding(v);assert v['verified']
 if name=='native':
  q=load(d/'native/native-verified.json');assert q['outcome']=='passed';binding(q['source_binding']);assert q['plan_sha256']==sha(HERE/'plan.json')
  assert {x['ui']for x in q['full_apps']}=={'tv','touch'}and len(q['full_apps'])==2
  expected={'sdk-compile','sdk-run','languages','c-contracts','retained-jvm','reporting-suites','link-tv','link-touch'}
  assert {x['name']for x in q['gates']}==expected and len(q['gates'])==len(expected)
  for g in q['gates']:assert g['returncode']==0 and sha(d/'native'/g['log'])==g['log_sha256']
  for app in q['full_apps']:
   assert app['translation_units']>100 and sha(d/'native'/('app-'+app['ui']))==app['sha256']
   for c in app['compile_records']:
    assert c['returncode']==0 and sha(d/'native'/('objects-'+app['ui'])/c['log'])==c['log_sha256']
  c=load(d/'native/contracts/report.json');assert c['outcome']=='passed' and c['baseline']==plan['base']
  assert set(c['settings_modes'])=={'desktop-tv','desktop-touch','android-tv','android-mobile'} and len(c['records'])==28
  for row in c['records']:assert row['exit_code']==0 and sha(d/'native/contracts'/(row['name']+'.log'))==row['output_sha256']
  retained=load(d/'native/retained-jvm.json');assert retained['verified'] and retained['contracts']==26 and retained['reexecuted']is False;binding(retained['current_source_binding'])
  report=load(d/'native/reporting/reporting-verified.json');assert report['outcome']=='passed';binding(report['source_binding']);assert len(report['records'])==5
  assert report['defaults_metadata_adapter']['original_unadapted_counted_as_pass']is False and report['sanitized_original_replay']['exit_code']==0
  for row in report['records']:assert row['returncode']==0 and sha(d/'native/reporting'/row['log'])==row['log_sha256']
  for relative,digest in report['compiler_proofs'].items():assert sha(d/'native/reporting'/relative)==digest
  groups[name]={'full_apps':2,'focused_gates':len(expected)}
 elif name=='android':
  q=load(d/'android/android-verified.json');assert q['outcome']=='passed';binding(q['source_binding']);assert q['plan_sha256']==sha(HERE/'plan.json')
  assert q['package']=='space.nuvio.nativelegacy'and q['private_logging_absent']and q['native_16kb_pages']=='passed'
  assert {x['abi']for x in q['actual_arm_compiles']}=={'arm64-v8a','armeabi-v7a'}
  assert sha(d/'android/standard-validation.apk')==q['apk_sha256']
  for abi in q['actual_arm_compiles']:
   for db in abi['compile_databases']:assert sha(d/'android'/db['file'])==db['sha256']
  before=load(d/'android-workspace/before.json');workspace=load(d/'android-workspace/after.json')
  assert before['outcome']=='passed'and before['phase']=='before'and before['cache_before_cleanup']['present']is False and before['cache_before_cleanup']['tracked_paths']==[]
  assert workspace['outcome']=='passed'and workspace['phase']=='after'and workspace['strict_clean_after']and workspace['source_commit']==plan['source_commit']and workspace['source_tree']==plan['source_tree']
  assert workspace['source']==before['source']and before['source_commit']==plan['source_commit']and before['source_tree']==plan['source_tree']
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
a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(result,indent=2)+'\n');print('PASS all5 actual producer jobs and source-bound checks')
