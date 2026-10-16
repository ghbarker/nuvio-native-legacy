"""Bind the exact Logs21 integration; pending source may be inspected, never built."""
from pathlib import Path
import hashlib,json,os,re

def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def load_plan(path,allow_review=False):
 path=Path(path).resolve();p=json.loads(path.read_text());root=path.parent
 assert p['format']==2 and p['identity']['version_code']==20031
 assert p['identity']['package']=='space.nuvio.nativelegacy.touch'
 assert p['identity']['version_name']=='2.0.31-test-logs21'
 assert p['identity']['certificate_sha256']=='862d7ef63a0dbde2929f6f231cae295510726d9ac28f9292496d2a38304be5e6'
 assert p['scope']['android_test_apk_only'] and p['scope']['private_overlay_never_public']
 for name,digest in p['recipe_pins'].items():
  file=(root/name).resolve();assert file.is_relative_to(root) and sha(file)==digest,name
 assert sha(root/p['composition_manifest']['path'])==p['composition_manifest']['sha256']
 comp=json.loads((root/p['composition_manifest']['path']).read_text());assert comp['sources']==p['sources']
 assert comp['base']==p['upstream']['commit']
 assert set(map(str,comp['six_pr_heads']))=={'431','538','540','541','543','546'}
 for source in p['sources'].values():
  assert re.fullmatch('[0-9a-f]{40}',source['commit']) and re.fullmatch('[0-9a-f]{40}',source['tree'])
  assert sha(root/source['manifest'])==source['manifest_sha256']
 approved=all(p.get(k) is True for k in ['frozen','root_reviewed','source_frozen','ci_execution_permitted'])
 if approved:
  assert not p['pending'] and comp['issue495']['frozen'] is True
  assert re.fullmatch('[0-9a-f]{40}',comp['issue495']['head'] or '')
  assert re.fullmatch('[0-9a-f]{40}',comp['issue495']['tree'] or '')
  assert comp['failover']['frozen'] and re.fullmatch('[0-9a-f]{40}',comp['failover']['head'])
  assert sha(path)==os.getenv('NUVIO_REVIEW_PLAN_SHA256'),'Wrong exact reviewed plan'
 else:assert allow_review,'Closed source/recipe cannot execute'
 return p,approved
