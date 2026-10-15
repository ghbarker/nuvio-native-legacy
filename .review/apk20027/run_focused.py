"""Fresh 20-case actual-C closures on exact materialized source layers."""
from pathlib import Path
import argparse,hashlib,json,os,platform,subprocess,sys,time
from require_plan import load_plan,verify_source,sha
p=argparse.ArgumentParser();p.add_argument('--layers',nargs='+',choices=['product','test'],required=True);p.add_argument('--source-root',type=Path,required=True);p.add_argument('--proof-root',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--original-fileiras',action='store_true');a=p.parse_args();here=Path(__file__).resolve().parent;plan,approved=load_plan(here/'plan.json');assert approved
out=a.output.resolve();out.mkdir(parents=True,exist_ok=True);records=[]
for layer in a.layers:
 s=(a.source_root/('source-'+layer)).resolve();pf=(a.proof_root/(layer+'-source.json')).resolve();proof=json.loads(pf.read_text());verify_source(plan,proof,layer,s)
 for mode in ['normal','asan']:
  destination=out/(layer+'-'+mode);cmd=[sys.executable,str(here/'focused/run.py'),'--source',str(s),'--out',str(destination)]+(['--sanitize']if mode=='asan'else[]);t=time.monotonic();subprocess.run(cmd,check=True);receipt=json.loads((destination/'receipt.json').read_text());assert receipt['all_passed']and len(receipt['records'])==21 and receipt['sanitized']==(mode=='asan')
  records.append({'layer':layer,'mode':mode,'source_commit':proof['source_commit'],'tree':proof['tree'],'source_proof_sha256':sha(pf),'receipt':str(destination.relative_to(out)/'receipt.json'),'receipt_sha256':sha(destination/'receipt.json'),'cases':20,'seconds':round(time.monotonic()-t,3)})
original=None
if a.original_fileiras:
 assert platform.system()=='Darwin'and a.layers==['product'];s=(a.source_root/'source-product').resolve();t=time.monotonic();r=subprocess.run(['bash','tests/fileiras.sh'],cwd=s,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT);log=out/'original-fileiras.log';log.write_text(r.stdout);assert r.returncode==0,r.stdout[-4000:];original={'passed':True,'source_commit':plan['sources']['product']['commit'],'script_sha256':sha(s/'tests/fileiras.sh'),'assertions_sha256':sha(s/'tests/fileiras.c'),'log_sha256':sha(log),'seconds':round(time.monotonic()-t,3)}
review=os.environ['GITHUB_SHA'];run=int(os.environ['GITHUB_RUN_ID']);attempt=int(os.environ['GITHUB_RUN_ATTEMPT']);assert run>0 and attempt>0
result={'passed':True,'review_commit':review,'run_id':run,'run_attempt':attempt,'plan_sha256':sha(here/'plan.json'),'platform':platform.system(),'fixture_manifest_sha256':plan['required_removed_addon_rows']['fixture_manifest_sha256'],'records':records,'original_fileiras':original,'prior_passes_reused':False};(out/'focused-verified.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({'passed':True,'invocations':len(records),'platform':platform.system()}))
