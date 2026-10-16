"""Bound Android build helpers to the one materialized source proof."""
from pathlib import Path
import hashlib,json,subprocess,os,re
HERE=Path(__file__).resolve().parent
DEBUG_SIGNER = '''        getByName("debug") {
            storeFile = file(System.getenv("NUVIO_TEST_DEBUG_KEYSTORE") ?: error("Persistent test signing key missing"))
            storePassword = "android"
            keyAlias = "androiddebugkey"
            keyPassword = "android"
        }
'''


def sha(data): return hashlib.sha256(data).hexdigest()
def file_sha(path): return sha(Path(path).read_bytes())
def plan():
 import sys
 sys.path.insert(0,str(HERE.parent))
 from require_plan import load_plan
 value,approved=load_plan(HERE.parent/'plan.json')
 assert approved
 return value

def binding(source,proof,layer):
 source=Path(source).resolve(); p=plan(); v=json.loads(Path(proof).read_text()); e=p['sources'][layer]
 assert v['verified'] and v['source_clean'] and v['tests_tree_preserved'] is False
 assert v['existing_test_entries_preserved'] and v['reviewed_tests_delta_verified']
 assert not v.get('review_only',False) and v['materialization_mode']=='checkout','Object-only or review-only proof cannot build'
 if os.getenv('GITHUB_SHA'): assert v['review_commit']==os.environ['GITHUB_SHA'],'Source proof belongs to a different CI review commit'
 assert v['layer']==layer and v['source_commit']==e['commit'] and v['tree']==e['tree']
 assert v['manifest_sha256']==e['manifest_sha256'] and v['source_binding']=='exact-git-tree'
 def git(*args): return subprocess.check_output(['git','-C',str(source),*args],text=True).strip()
 assert git('rev-parse','HEAD')==e['commit'] and git('rev-parse','HEAD^{tree}')==e['tree']
 mpath=HERE.parent/e['manifest'];assert file_sha(mpath)==e['manifest_sha256'];m=json.loads(mpath.read_text())
 assert git('rev-parse','HEAD:tests')==m['tests_tree'] and m['tests_tree_preserved'] is False
 assert m['existing_test_entries_preserved'] and m['existing_test_entries']==1574
 return p,v,m,git

def transformed(source,proof,layer,receipt):
 p,v,m,git=binding(source,proof,layer);r=json.loads(Path(receipt).read_text())
 assert r['source_commit']==v['source_commit'] and r['tree']==v['tree'] and r['layer']==layer
 assert r['source_proof_sha256']==file_sha(proof),'Transform receipt source-proof binding mismatch'
 expected={x['path']:x for x in r['changes']}
 if layer=='test': assert r['changes']==[],'The pinned private test layer must remain unmodified'
 else:
  rel='android/app/build.gradle.kts'
  assert len(r['changes'])==1 and set(expected)=={rel},'Only the product debug signer insertion is allowed'
  original=subprocess.check_output(['git','-C',str(source),'show','HEAD:'+rel]).decode()
  marker='    signingConfigs {\n'
  assert original.count(marker)==1 and 'getByName("debug")' not in original and 'NUVIO_TEST_DEBUG_KEYSTORE' not in original
  approved=original.replace(marker,marker+DEBUG_SIGNER,1)
  assert expected[rel]['before_sha256']==sha(original.encode())
  assert expected[rel]['after_sha256']==sha(approved.encode()),'Unapproved product build-script mutation'
 assert set(git('diff','--name-only','HEAD').splitlines())==set(expected)
 for rel,e in expected.items():
  original=subprocess.check_output(['git','-C',str(source),'show','HEAD:'+rel]);assert sha(original)==e['before_sha256']
  assert file_sha(Path(source)/rel)==e['after_sha256']
 for e in m['files']:
  if e['path'] not in expected:
   if e.get('deleted'):assert not (Path(source)/e['path']).exists()
   else:assert file_sha(Path(source)/e['path'])==e['sha256'],e['path']
 return p,v,m,r
