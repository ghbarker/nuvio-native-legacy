"""Closed source and execution bindings for the scoped Logs17 review."""
from pathlib import Path
import hashlib,json,os,subprocess

def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def git(s,*a):return subprocess.check_output(['git','-C',str(s),*a],text=True).strip()
def load_plan(path,allow_review=False):
 path=Path(path).resolve();p=json.loads(path.read_text());assert p['format']==1 and p['source_frozen']
 c=p.get('required_removed_addon_rows',{});assert c.get('bound')is True and c.get('cases')==20
 assert sha(path.parent/c['fixture_manifest'])==c['fixture_manifest_sha256']
 f=json.loads((path.parent/c['fixture_manifest']).read_text())
 for name,h in f['files'].items():assert sha(path.parent/'focused'/name)==h,name
 assert sha(path.parent/p['composition_manifest']['path'])==p['composition_manifest']['sha256']
 for name,h in p.get('recipe_pins',{}).items():assert sha(path.parent/name)==h,name
 approved=all(p.get(k)is True for k in ['frozen','root_reviewed','ci_execution_permitted'])
 assert approved or allow_review,'Root review is required before execution'
 if approved:assert sha(path)==os.getenv('NUVIO_REVIEW_PLAN_SHA256'),'Wrong reviewed plan hash'
 return p,approved

def verify_source(p,proof,layer,source):
 e=p['sources'][layer];assert proof['verified']and proof['source_clean']and not proof['review_only']and proof['original_tests_tree_unchanged']
 assert proof['layer']==layer and proof['source_commit']==e['commit']and proof['tree']==e['tree']
 assert proof['manifest_sha256']==e['manifest_sha256']and proof['bundle_sha256']==p['bundle']['sha256']and proof['base']==p['upstream']['commit']
 assert os.environ.get('GITHUB_SHA')and proof['review_commit']==os.environ['GITHUB_SHA']
 assert git(source,'rev-parse','HEAD')==e['commit']and git(source,'rev-parse','HEAD^{tree}')==e['tree']and not git(source,'status','--porcelain')
 assert git(source,'rev-parse','HEAD:tests')==git(source,'rev-parse',proof['base']+':tests')
