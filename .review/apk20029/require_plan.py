"""Closed exact-source binding for the Logs19 issue528 test APK."""
from pathlib import Path
import hashlib,json,os

def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def load_plan(path,allow_review=False):
 path=Path(path).resolve();p=json.loads(path.read_text())
 assert p['format']==1 and p['source_frozen']
 assert p['scope']['android_test_apk_only'] and p['scope']['only_new_product_change_is_issue528']
 assert sha(path.parent/p['composition_manifest']['path'])==p['composition_manifest']['sha256']
 for name,digest in p['recipe_pins'].items():assert sha(path.parent/name)==digest,name
 approved=all(p.get(k) is True for k in ['frozen','root_reviewed','ci_execution_permitted'])
 assert approved or allow_review,'Root review is required before execution'
 if approved:assert sha(path)==os.getenv('NUVIO_REVIEW_PLAN_SHA256'),'Wrong reviewed plan hash'
 return p,approved
