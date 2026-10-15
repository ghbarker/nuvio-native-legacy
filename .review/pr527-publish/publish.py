from pathlib import Path
import base64,hashlib,io,json,os,subprocess,sys,tempfile,time,urllib.error,urllib.request,zipfile
REPO='ghbarker/nuvio-native-legacy'
PARENT='34fa4395200d2b6f9bb6428e43f5aee1a32487bd'
SOURCE='f2a03e9797857bb0c1b9a671f2b32e85a536c545'
TREE='89512738dbe24dea56b41fa61b5729d62a29b8f9'
BASE='5d2f8c8c99113e416e1fb06f8b60da1d13635e85'
OLD='16194b499c273c9b1e602b8e158c92c34d6a9ef8'
BRANCH='feat/collection-view-default-362-20261015'
PLAN='4a07afd5cd17ead9540ceb0400ea842bc75a98cb9aefca99ced4a16e518cfd43'
AGG_RUN=38812578848
AGG_JOB=116689191132
ARTIFACT=12009820156
ZIP_SHA='32eca528104c5a1522c3087f264d09c41c6357b94c0d6d563ca2cf2212576508'
OLD_REVIEW='d4d28aa0b8aa0c798d3bc489dd91149fc891a145'
ANDROID_REVIEW='064b2365628d61cfd3ea92a7dcf4a67beb44c6c4'
EXPECTED={'native':116680430003,'fontepref':116680429894,'fontepref_file':116680430238,'player_retido':116680430193,'parallel':116680430347,'collection':116680430187,'android':116683572726}
ALLOWED={'.github/workflows/pr527-publish-approved.yml','.review/pr527-publish/publish.py'}
ROOT=Path.cwd()
sys.path.insert(0,str(ROOT/'.review/pr527-parallel-public'))
from common import approved,sha,source_binding
def api(repo,path):
 request=urllib.request.Request('https://api.github.com/repos/'+repo+'/'+path,headers={'Authorization':'Bearer '+os.environ['GH_TOKEN'],'Accept':'application/vnd.github+json','X-GitHub-Api-Version':'2022-11-28'})
 with urllib.request.urlopen(request,timeout=60)as response:return json.load(response)
def git(*args,env=None):
 return subprocess.check_output(['git',*args],text=True,env=env).strip()
class NoRedirect(urllib.request.HTTPRedirectHandler):
 def redirect_request(self,*args,**kwargs):return None
plan=approved()
assert plan['source_commit']==SOURCE and plan['source_tree']==TREE and plan['base']==BASE
assert sha(ROOT/'.review/pr527-parallel-public/plan.json')==PLAN
review=os.environ['GITHUB_SHA']
assert git('show','-s','--format=%P',review)==PARENT
assert set(git('diff','--name-only',PARENT,review).splitlines())==ALLOWED
run=api(REPO,'actions/runs/'+str(AGG_RUN));job=api(REPO,'actions/jobs/'+str(AGG_JOB))
assert run['head_sha']==PARENT and run['status']=='completed'and run['conclusion']=='success'and run['run_attempt']==1
assert run['path']=='.github/workflows/pr527-aggregate-recovery.yml'
assert job['run_id']==AGG_RUN and job['head_sha']==PARENT and job['status']=='completed'and job['conclusion']=='success'and job['run_attempt']==1
artifact=api(REPO,'actions/artifacts/'+str(ARTIFACT))
assert artifact['name']=='pr527-final-all-gates-'+PARENT and not artifact['expired']
assert artifact['size_in_bytes']==9444 and artifact['digest']=='sha256:'+ZIP_SHA
assert artifact['workflow_run']['id']==AGG_RUN and artifact['workflow_run']['head_sha']==PARENT
request=urllib.request.Request('https://api.github.com/repos/'+REPO+'/actions/artifacts/'+str(ARTIFACT)+'/zip',headers={'Authorization':'Bearer '+os.environ['GH_TOKEN']})
try:
 response=urllib.request.build_opener(NoRedirect).open(request,timeout=60)
except urllib.error.HTTPError as error:
 assert error.code in [301,302,303,307,308]
 location=error.headers['Location'];assert location.startswith('https://')
 response=urllib.request.urlopen(location,timeout=60)
with response:raw=response.read()
assert len(raw)==9444 and hashlib.sha256(raw).hexdigest()==ZIP_SHA
with zipfile.ZipFile(io.BytesIO(raw))as archive:
 files=[x for x in archive.infolist()if not x.is_dir()]
 assert len(files)==1 and files[0].filename=='all-gates-verified.json'
 proof_bytes=archive.read(files[0]);proof=json.loads(proof_bytes)
assert proof['outcome']=='passed'and proof['review_commit']==PARENT and int(proof['run_id'])==AGG_RUN and proof['run_attempt']==1
assert proof['source_commit']==SOURCE and proof['source_tree']==TREE and proof['plan_sha256']==PLAN
assert proof['source_manifest_sha256']==plan['source_manifest_sha256']
origins=proof['execution_origins']
assert origins['all_seven_successful_producers_retained']and origins['six_original_successes_retained']and origins['successful_android_recovery_preserved']
assert origins['receipts_rewritten']is False and origins['same_run_all_producers']is False
assert set(origins['producer_origins'])==set(EXPECTED)==set(proof['required_jobs'])==set(proof['verified_groups'])
for name,job_id in EXPECTED.items():
 origin=origins['producer_origins'][name];expected_review=ANDROID_REVIEW if name=='android'else OLD_REVIEW;expected_run=38810905287 if name=='android'else 38809969366
 assert origin['job_id']==job_id and origin['review_commit']==expected_review and origin['run_id']==expected_run and origin['run_attempt']==1
 assert origin['job_conclusion']=='success'and origin['retained_without_reexecution']is True
 assert origin['artifact']['api_digest']=='sha256:'+origin['artifact']['zip_sha256']and origin['artifact']['zip_bytes']>0
 actual=api(REPO,'actions/jobs/'+str(job_id))
 assert actual['run_id']==expected_run and actual['head_sha']==expected_review and actual['status']=='completed'and actual['conclusion']=='success'and actual['run_attempt']==1
 assert proof['required_jobs'][name]['job_id']==job_id and proof['required_jobs'][name]['result']=='success'
 assert 'pr527-'+name+'-'+expected_review+'/source-verified.json'in proof['artifact_inputs_sha256']
groups=proof['verified_groups']
assert groups['native']=={'full_apps':2,'focused_gates':16}
assert groups['parallel']=={'reader_cases_per_mode':45,'ui_cases_per_mode':10,'owned_url_cases_per_mode':1,'modes':['normal','asan'],'actual_public_reader_ui_owned_url':True}
for name in ['fontepref','fontepref_file','player_retido','collection']:assert groups[name]['modes']==['normal','asan']
assert groups['collection']['actual_repository_script']and groups['android']['abis']==2 and len(groups['android']['apk_sha256'])==64
assert proof['release_or_pr_created']is False
out=ROOT/'publication';out.mkdir()
(out/'all-gates-verified.json').write_bytes(proof_bytes)
subprocess.run([sys.executable,'.review/pr527-parallel-public/materialize.py','--target','source-app','--proof','publication/source-verified.json'],check=True)
source_binding(ROOT/'source-app',out/'source-verified.json')
assert git('-C','source-app','rev-parse','HEAD')==SOURCE and git('-C','source-app','rev-parse','HEAD^{tree}')==TREE
pr=api('iqui27/nuvio-native-legacy','pulls/527')
assert pr['state']=='open'and pr['head']['repo']['full_name']==REPO and pr['head']['ref']==BRANCH and pr['head']['sha']==OLD
url='https://github.com/'+REPO+'.git'
env=dict(os.environ);env['GIT_TERMINAL_PROMPT']='0'
with tempfile.TemporaryDirectory(prefix='pr527-push-auth-')as directory:
 askpass=Path(directory)/'askpass.sh'
 askpass.write_text('#!/bin/sh\ncase "$1" in *Username*) printf "%s\\n" "x-access-token";; *) printf "%s\\n" "$GH_TOKEN";; esac\n')
 askpass.chmod(0o700);env['GIT_ASKPASS']=str(askpass)
 remote=git('ls-remote','--heads',url,'refs/heads/'+BRANCH,env=env)
 assert remote.split()==[OLD,'refs/heads/'+BRANCH]
 subprocess.run(['git','-C','source-app','push','--force-with-lease=refs/heads/'+BRANCH+':'+OLD,url,'HEAD:refs/heads/'+BRANCH],env=env,check=True)
 remote=git('ls-remote','--heads',url,'refs/heads/'+BRANCH,env=env)
 assert remote.split()==[SOURCE,'refs/heads/'+BRANCH]
for attempt in range(30):
 pr=api('iqui27/nuvio-native-legacy','pulls/527')
 if pr['head']['sha']==SOURCE:break
 time.sleep(2)
assert pr['head']['sha']==SOURCE
result={'outcome':'passed','source_commit':SOURCE,'source_tree':TREE,'base':BASE,'old_head':OLD,'new_head':SOURCE,'branch':BRANCH,'force_with_lease_expected':OLD,'publisher_review':review,'publisher_run':os.environ['GITHUB_RUN_ID'],'plan_sha256':PLAN,'aggregate_run':AGG_RUN,'aggregate_job':AGG_JOB,'aggregate_artifact':ARTIFACT,'aggregate_zip_sha256':ZIP_SHA,'aggregate_json_sha256':hashlib.sha256(proof_bytes).hexdigest(),'verified_groups':groups,'producer_origins':origins['producer_origins'],'only_source_branch_updated':True,'sdk_builds_or_tests_rerun':False,'pr_ready_changed':False}
(out/'publish-result.json').write_text(json.dumps(result,indent=2)+'\n')
print('Published exact tested public source '+SOURCE+' with lease '+OLD,flush=True)
