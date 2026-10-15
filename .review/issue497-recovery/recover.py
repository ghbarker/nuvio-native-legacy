"""Accept only fresh successful producers and their actual current-run files."""
from pathlib import Path
import argparse,hashlib,json,os
from common import HERE,approved,sha
from pathlib import Path
import hashlib,json,os,shutil,stat,subprocess,sys,time,urllib.error,urllib.parse,urllib.request,zipfile
RECOVERY=Path(__file__).resolve().parent

def recovery_contract():
 c=json.loads((RECOVERY/'contract.json').read_text())
 assert c['frozen'] and c['root_reviewed'] and c['ci_execution_permitted']
 assert sha(RECOVERY/'contract.json')==os.environ['ISSUE497_RECOVERY_CONTRACT_SHA256']
 assert sha(HERE/'plan.json')==c['original_plan_sha256']
 for name,digest in c['recipe_inputs'].items():assert sha(RECOVERY/name)==digest,name
 original=(HERE/'affected.py').read_bytes();corrected=(RECOVERY/'affected.py').read_bytes()
 assert hashlib.sha256(original).hexdigest()==c['original_runner_sha256']
 assert hashlib.sha256(corrected).hexdigest()==c['corrected_runner_sha256']
 assert original.count(b",'FALHOU'")==1 and corrected==original.replace(b",'FALHOU'",b'')
 assert os.environ['GITHUB_REF']=='refs/heads/'+c['review_branch']
 review=os.environ['GITHUB_SHA']
 assert subprocess.check_output(['git','rev-list','--parents','-n','1',review],text=True).split()==[review,c['original_review']]
 assert subprocess.check_output(['git','rev-parse',c['original_review']+'^{tree}'],text=True).strip()==c['original_review_tree']
 assert subprocess.check_output(['git','diff','--name-only',c['original_review'],review],text=True).splitlines()==sorted(c['allowed_new_paths'])
 return c
class SafeRedirect(urllib.request.HTTPRedirectHandler):
 def redirect_request(self,req,fp,code,msg,headers,newurl):
  result=super().redirect_request(req,fp,code,msg,headers,newurl)
  if result is not None and urllib.parse.urlsplit(req.full_url).netloc!=urllib.parse.urlsplit(newurl).netloc:result.remove_header('Authorization')
  return result
OPENER=urllib.request.build_opener(SafeRedirect())
def request(url):
 return OPENER.open(urllib.request.Request(url,headers={'Authorization':'Bearer '+os.environ['GITHUB_TOKEN'],'Accept':'application/vnd.github+json','X-GitHub-Api-Version':'2022-11-28','User-Agent':'issue497-exact-recovery'}),timeout=180)
def api(path):
 with request('https://api.github.com/repos/ghbarker/nuvio-native-legacy/'+path)as response:return json.load(response)
def actual_job(job,run,attempt,name):
 assert job['run_id']==run and job['run_attempt']==attempt and job['name']==name
 assert job['status']=='completed' and job['conclusion']=='success',(job['id'],job['status'],job['conclusion'])
def unpack_once(root,job,run,attempt,review,job_record):
 name='issue497-'+job+'-'+review+'-attempt'+str(attempt)
 listing=api('actions/runs/'+str(run)+'/artifacts?per_page=100')
 found=[a for a in listing['artifacts']if a['name']==name]
 assert len(found)==1,name
 spec=found[0];assert not spec['expired'] and spec['workflow_run']['id']==run and spec['workflow_run']['head_sha']==review
 expected=spec['digest'];assert expected.startswith('sha256:')and len(expected)==71
 archive=root/(str(spec['id'])+'.zip');assert not archive.exists()
 h=hashlib.sha256();n=0
 # Actions ZIP endpoints require the normal GitHub JSON Accept header.
 with request('https://api.github.com/repos/ghbarker/nuvio-native-legacy/actions/artifacts/'+str(spec['id'])+'/zip')as response,archive.open('xb')as target:
  while True:
   block=response.read(1024*1024)
   if not block:break
   n+=len(block);assert n<=spec['size_in_bytes'];target.write(block);h.update(block)
 assert n==spec['size_in_bytes'] and h.hexdigest()==expected.removeprefix('sha256:')
 destination=root/name;assert not destination.exists();destination.mkdir()
 with zipfile.ZipFile(archive)as z:
  for item in z.infolist():
   target=(destination/item.filename).resolve();assert target.is_relative_to(destination.resolve())
   assert not stat.S_ISLNK(item.external_attr>>16)
   if item.is_dir():target.mkdir(parents=True,exist_ok=True);continue
   target.parent.mkdir(parents=True,exist_ok=True)
   with z.open(item)as source,target.open('xb')as output:shutil.copyfileobj(source,output)
 return {'run':run,'attempt':attempt,'review':review,'job_id':job_record['id'],'job_name':job_record['name'],'artifact':{'id':spec['id'],'name':name,'bytes':n,'sha256':h.hexdigest()},'recipe_plan_sha256':sha(HERE/'plan.json')}
def gather(c,root):
 assert not root.exists();root.mkdir(parents=True)
 old=api('actions/runs/'+str(c['original_run']));assert old['head_sha']==c['original_review']and old['run_attempt']==c['original_attempt']
 failed=api('actions/jobs/'+str(c['original_affected_job']));assert failed['run_id']==c['original_run']and failed['run_attempt']==c['original_attempt']and failed['conclusion']=='failure'
 deadline=time.monotonic()+1800
 while True:
  jobs={name:api('actions/jobs/'+str(spec['id']))for name,spec in c['sdk_jobs'].items()}
  pending=[]
  for name,j in jobs.items():
   spec=c['sdk_jobs'][name];assert j['run_id']==c['original_run']and j['run_attempt']==c['original_attempt']and j['name']==spec['name']
   if j['status']!='completed':pending.append(name)
   else:actual_job(j,c['original_run'],c['original_attempt'],spec['name'])
  if not pending:break
  assert time.monotonic()<deadline,'Retained SDK producers did not finish before the bound'
  print('Waiting only for original SDK producers:',','.join(pending),flush=True);time.sleep(15)
 current_run=int(os.environ['GITHUB_RUN_ID']);attempt=int(os.environ['GITHUB_RUN_ATTEMPT']);review=os.environ['GITHUB_SHA']
 current=api('actions/runs/'+str(current_run));assert current['head_sha']==review and current['run_attempt']==attempt
 selected=[j for j in api('actions/runs/'+str(current_run)+'/jobs?per_page=100')['jobs']if j['name']==c['recovery_job_name']]
 assert len(selected)==1;actual_job(selected[0],current_run,attempt,c['recovery_job_name'])
 origins={name:unpack_once(root,name,c['original_run'],c['original_attempt'],c['original_review'],j)for name,j in jobs.items()}
 origins['affected']=unpack_once(root,'affected',current_run,attempt,review,selected[0])
 return origins

p=argparse.ArgumentParser();p.add_argument('--input',type=Path);p.add_argument('--output',type=Path);p.add_argument('--preflight-only',action='store_true');a=p.parse_args()
contract=recovery_contract()
if a.preflight_only:
 approved();print('PASS exact one-token recovery contract and unchanged original kit');raise SystemExit(0)
assert a.input is not None and a.output is not None
plan=approved();root=a.input.resolve();required=plan['required_jobs']
needs=json.loads(os.environ['REQUIRED_JOB_RESULTS'])
assert set(needs)=={'affected'} and needs['affected']['result']=='success'
origins=gather(contract,root)
def read(path):return json.loads(path.read_text())
def checked(parent,relative,digest):
 path=(parent/relative).resolve();assert path.is_relative_to(parent.resolve()) and path.is_file(),str(path)
 assert sha(path)==digest,str(path)
 return path
def source(v,origin):
 assert v['verified'] and v['source_clean'] and v['product_only']
 assert v['source_commit']==plan['source_commit'] and v['tree']==plan['source_tree'] and v['base']==plan['base']
 assert v['source_parents']==[plan['base']] and v['review_commit']==origin['review']
 assert v['workflow_run_id']==origin['run'] and v['workflow_run_attempt']==origin['attempt']
 assert v['manifest_sha256']==plan['source_manifest_sha256'] and v['tests_unchanged'] and v['all_non_source_paths_unchanged']
def receipt(path,binding):
 r=read(path);assert r['outcome']=='passed' and r['plan_sha256']==sha(HERE/'plan.json')
 for key,value in r['source_binding'].items():assert binding[key]==value,(str(path),key)
 return r
proofs={};source_proofs={}
for job in required:
 origin=origins[job]
 directory=root/origin['artifact']['name']
 assert directory.is_dir(),str(directory)
 v=read(directory/'source-verified.json');source(v,origin);source_proofs[job]=sha(directory/'source-verified.json')
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
result={'outcome':'passed','source_commit':plan['source_commit'],'source_tree':plan['source_tree'],'review_commit':os.environ['GITHUB_SHA'],'workflow_run_id':int(os.environ['GITHUB_RUN_ID']),'workflow_run_attempt':int(os.environ['GITHUB_RUN_ATTEMPT']),'plan_sha256':sha(HERE/'plan.json'),'required_jobs':required,'producer_source_proofs':source_proofs,'producer_origins':origins,'recovery_contract_sha256':sha(RECOVERY/'contract.json'),'corrected_runner_sha256':contract['corrected_runner_sha256'],'original_plan_sha256':contract['original_plan_sha256'],'producer_receipts':proofs,'retained_sdk_successes':5,'fresh_affected_successes':1,'receipts_rewritten':False,'sdk_rebuilt':False}
a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(result,indent=2)+'\n')
print('PASS all fresh exact-source #497 gates',flush=True)
