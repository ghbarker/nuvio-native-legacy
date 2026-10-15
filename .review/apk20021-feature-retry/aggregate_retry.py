#!/usr/bin/env python3
"""Preserve six successful producer origins and require a fresh focused-feature retry."""
from pathlib import Path,PurePosixPath
import argparse,hashlib,importlib.util,json,os,re,stat,subprocess,sys,zipfile
HERE=Path(__file__).resolve().parent
ORIGINAL=HERE.parent/'apk20021'
sys.path.insert(0,str(ORIGINAL))
from require_plan import load_plan,verify_inherited,git

def sha(p):
 h=hashlib.sha256()
 with Path(p).open('rb')as f:
  for b in iter(lambda:f.read(1024*1024),b''):h.update(b)
 return h.hexdigest()
def read(p):return json.loads(Path(p).read_text())
def write(p,v):Path(p).write_text(json.dumps(v,indent=2)+'\n')
def gh(repo,path):return json.loads(subprocess.check_output(['gh','api','repos/'+repo+'/'+path],text=True,timeout=60))
def validate_job(v,e,run,review):
 assert v['id']==e['job_id'] and v['run_id']==run and v['head_sha']==review
 assert v['name']==e['job_name'] and v['status']=='completed' and v['conclusion']=='success'
 assert v['run_attempt']==e['run_attempt']
def validate_artifact(v,e,run,review):
 assert v['id']==e['id'] and v['name']==e['name'] and v['size_in_bytes']==e['bytes']
 assert v['digest']=='sha256:'+e['sha256'] and not v['expired']
 assert v['workflow_run']['id']==run and v['workflow_run']['head_sha']==review

def download(repo,e,run,review,destination):
 v=gh(repo,'actions/artifacts/'+str(e['id']));validate_artifact(v,e,run,review)
 destination.mkdir(parents=True,exist_ok=True);archive=destination/(e['name']+'.zip')
 with archive.open('xb') as f:subprocess.run(['gh','api','repos/'+repo+'/actions/artifacts/'+str(e['id'])+'/zip'],stdout=f,check=True,timeout=180)
 assert archive.stat().st_size==e['bytes'] and sha(archive)==e['sha256']
 target=destination/e['name'];assert not target.exists()
 with zipfile.ZipFile(archive)as z:
  infos=z.infolist();assert len(infos)==len({i.filename for i in infos}) and sum(i.file_size for i in infos)<2*1024**3
  for i in infos:
   p=PurePosixPath(i.filename);assert not p.is_absolute() and '..'not in p.parts and not stat.S_ISLNK(i.external_attr>>16)
  target.mkdir();z.extractall(target)
 return v

def adapted_validator(out,lock):
 original=ORIGINAL/'aggregate.py';assert sha(original)==lock['original_aggregate_sha256']
 text=original.read_text();changes=[
 ('def __init__(self, root, plan, review, stage=HERE):','def __init__(self, root, plan, review, stage=HERE, included=JOBS):'),
 ('        for job, name in ARTIFACTS.items():\n','        for job, name in ARTIFACTS.items():\n            if job not in included:\n                continue\n')]
 for old,new in changes:assert text.count(old)==1;text=text.replace(old,new,1)
 p=out/'origin-selected-validator.py';p.write_text(text);compile(text,str(p),'exec')
 spec=importlib.util.spec_from_file_location('origin_selected_validator',p);module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
 return module,changes

def main():
 a=argparse.ArgumentParser();a.add_argument('--output',type=Path,required=True);a.add_argument('--repository',required=True);a.add_argument('--job-results-json',required=True);args=a.parse_args()
 assert sys.flags.optimize==0 and args.repository=='ghbarker/nuvio-native-legacy'
 results=json.loads(args.job_results_json);assert set(results)=={'feature-contracts'} and results['feature-contracts']['result']=='success'
 lock=read(HERE/'retry-lock.json');assert lock['frozen'] and lock['root_reviewed']
 assert sha(HERE/'retry-lock.json')==os.environ['APK20021_RETRY_LOCK_SHA256']
 plan,_=load_plan(ORIGINAL/'plan.json');assert sha(ORIGINAL/'plan.json')==lock['plan_sha256']
 oldplan=read(HERE/'original-plan.json');assert sha(HERE/'original-plan.json')==lock['original_plan_sha256']
 assert oldplan['frozen'] and oldplan['root_reviewed'] and oldplan['ci_execution_permitted']
 assert oldplan['sources']==plan['sources'] and oldplan['bundle']==plan['bundle'] and oldplan['composition_manifest']==plan['composition_manifest']
 for name,e in lock['sources'].items():assert e['commit']==plan['sources'][name]['commit'] and e['tree']==plan['sources'][name]['tree']
 for path,expected in lock['retry_script_sha256'].items():assert sha(HERE.parent/path)==expected,path
 review=os.environ['GITHUB_SHA'];run=int(os.environ['GITHUB_RUN_ID']);assert re.fullmatch('[0-9a-f]{40}',review) and git(HERE.parents[1],'rev-parse','HEAD')==review
 verify_inherited(plan,HERE.parents[1]);oldrun=lock['retained_run_id'];oldreview=lock['retained_review_commit']
 assert oldrun==38732464381 and oldreview=='3b7ea260e5401daf47d806a9a6ddb0d144c5dd08'
 previous=gh(args.repository,'actions/runs/'+str(oldrun));assert previous['head_sha']==oldreview and previous['status']=='completed' and previous['conclusion']=='failure' and previous['run_attempt']==lock['retained_run_attempt']
 current=gh(args.repository,'actions/runs/'+str(run));assert current['id']==run and current['head_sha']==review and current['head_branch']=='review/apk20021-feature-retry-20261015'
 assert current['run_attempt']==int(os.environ['GITHUB_RUN_ATTEMPT'])
 assert not args.output.exists();out=args.output.resolve();out.mkdir(parents=True);origins=[]
 retained=lock['retained'];assert len(retained)==6 and {e['scope']for e in retained}=={'native','original-fontepref','original-fontepref_file','original-player_retido','android','jvm'}
 assert len({e['job_id']for e in retained})==6 and len({e['artifact']['id']for e in retained})==6
 for e in retained:
  assert e['actual_success_verified'] is True and e['job_id']!=lock['excluded_failed_feature']['job_id'] and e['artifact']['id']!=lock['excluded_failed_feature']['artifact_id']
  job=gh(args.repository,'actions/jobs/'+str(e['job_id']));validate_job(job,dict(e,run_attempt=lock['retained_run_attempt']),oldrun,oldreview)
  artifact=download(args.repository,e['artifact'],oldrun,oldreview,out/'retained')
  origins.append({'scope':e['scope'],'run_id':oldrun,'run_attempt':job['run_attempt'],'review_commit':oldreview,'plan_sha256':lock['original_plan_sha256'],'job_id':job['id'],'job_conclusion':'success','artifact':e['artifact']})
 jobs=gh(args.repository,'actions/runs/'+str(run)+'/jobs?per_page=100');assert jobs['total_count']<=100
 found=[v for v in jobs['jobs']if v['name']==lock['fresh_feature_job_name']];assert len(found)==1
 freshjob=found[0];validate_job(freshjob,{'job_id':freshjob['id'],'job_name':lock['fresh_feature_job_name'],'run_attempt':current['run_attempt']},run,review)
 listed=gh(args.repository,'actions/runs/'+str(run)+'/artifacts?per_page=100');assert listed['total_count']<=100
 found=[v for v in listed['artifacts']if v['name']=='apk20021-features-'+review];assert len(found)==1
 art=found[0];assert art['digest'].startswith('sha256:') and re.fullmatch('[0-9a-f]{64}',art['digest'][7:])
 entry={'id':art['id'],'name':art['name'],'bytes':art['size_in_bytes'],'sha256':art['digest'][7:]};download(args.repository,entry,run,review,out/'fresh')
 origins.append({'scope':'feature-contracts','run_id':run,'run_attempt':freshjob['run_attempt'],'review_commit':review,'plan_sha256':lock['plan_sha256'],'job_id':freshjob['id'],'job_conclusion':'success','artifact':entry})
 module,changes=adapted_validator(out,lock);bound=dict(plan,_sha256=lock['plan_sha256']);oldbound=dict(oldplan,_sha256=lock['original_plan_sha256'])
 old=module.Aggregate(out/'retained',oldbound,oldreview,stage=ORIGINAL,included=[e['scope']for e in retained]);fresh=module.Aggregate(out/'fresh',bound,review,stage=ORIGINAL,included=['feature-contracts'])
 native,modules=old.native();original=old.originals(modules);jvm=old.jvm();android=old.android();features=fresh.features()
 value={'format':1,'outcome':'passed','review_commit':review,'github_run_id':str(run),'github_run_attempt':str(current['run_attempt']),'plan_sha256':lock['plan_sha256'],'retry_lock_sha256':sha(HERE/'retry-lock.json'),'bundle_sha256':plan['bundle']['sha256'],'source_bindings':plan['sources'],'producer_origins':origins,'native':native,'original_full_source':original,'jvm':jvm,'android':android,'features':features,'retained_failed_run_conclusion':'failure','excluded_failed_feature':lock['excluded_failed_feature'],'retained_records_rewritten':False,'original_plan_sha256':lock['original_plan_sha256'],'original_validator_sha256':lock['original_aggregate_sha256'],'validator_adapter_changes':changes,'artifact_inputs_sha256':{'retained':old.files,'fresh':fresh.files},'publish_release_refs_or_backend_actions':False,'scope':'Six exact successful original producers retained with original-plan proof context and unchanged bytes; all24focused feature groups freshly verified with the corrected complete real-producer closure and fresh plan. The original failed feature job, aggregate and overall run remain failures.'}
 write(out/'all-gates-verified.json',value);write(out/'validation-origin.json',{'repository':args.repository,'review_commit':review,'run_id':run,'plan_sha256':lock['plan_sha256'],'original_plan_sha256':lock['original_plan_sha256'],'producer_origins':origins,'sources':lock['sources'],'excluded_failed_feature':lock['excluded_failed_feature']})
 print('PASS six retained successful producers plus all24fresh focused feature groups')
if __name__=='__main__':main()
