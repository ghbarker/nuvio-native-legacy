"""Accept only fresh successful producers and their actual current-run files."""
from pathlib import Path
import argparse,hashlib,json,os
from common import HERE,approved,sha
p=argparse.ArgumentParser();p.add_argument('--input',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
plan=approved();root=a.input.resolve();required=plan['required_jobs']
needs=json.loads(os.environ['REQUIRED_JOB_RESULTS'])
assert set(needs)==set(required) and all(needs[k]['result']=='success'for k in required)
def read(path):return json.loads(path.read_text())
def checked(parent,relative,digest):
 path=(parent/relative).resolve();assert path.is_relative_to(parent.resolve()) and path.is_file(),str(path)
 assert sha(path)==digest,str(path)
 return path
def source(v):
 assert v['verified'] and v['source_clean'] and v['product_only']
 assert v['source_commit']==plan['source_commit'] and v['tree']==plan['source_tree'] and v['base']==plan['base']
 assert v['source_parents']==[plan['base']] and v['review_commit']==os.environ['GITHUB_SHA']
 assert v['workflow_run_id']==int(os.environ['GITHUB_RUN_ID']) and v['workflow_run_attempt']==int(os.environ['GITHUB_RUN_ATTEMPT'])
 assert v['manifest_sha256']==plan['source_manifest_sha256'] and v['tests_unchanged'] and v['all_non_source_paths_unchanged']
def receipt(path,binding):
 r=read(path);assert r['outcome']=='passed' and r['plan_sha256']==sha(HERE/'plan.json')
 for key,value in r['source_binding'].items():assert binding[key]==value,(str(path),key)
 return r
proofs={};origins={}
for job in required:
 directory=root/('issue497-'+job+'-'+os.environ['GITHUB_SHA']+'-attempt'+os.environ['GITHUB_RUN_ATTEMPT'])
 assert directory.is_dir(),str(directory)
 v=read(directory/'source-verified.json');source(v);origins[job]=sha(directory/'source-verified.json')
 if job=='affected':
  f=directory/'affected';r=receipt(f/'affected-verified.json',v)
  assert {(c['kind'],c['mode'])for c in r['contracts']}=={(k,m)for k in ['startup','profile-data','syncprog','homejanelas']for m in ['normal','asan']}
  for gate in r['gates']:assert gate['returncode']==0;checked(f,gate['log'],gate['log_sha256'])
  for c in r['contracts']:
   if c['kind'] in ['syncprog','homejanelas']:checked(f,c['binary'],c['sha256']);continue
   path=checked(f,c['receipt'],c['sha256']);q=read(path)
   if c['kind']=='profile-data':
    assert q['passed'] and q['source_binding']==v and len(q['scenario_logs'])==9 and len(q['binaries'])==7
    data=path.parent/'profile-data/data'
    for name,digest in q['scenario_logs'].items():checked(data,name,digest)
    for name,digest in q['binaries'].items():checked(data,name,digest)
   else:
    assert q['passed'] and q['source_commit']==plan['source_commit'] and q['source_tree']==plan['source_tree']
    assert q['sanitizers']==(['address','undefined']if c['mode']=='asan'else[]) and q['groups']==['discovery','sync']
    for name,digest in plan['source_files'].items():assert q['source_inputs_sha256'][name]==digest,name
    for name,component in q['components'].items():
     sub=checked(path.parent,component['receipt_relative'],component['receipt_sha256']);n=read(sub)
     assert n['passed'] and n['sanitize']==(c['mode']=='asan') and len(n['scenarios'])==(4 if name=='discovery'else 3)
     checked(sub.parent,n['raw_receipt_relative'],n['raw_receipt_sha256'])
     checked(sub.parent,n['binary_relative'],n['binary_sha256'])
     for log,digest in n['scenario_logs_sha256'].items():checked(sub.parent,log,digest)
  proofs[job]=sha(f/'affected-verified.json')
 elif job=='native':
  f=directory/'native';r=receipt(f/'native-verified.json',v);assert {app['ui']for app in r['full_apps']}=={'tv','touch'}
  for gate in r['gates']:assert gate['returncode']==0;checked(f,gate['log'],gate['log_sha256'])
  for app in r['full_apps']:
   checked(f,'app-'+app['ui'],app['sha256']);assert app['translation_units']==len(app['compile_records']) and app['translation_units']>=300
   for gate in app['compile_records']:assert gate['returncode']==0;checked(f/('objects-'+app['ui']),gate['log'],gate['log_sha256'])
  proofs[job]=sha(f/'native-verified.json')
 elif job=='android':
  f=directory/'android';r=receipt(f/'android-verified.json',v)
  assert r['package']=='space.nuvio.nativelegacy' and r['private_logging_absent'] and r['native_16kb_pages']=='passed'
  assert {arm['abi']for arm in r['actual_arm_compiles']}=={'armeabi-v7a','arm64-v8a'}
  checked(f,'standard-validation.apk',r['apk_sha256'])
  for arm in r['actual_arm_compiles']:
   for database in arm['compile_databases']:checked(f,database['file'],database['sha256'])
  proofs[job]=sha(f/'android-verified.json')
 else:
  proofs[job]={}
  for mode in ['normal','asan']:
   f=directory/job/mode;path=f/('original-'+mode+'-verified.json');r=receipt(path,v)
   assert r['suite']==job and r['mode']==mode and r['script_unchanged'] and r['original_flags_unchanged']
   assert r['sanitizers']==(['address','undefined']if mode=='asan'else[])
   checked(f,job+'.log',r['log_sha256'])
   for item in r['compile_proofs']:checked(f,item['file'],item['sha256'])
   proofs[job][mode]=sha(path)
result={'outcome':'passed','source_commit':plan['source_commit'],'source_tree':plan['source_tree'],'review_commit':os.environ['GITHUB_SHA'],'workflow_run_id':int(os.environ['GITHUB_RUN_ID']),'workflow_run_attempt':int(os.environ['GITHUB_RUN_ATTEMPT']),'plan_sha256':sha(HERE/'plan.json'),'required_jobs':required,'producer_source_proofs':origins,'producer_receipts':proofs,'previous_execution_reused':False}
a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(result,indent=2)+'\n')
print('PASS all fresh exact-source #497 gates',flush=True)
