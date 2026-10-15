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
 path=HERE.parent/'plan.json';p=json.loads(path.read_text()); assert p['format']==1
 assert p.get('frozen') is True and p.get('root_reviewed') is True,'Root has not approved the exact final composition'
 assert p.get('source_frozen') is True and p.get('required_pointer_regression',{}).get('bound') is True,'Source/pointer regression binding is incomplete'
 assert p['required_pointer_regression'].get('all_new_geometry_coverage_bound') is True,'Portrait reachability graph is not bound'
 assert p['required_pointer_regression'].get('async_row_load_regression_bound') is True,'Asynchronous Showcase row-load graph is not bound'
 assert p.get('required_settings_style_regression',{}).get('bound') is True and p['required_settings_style_regression'].get('fixture_manifest_sha256'),'Settings Subtitle style entry graph is not bound'
 assert p.get('required_synopsis_percentage_regression',{}).get('bound') is True and p['required_synopsis_percentage_regression'].get('fixture_manifest_sha256'),'Synopsis percentage/migration graph is not bound'
 assert p.get('required_collection_identity_regression',{}).get('bound') is True and p['required_collection_identity_regression'].get('fixture_manifest_sha256'),'Cold-start collection identity graph is not bound'
 expected=os.environ.get('NUVIO_REVIEW_PLAN_SHA256','')
 assert re.fullmatch(r'[0-9a-f]{64}',expected) and file_sha(path)==expected,'Explicit approved plan SHA256 missing or mismatched'
 assert p['identity']['version_code']==20026
 assert p['identity']['version_name']=='2.0.26-test-logs16'
 assert p['identity']['package']=='space.nuvio.nativelegacy.touch'
 assert p['identity']['label']=='Nuvio TV/Mobile Test'
 assert p['identity']['certificate_sha256']=='862d7ef63a0dbde2929f6f231cae295510726d9ac28f9292496d2a38304be5e6'
 assert re.fullmatch(r'[0-9a-f]{64}',p['bundle']['sha256'] or ''),'Source bundle pin missing'
 for layer in ('product','test'):
  source=p['sources'][layer]
  for field in ('commit','tree'):assert re.fullmatch(r'[0-9a-f]{40}',source[field] or ''),'Source '+field+' pin missing'
  assert re.fullmatch(r'[0-9a-f]{64}',source['manifest_sha256'] or ''),'Source manifest pin missing'
  manifest=(HERE.parent/source['manifest']).resolve();assert manifest.is_relative_to(HERE.parent.resolve())
  assert file_sha(manifest)==source['manifest_sha256'],'Source manifest differs from approved plan'
 return p

def binding(source,proof,layer):
 source=Path(source).resolve(); p=plan(); v=json.loads(Path(proof).read_text()); e=p['sources'][layer]
 assert v['verified'] and v['source_clean'] and v['original_tests_tree_unchanged']
 assert not v.get('review_only',False),'Review-only materialization cannot execute Android builds'
 if os.getenv('GITHUB_SHA'): assert v['review_commit']==os.environ['GITHUB_SHA'],'Source proof belongs to a different CI review commit'
 assert v['layer']==layer and v['source_commit']==e['commit'] and v['tree']==e['tree']
 assert v['manifest_sha256']==e['manifest_sha256'] and v['bundle_sha256']==p['bundle']['sha256']
 def git(*args): return subprocess.check_output(['git','-C',str(source),*args],text=True).strip()
 assert git('rev-parse','HEAD')==e['commit'] and git('rev-parse','HEAD^{tree}')==e['tree']
 mpath=HERE.parent/e['manifest'];assert file_sha(mpath)==e['manifest_sha256'];m=json.loads(mpath.read_text())
 assert git('rev-parse','HEAD:tests')==m['tests_tree'] and m['tests_tree_unchanged']
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
  if e['path'] not in expected: assert file_sha(Path(source)/e['path'])==e['sha256'],e['path']
 return p,v,m,r
