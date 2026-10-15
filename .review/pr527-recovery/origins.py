from pathlib import Path
import datetime,hashlib,json,os,subprocess,tempfile,time,urllib.error,urllib.request,zipfile

OLD_RUN=38809969366
OLD_REVIEW='d4d28aa0b8aa0c798d3bc489dd91149fc891a145'
PLAN_SHA='4a07afd5cd17ead9540ceb0400ea842bc75a98cb9aefca99ced4a16e518cfd43'
SOURCE='f2a03e9797857bb0c1b9a671f2b32e85a536c545'
TREE='89512738dbe24dea56b41fa61b5729d62a29b8f9'
REPO='ghbarker/nuvio-native-legacy'
OLD_JOBS={'native':116680430003,'fontepref':116680429894,'fontepref_file':116680430238,'player_retido':116680430193,'parallel':116680430347,'collection':116680430187}
ALLOWED={'.github/workflows/pr527-android-recovery.yml','.review/pr527-recovery/origins.py','.review/pr527-recovery/aggregate.py'}

def api(path):
    request=urllib.request.Request('https://api.github.com/repos/'+REPO+'/'+path,headers={'Authorization':'Bearer '+os.environ['GH_TOKEN'],'Accept':'application/vnd.github+json','X-GitHub-Api-Version':'2022-11-28'})
    with urllib.request.urlopen(request,timeout=60)as response:return json.load(response)

def check_recipe(here):
    sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
    assert sha(here/'plan.json')==PLAN_SHA
    plan=json.loads((here/'plan.json').read_text());assert plan['source_commit']==SOURCE and plan['source_tree']==TREE
    review=os.environ['GITHUB_SHA'];assert review!=OLD_REVIEW
    parent=subprocess.check_output(['git','show','-s','--format=%P',review],text=True).strip();assert parent==OLD_REVIEW
    changes=subprocess.check_output(['git','diff','--name-only',OLD_REVIEW,review],text=True).splitlines();assert set(changes)==ALLOWED
    for relative in ['plan.json',*plan['recipe_inputs']]:
        path=here/relative
        prior=subprocess.check_output(['git','show',OLD_REVIEW+':'+str(path.relative_to(Path.cwd()))])
        assert path.read_bytes()==prior,relative
    return plan

class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self,*args,**kwargs):return None

def fetch_zip(artifact,target):
    assert artifact['digest'].startswith('sha256:')
    request=urllib.request.Request('https://api.github.com/repos/'+REPO+'/actions/artifacts/'+str(artifact['id'])+'/zip',headers={'Authorization':'Bearer '+os.environ['GH_TOKEN'],'Accept':'application/vnd.github+json'})
    try:
        response=urllib.request.build_opener(NoRedirect).open(request,timeout=60)
    except urllib.error.HTTPError as error:
        assert error.code in [301,302,303,307,308]
        location=error.headers['Location'];assert location.startswith('https://')
        response=urllib.request.urlopen(location,timeout=120)
    target.mkdir(parents=True,exist_ok=False)
    with tempfile.TemporaryFile()as raw:
        digest=hashlib.sha256();size=0
        with response:
            while True:
                block=response.read(1024*1024)
                if not block:break
                digest.update(block);size+=len(block);raw.write(block)
        assert size==artifact['size_in_bytes']and 'sha256:'+digest.hexdigest()==artifact['digest']
        raw.seek(0)
        with zipfile.ZipFile(raw)as archive:
            assert sum(x.file_size for x in archive.infolist())<8*1024**3
            for item in archive.infolist():
                p=Path(item.filename);assert not p.is_absolute()and'..'not in p.parts
                assert (item.external_attr>>16)&0o170000!=0o120000
            archive.extractall(target)
    return {'id':artifact['id'],'name':artifact['name'],'zip_sha256':digest.hexdigest(),'zip_bytes':size,'api_digest':artifact['digest'],'workflow_run':artifact['workflow_run']}

def gather(here,incoming):
    plan=check_recipe(here);current=int(os.environ['GITHUB_RUN_ID']);review=os.environ['GITHUB_SHA'];assert current!=OLD_RUN
    old=api('actions/runs/'+str(OLD_RUN));new=api('actions/runs/'+str(current))
    assert old['head_sha']==OLD_REVIEW and new['head_sha']==review
    assert old['path']=='.github/workflows/pr527-parallel-public-review.yml'
    assert new['path']=='.github/workflows/pr527-android-recovery.yml'
    assert int(new['run_attempt'])==int(os.environ['GITHUB_RUN_ATTEMPT'])==1
    deadline=time.monotonic()+15*60
    while True:
        jobs={};unfinished=[]
        for name,job_id in OLD_JOBS.items():
            job=api('actions/jobs/'+str(job_id));assert job['run_id']==OLD_RUN
            assert job['head_sha']==OLD_REVIEW and job['run_attempt']==1
            if job['status']=='completed':assert job['conclusion']=='success',(name,job['conclusion'])
            else:unfinished.append(name)
            jobs[name]=job
        if not unfinished:break
        assert time.monotonic()<deadline,('Timed out waiting for original producers',unfinished)
        print('Waiting for original producer completion: '+','.join(unfinished),flush=True)
        time.sleep(15)
    old=api('actions/runs/'+str(OLD_RUN));assert old['head_sha']==OLD_REVIEW
    new_jobs=api('actions/runs/'+str(current)+'/jobs?filter=latest&per_page=100')['jobs']
    android=[x for x in new_jobs if x['name']=='Standard Android ARM32/ARM64 APK, actual JNI/DEX and 16KB pages'];assert len(android)==1
    job=android[0];assert job['status']=='completed'and job['conclusion']=='success'
    assert job['run_id']==current and job['head_sha']==review and job['run_attempt']==1
    jobs['android']=job
    artifacts={run:api('actions/runs/'+str(run)+'/artifacts?per_page=100')['artifacts']for run in [OLD_RUN,current]}
    origins={};incoming.mkdir(parents=True,exist_ok=True)
    for name in ['native','fontepref','fontepref_file','player_retido','android','parallel','collection']:
        run=current if name=='android'else OLD_RUN;commit=review if name=='android'else OLD_REVIEW
        artifact_name='pr527-'+name+'-'+commit;matches=[x for x in artifacts[run]if x['name']==artifact_name and not x['expired']];assert len(matches)==1,artifact_name
        artifact=matches[0];assert artifact['workflow_run']['id']==run and artifact['workflow_run']['head_sha']==commit
        zipped=fetch_zip(artifact,incoming/artifact_name)
        job=jobs[name]
        origins[name]={'review_commit':commit,'run_id':run,'run_attempt':1,'job_id':job['id'],'job_name':job['name'],'job_conclusion':job['conclusion'],'started_at':job['started_at'],'completed_at':job['completed_at'],'runner_name':job.get('runner_name'),'retained_without_reexecution':name!='android','artifact':zipped}
    return {'checked_at':datetime.datetime.now(datetime.timezone.utc).isoformat(),'source_commit':SOURCE,'source_tree':TREE,'plan_sha256':PLAN_SHA,'recovery_review_commit':review,'recovery_run_id':current,'producer_origins':origins,'original_workflow_outcome':old['conclusion'],'same_run_all_producers':False,'six_original_successes_retained':True,'receipts_rewritten':False}
