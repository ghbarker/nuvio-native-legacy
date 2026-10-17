"""Bind one private Logs31 font-import source; only root can open the closed CI plan."""
from pathlib import Path
import hashlib,json,os,re

def sha(path): return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def load_plan(path,allow_review=False):
 path=Path(path).resolve();p=json.loads(path.read_text());root=path.parent
 assert p['format']==3 and p['identity']['version_code']==20041
 assert p['identity']['version_name']=='2.0.41-test-logs31'
 assert p['identity']['package']=='space.nuvio.nativelegacy.touch'
 assert p['identity']['certificate_sha256']=='862d7ef63a0dbde2929f6f231cae295510726d9ac28f9292496d2a38304be5e6'
 assert p['scope']['android_test_apk_only'] and p['scope']['private_overlay_never_public']
 assert list(p['sources'])==['test'] and p['required_jobs']==['android']
 for name,digest in p['recipe_pins'].items():
  file=(root/name).resolve();assert file.is_relative_to(root) and sha(file)==digest,name
 assert sha(root/p['composition_manifest']['path'])==p['composition_manifest']['sha256']
 comp=json.loads((root/p['composition_manifest']['path']).read_text());assert comp['sources']==p['sources']
 assert p['source_frozen'], 'Final Logs31 source is not frozen; this template cannot execute'
 source=p['sources']['test']
 for key in ('commit','tree'):assert re.fullmatch('[0-9a-f]{40}',source[key]),key
 assert source['tree']==p['local_trace']['tree']==comp['final_source_tree']
 assert sha(root/source['manifest'])==source['manifest_sha256']
 approved=all(p.get(k) is True for k in ['frozen','root_reviewed','source_frozen','ci_execution_permitted'])
 if approved:
  assert not p['pending'] and p['publication_permitted'] is False
  assert sha(path)==os.getenv('NUVIO_REVIEW_PLAN_SHA256'),'Wrong exact reviewed plan'
 else:assert allow_review,'Closed source/recipe cannot execute'
 return p,approved
