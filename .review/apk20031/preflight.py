"""Review-only Git-object preflight; no local checkout, compiler, SDK or APK."""
from pathlib import Path
import argparse,ast,hashlib,json,os,subprocess,sys,base64
from require_plan import load_plan
from importlib.util import spec_from_file_location,module_from_spec
HERE=Path(__file__).resolve().parent
p=argparse.ArgumentParser();p.add_argument('--approved',action='store_true');p.add_argument('--base-repository',type=Path);a=p.parse_args()
plan,approved=load_plan(HERE/'plan.json',allow_review=not a.approved)
repo=(a.base_repository or Path(os.getenv('GITHUB_WORKSPACE',str(HERE.parent/'repository')))).resolve()
def git(*args):return subprocess.check_output(['git','-C',str(repo),*args],stderr=subprocess.PIPE)
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
workflow=HERE/'apk20031-review.yml'
if not workflow.is_file(): workflow=HERE.parents[1]/'.github/workflows/apk20031-review.yml'
w=json.loads(workflow.read_text());assert list(w['jobs'])==['android']
assert w['permissions']=={'contents':'read'}
normalized=json.loads(json.dumps(w));normalized['jobs']['android']['if']=False
for env in [normalized['env'],normalized['jobs']['android']['env']]:env['NUVIO_REVIEW_PLAN_SHA256']='PENDING_FROZEN_SOURCE_AND_ROOT_REVIEW'
assert hashlib.sha256(json.dumps(normalized,sort_keys=True,separators=(',',':')).encode()).hexdigest()==plan['workflow_normalized_sha256']
if approved:
 assert w['jobs']['android']['if'] is True
 for env in [w['env'],w['jobs']['android']['env']]:assert env['NUVIO_REVIEW_PLAN_SHA256']==sha(HERE/'plan.json')
else:
 assert w['jobs']['android']['if'] is False
 for env in [w['env'],w['jobs']['android']['env']]:assert env['NUVIO_REVIEW_PLAN_SHA256']=='PENDING_FROZEN_SOURCE_AND_ROOT_REVIEW'
for path in HERE.rglob('*.py'):ast.parse(path.read_text(),filename=str(path))
comp=json.loads((HERE/'composition-manifest.json').read_text());product=plan['sources']['product']['commit'];test=plan['sources']['test']['commit']
assert git('show','-s','--format=%P',test).decode().split()==[product]
for head in comp['six_pr_heads'].values():git('merge-base','--is-ancestor',head,product)
if comp['issue495']['head']:git('merge-base','--is-ancestor',comp['issue495']['head'],product)
assert comp['failover']['frozen'] and comp['failover']['base']==comp['base']
git('merge-base','--is-ancestor',comp['failover']['head'],product)
git('merge-base','--is-ancestor',comp['previous_logs20']['product'],product)
private_paths=git('diff','--name-only',product,test).decode().splitlines();assert private_paths==comp['private_overlay']['paths']
diff_args=['--full-index', '--no-ext-diff', '--no-textconv', '--no-color', '--no-renames', '--diff-algorithm=myers', '--no-indent-heuristic', '--src-prefix=a/', '--dst-prefix=b/']
assert comp['private_overlay']['canonical_diff_arguments']==diff_args
patch=git('diff',*diff_args,product,test);assert hashlib.sha256(patch).hexdigest()==comp['private_overlay']['patch_sha256']
changed=[x for x in patch.splitlines() if x.startswith((b'+',b'-')) and not x.startswith((b'+++',b'---'))]
assert hashlib.sha256(b'\n'.join(changed)).hexdigest()==comp['private_overlay']['normalized_changed_lines_sha256']
assert len(changed)==363,'Private logic changed from the reviewed Logs19 layer'
assert git('rev-parse',product+':tests')==git('rev-parse',test+':tests')
spec=spec_from_file_location('logs21_cache_preflight',HERE/'ci-cache-preflight.py');mod=module_from_spec(spec);spec.loader.exec_module(mod);keys=mod.validate(w)
secret=[x for x in w['jobs']['android']['steps'] if x.get('name')=='Prepare private configuration and persistent debug signing'];assert len(secret)==1
assert secret[0]['env']=={'NUVIO_TOUCH_CORE_PROPERTIES':'${{ secrets.NUVIO_TOUCH_CORE_PROPERTIES }}','NUVIO_TOUCH_DEBUG_KEYSTORE_B64':'${{ secrets.NUVIO_TOUCH_DEBUG_KEYSTORE_B64 }}'}
assert 'source.bundle.base64' in plan['bundle']['path']
proofdir=Path(os.getenv('GITHUB_WORKSPACE',str(HERE.parent)))/'review/object-preflight'
for layer in ['product','test']:
 subprocess.run([sys.executable,str(HERE/'materialize.py'),'--layer',layer,'--verify-only','--allow-review-plan',
  '--base-repository',str(repo),'--proof',str(proofdir/(layer+'.json'))],check=True)
receipt={'passed':True,'approved_for_execution':approved,'source_frozen':plan['source_frozen'],
 'source_product':product,'source_test':test,'plan_sha256':sha(HERE/'plan.json'),
 'cache_keys':keys,'private_hunks_match_reviewed_logs19_after_version':True,
 'product_tests_preserved_in_private':True,'local_full_checkout':False,'local_sdk_or_compiler':False}
proofdir.mkdir(parents=True,exist_ok=True);(proofdir/'preflight.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(json.dumps(receipt,indent=2))
