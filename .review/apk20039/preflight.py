"""Closed recipe/object checks only; no compiler, SDK, APK or network."""
from pathlib import Path
import argparse,ast,hashlib,json,os,subprocess,sys
from importlib.util import spec_from_file_location,module_from_spec
from require_plan import load_plan
HERE=Path(__file__).resolve().parent
p=argparse.ArgumentParser();p.add_argument('--approved',action='store_true');p.add_argument('--base-repository',type=Path);a=p.parse_args()
plan,approved=load_plan(HERE/'plan.json',allow_review=not a.approved)
repo=(a.base_repository or Path(os.environ['GITHUB_WORKSPACE'])).resolve()
workflow=HERE/'apk20039-review.yml'
if not workflow.is_file():workflow=HERE.parents[1]/'.github/workflows/apk20039-review.yml'
w=json.loads(workflow.read_text());assert list(w['jobs'])==['android'] and w['permissions']=={'contents':'read'}
normalized=json.loads(json.dumps(w));normalized['jobs']['android']['if']=False
for env in [normalized['env'],normalized['jobs']['android']['env']]:env['NUVIO_REVIEW_PLAN_SHA256']='PENDING_FROZEN_SOURCE_AND_ROOT_REVIEW'
assert hashlib.sha256(json.dumps(normalized,sort_keys=True,separators=(',',':')).encode()).hexdigest()==plan['workflow_normalized_sha256']
assert w['jobs']['android']['if'] is approved
expected=hashlib.sha256((HERE/'plan.json').read_bytes()).hexdigest() if approved else 'PENDING_FROZEN_SOURCE_AND_ROOT_REVIEW'
for env in [w['env'],w['jobs']['android']['env']]:assert env['NUVIO_REVIEW_PLAN_SHA256']==expected
assert len([s for s in w['jobs']['android']['steps'] if s.get('run','').find('/android/run.py')>=0])==1
for path in HERE.rglob('*.py'):ast.parse(path.read_text(),filename=str(path))
spec=spec_from_file_location('logs29_cache_preflight',HERE/'ci-cache-preflight.py');mod=module_from_spec(spec);spec.loader.exec_module(mod);keys=mod.validate(w)
secret=[x for x in w['jobs']['android']['steps'] if x.get('name')=='Prepare private configuration and persistent debug signing'];assert len(secret)==1
assert secret[0]['env']=={'NUVIO_TOUCH_CORE_PROPERTIES':'${{ secrets.NUVIO_TOUCH_CORE_PROPERTIES }}','NUVIO_TOUCH_DEBUG_KEYSTORE_B64':'${{ secrets.NUVIO_TOUCH_DEBUG_KEYSTORE_B64 }}'}
proofdir=(Path(os.environ['GITHUB_WORKSPACE']) if os.getenv('GITHUB_WORKSPACE') else HERE.parent)/'review/object-preflight'
subprocess.run([sys.executable,str(HERE/'materialize.py'),'--layer','test','--verify-only','--allow-review-plan',
 '--base-repository',str(repo),'--proof',str(proofdir/'test-source.json')],check=True)
receipt={'passed':True,'approved_for_execution':approved,'source_frozen':plan['source_frozen'],
 'remote_source_commit':plan['sources']['test']['commit'],'source_tree':plan['sources']['test']['tree'],
 'cache_keys':keys,'single_private_apk_only':True,'local_full_checkout':False,'local_sdk_or_compiler':False}
proofdir.mkdir(parents=True,exist_ok=True);(proofdir/'preflight.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(json.dumps(receipt,indent=2))
