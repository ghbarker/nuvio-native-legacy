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
 d=a.input/('issue529-'+name+'-'+review);v=load(d/'source-verified.json');binding(v);assert v['verified']
 if name=='native':
  q=load(d/'native/native-verified.json');assert q['outcome']=='passed';binding(q['source_binding']);assert q['plan_sha256']==sha(HERE/'plan.json')
  assert {x['ui']for x in q['full_apps']}=={'tv','touch'}and len(q['full_apps'])==2
  expected={'sdk-compile','sdk-run','languages','link-tv','link-touch'}|{'language-'+ui+'-'+mode for ui in ['tv','android-touch']for mode in ['normal','asan']}
  assert {x['name']for x in q['gates']}==expected and len(q['gates'])==len(expected)
  for g in q['gates']:assert g['returncode']==0 and sha(d/'native'/g['log'])==g['log_sha256']
  assert len(q['contracts'])==4 and {(c['platform'],c['mode'])for c in q['contracts']}=={(ui,mode)for ui in ['tv','android-touch']for mode in ['normal','asan']}
  for c in q['contracts']:
   relative=Path(c['receipt']);assert not relative.is_absolute()and'..'not in relative.parts
   path=d/'native'/relative;r=load(path);assert sha(path)==c['sha256']and r['outcome']=='passed'and r['source_clean']and r['source_commit']==plan['source_commit']and r['source_tree']==plan['source_tree']
   assert r['platform']==c['platform']and r['sanitizers']==(['address','undefined']if c['mode']=='asan'else[])
   assert len(r['groups'])==6 and {g['group']for g in r['groups']}=={'chip','account-reset','subtitles','original-languages','original-automatic','original-i18n'}
   for g in r['groups']:
    assert g['passed']and g['commands']and g['dependency_sha256']
    if g['group']=='chip':assert 'language-chip cases=70 failures=0'in g['output']
    if g['group']=='account-reset':assert 'account-reset actual Settings + language policy cases=21'in g['output']
    for relative,h in g['logs_sha256'].items():
     relative=Path(relative);assert not relative.is_absolute()and'..'not in relative.parts
     assert sha(path.parent/relative)==h
   fixture=HERE/'fixtures/language'
   for relative,h in r['fixture_files_sha256'].items():
    relative=Path(relative);assert not relative.is_absolute()and'..'not in relative.parts
    assert sha(fixture/relative)==h
  for app in q['full_apps']:
   assert app['translation_units']>100 and sha(d/'native'/('app-'+app['ui']))==app['sha256']
   for c in app['compile_records']:assert c['returncode']==0 and sha(d/'native'/('objects-'+app['ui'])/c['log'])==c['log_sha256']
  groups[name]={'full_apps':2,'focused_groups':24,'chip_assertions_per_variant':70,'account_reset_assertions_per_variant':21,'translation_alignment':'passed'}
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
a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(result,indent=2)+'\n');print('PASS5fresh exact-source producers and language regression proofs')
