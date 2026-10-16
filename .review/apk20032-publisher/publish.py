"""Publish only the approved Logs22 APK after Android and combined macOS closure."""
from pathlib import Path
import hashlib,io,json,os,re,urllib.error,urllib.parse,urllib.request,zipfile
HERE=Path(__file__).resolve().parent
PINS=json.loads((HERE/'pins.json').read_text())
REPO='ghbarker/nuvio-native-legacy';API='https://api.github.com/repos/'+REPO
REVIEW=PINS['review_commit'];REVIEW_TREE=PINS['review_tree']
SOURCE=PINS['source_commit'];SOURCE_TREE=PINS['source_tree']
PLAN=PINS['plan_sha256'];RUN=PINS['run_id'];ATTEMPT=PINS['run_attempt']
ANDROID=PINS['apk_artifact'];PROOF=PINS['proof_artifact']
APK_NAME='Nuvio-TV-Mobile-Test-Logs22.apk';APK_BYTES=PINS['apk_bytes'];APK_SHA=PINS['apk_sha256']
VALIDATION_NAME='Logs22-validation.json';VALIDATION_SHA=PINS['validation_sha256']
BUILD_SHA=PINS['build_receipt_sha256'];APK_PROOF_SHA=PINS['apk_verification_sha256'];SOURCE_PROOF_SHA=PINS['source_proof_sha256']
PACKAGE='space.nuvio.nativelegacy.touch';CERT='862d7ef63a0dbde2929f6f231cae295510726d9ac28f9292496d2a38304be5e6'
TAG='tv-mobile-test-logs-v22';KNOWN_DRAFT=None
NOTES=(HERE/'notes.md').read_text()
class SafeRedirect(urllib.request.HTTPRedirectHandler):
 def redirect_request(self,req,fp,code,msg,headers,newurl):
  result=super().redirect_request(req,fp,code,msg,headers,newurl)
  if result is not None and urllib.parse.urlsplit(req.full_url).netloc!=urllib.parse.urlsplit(newurl).netloc:result.remove_header('Authorization')
  return result
opener=urllib.request.build_opener(SafeRedirect())
def digest(data):return hashlib.sha256(data).hexdigest()
def request(url,method='GET',data=None,accept='application/vnd.github+json',content_type='application/json',allow404=False):
 headers={'Authorization':'Bearer '+os.environ['GITHUB_TOKEN'],'Accept':accept,'X-GitHub-Api-Version':'2022-11-28','User-Agent':'Logs22-approved-publisher'}
 if data is not None:headers['Content-Type']=content_type
 try:return opener.open(urllib.request.Request(url,data=data,headers=headers,method=method),timeout=180)
 except urllib.error.HTTPError as error:
  if allow404 and error.code==404:return None
  raise

def api(path,method='GET',body=None,allow404=False):
 response=request(API+'/'+path,method,None if body is None else json.dumps(body).encode(),allow404=allow404)
 if response is None:return None
 with response:return json.load(response)
def download(spec,out):
 metadata=api('actions/artifacts/'+str(spec['id']));assert metadata['name']==spec['name']and metadata['size_in_bytes']==spec['bytes']and metadata['digest']=='sha256:'+spec['sha256']and not metadata['expired']
 assert metadata['workflow_run']['id']==RUN and metadata['workflow_run']['head_sha']==REVIEW
 with request(API+'/actions/artifacts/'+str(spec['id'])+'/zip')as response,out.open('xb')as target:
  h=hashlib.sha256();n=0
  while True:
   part=response.read(1024*1024)
   if not part:break
   n+=len(part);assert n<=spec['bytes'];target.write(part);h.update(part)
 assert n==spec['bytes']and h.hexdigest()==spec['sha256'];return metadata

def member(z,suffix):
 names=[n for n in z.namelist()if n.endswith(suffix)];assert len(names)==1,suffix;return z.read(names[0])
def target_ok(release):
 assert release['tag_name']==TAG and release['target_commitish']==SOURCE,'Unexpected release target'
 assert release['draft']or release['prerelease']is True,'Unexpected stable release'
 return release

def tag_ok(required=False):
 ref=api('git/ref/tags/'+TAG,allow404=True)
 if ref is None:assert not required;return
 obj=ref['object']
 for unused in range(4):
  if obj['type']=='commit':break
  assert obj['type']=='tag';obj=api('git/tags/'+obj['sha'])['object']
 assert obj['type']=='commit'and obj['sha']==SOURCE,'Tag points to unapproved source'

def existing_release():
 if KNOWN_DRAFT is not None:
  known=api('releases/'+str(KNOWN_DRAFT),allow404=True)
  if known is not None:return target_ok(known)
 tagged=api('releases/tags/'+TAG,allow404=True)
 if tagged is not None:return target_ok(tagged)
 matches=[]
 for page in range(1,11):
  rows=api('releases?per_page=100&page='+str(page));matches.extend(x for x in rows if x['tag_name']==TAG)
  if len(rows)<100:break
 assert len(matches)<=1,'Ambiguous releases for approved tag'
 return target_ok(matches[0])if matches else None

def verify_asset(asset,data):
 assert asset['state']=='uploaded'and asset['size']==len(data)
 if asset.get('digest'):assert asset['digest']=='sha256:'+digest(data)
 with request(asset['url'],accept='application/octet-stream')as response:
  h=hashlib.sha256();n=0
  while True:
   part=response.read(1024*1024)
   if not part:break
   n+=len(part);assert n<=len(data);h.update(part)
 assert n==len(data)and h.hexdigest()==digest(data),'Existing or uploaded asset bytes differ'

def main():
 assert PINS['frozen_pins_ready'] is True and PINS['root_publication_approved'] is True,'Publication remains closed pending actual build byte pins and root review'
 assert os.environ.get('GITHUB_TOKEN') and os.environ['GITHUB_REPOSITORY']==REPO
 for name in ['review_commit','review_tree','source_commit','source_tree']:assert re.fullmatch('[0-9a-f]{40}',PINS[name]),name
 for name in ['plan_sha256','apk_sha256','validation_sha256','build_receipt_sha256','apk_verification_sha256','source_proof_sha256','composition_sha256','combined_macos_validation_sha256']:assert re.fullmatch('[0-9a-f]{64}',PINS[name]),name
 for spec in [ANDROID,PROOF]:assert spec['id']>0 and spec['bytes']>0 and re.fullmatch('[0-9a-f]{64}',spec['sha256'])
 composition=(HERE/'composition-manifest.json').read_bytes();assert digest(composition)==PINS['composition_sha256'];c=json.loads(composition)
 assert c['format']==3 and c['final_source_tree']==SOURCE_TREE and c['remote_source_only_commit']==SOURCE
 assert list(c['sources'])==['test'] and c['sources']['test']['commit']==SOURCE and c['sources']['test']['tree']==SOURCE_TREE
 assert c['private_overlay']['preserved'] and c['private_overlay']['never_pr'] and not c['remaining_conflicts']
 assert all(x['original_added_statements_preserved'] for x in c['private_overlay']['paths'].values())
 work=Path('build/logs22-publication');work.mkdir(parents=True,exist_ok=False)
 run=api('actions/runs/'+str(RUN));assert run['head_sha']==REVIEW and run['run_attempt']==ATTEMPT and run['status']=='completed' and run['conclusion']=='success'
 expected=PINS['job'];job=api('actions/jobs/'+str(expected['id']));assert job['run_id']==RUN and job['run_attempt']==ATTEMPT and job['name']==expected['name'] and job['status']=='completed' and job['conclusion']=='success'
 assert api('git/commits/'+REVIEW)['tree']['sha']==REVIEW_TREE
 source_commit=api('git/commits/'+SOURCE);assert source_commit['tree']['sha']==SOURCE_TREE and [x['sha'] for x in source_commit['parents']]==c['sources']['test']['parents']
 review_commit=api('git/commits/'+REVIEW);assert SOURCE in [x['sha'] for x in review_commit['parents']],'Reviewed build must retain the exact source-only parent'
 android=work/'apk.zip';proofzip=work/'proof.zip';download(ANDROID,android);download(PROOF,proofzip)
 with zipfile.ZipFile(android) as z:apk=member(z,'Nuvio-2.0.32-test-logs22-tv-mobile-test.apk')
 with zipfile.ZipFile(proofzip) as z:
  build=member(z,'android-test/build-receipt.json');proof=member(z,'android-test/android-verified.json');source=member(z,'test-source.json')
  before=member(z,'android-test/workspace/before.json');after=member(z,'android-test/workspace/after.json')
  facts={arm['abi']:[(item,member(z,'android-test/'+item['facts_file'])) for item in arm['builds']] for arm in json.loads(proof)['actual_arm_compiles']}
 assert len(apk)==APK_BYTES and digest(apk)==APK_SHA
 assert digest(build)==BUILD_SHA and digest(proof)==APK_PROOF_SHA and digest(source)==SOURCE_PROOF_SHA
 b=json.loads(build);p=json.loads(proof);s=json.loads(source)
 assert b['verified'] and b['layer']=='test' and b['source_commit']==SOURCE and b['tree']==SOURCE_TREE and b['review_commit']==REVIEW and b['apk_sha256']==APK_SHA and b['plan_sha256']==PLAN and b['source_restored_clean'] and b['fresh_build_of_both_arms'] and not b['prior_apk_proofs_reused']
 assert b['source_proof_sha256']==SOURCE_PROOF_SHA and b['verification_sha256']==APK_PROOF_SHA
 assert digest(before)==b['workspace_before_sha256'] and digest(after)==b['workspace_after_sha256']
 for phase,raw in [('before',before),('after',after)]:
  w=json.loads(raw);assert w['outcome']=='passed' and w['phase']==phase and w['source_commit']==SOURCE and w['source_tree']==SOURCE_TREE
 assert json.loads(after)['before_sha256']==digest(before)
 for abi,items in facts.items():
  assert items
  for item,raw in items:
   f=json.loads(raw);assert digest(raw)==item['facts_sha256'] and f['abi']==abi and f['source_abi_build_bound'] and f['sysroot_bound'] and f['private_native_opt_in'] and f['compiler']['ndk_version']=='27.2.12479018'
   assert item['source_abi_compiler_bound'] and item['private_native_opt_in'] and item['report_endpoint_configured'] and len(f['source_relative_units'])==item['main_sources']==316 and len(set(f['source_relative_units']))==316
 assert p['outcome']=='passed' and p['source_commit']==SOURCE and p['tree']==SOURCE_TREE and p['review_commit']==REVIEW and p['plan_sha256']==PLAN and p['apk_sha256']==APK_SHA and p['apk_bytes']==APK_BYTES and p['package']==PACKAGE and p['version_code']==20032 and p['version_name']=='2.0.32-test-logs22' and p['certificate_sha256']==CERT and p['native_16kb_pages']=='passed' and {x['abi'] for x in p['actual_arm_compiles']}=={'arm64-v8a','armeabi-v7a'} and p['private_log_opt_in'] and p['legacy_jni_overloads_preserved'] and p['file_provider']=={'authority':'space.nuvio.nativelegacy.touch.atualizacao','nonexported':True,'updater_path_only':True}
 assert s['verified'] and s['source_clean'] and not s['review_only'] and s['tests_tree_preserved'] and s['materialization_mode']=='checkout' and s['source_frozen'] and s['tests_tree']==c['tests_tree'] and s['source_commit']==SOURCE and s['expected_remote_source_commit']==SOURCE and s['tree']==SOURCE_TREE and s['review_commit']==REVIEW and s['source_binding']=='exact-git-tree'
 assert s['manifest_sha256']==c['sources']['test']['manifest_sha256']
 assert len(p['all_player_jni_lookups'])>=19 and len(p['all_player_jni_lookups'])==len(set(tuple(x) for x in p['all_player_jni_lookups']))
 assert p['compiled_android_classes'] and p['apk_dex_sha256']
 with zipfile.ZipFile(io.BytesIO(apk)) as z:
  for name,h in p['apk_dex_sha256'].items():assert digest(z.read(name))==h
 mac_raw=(HERE/'combined-macos-validation.json').read_bytes();assert digest(mac_raw)==PINS['combined_macos_validation_sha256']
 mac=json.loads(mac_raw);assert mac['verified'] is True and mac['all_passed'] is True and mac['validation_scope']=='composed','Combined macOS closure is still pending'
 assert mac['apk_source_commit']==SOURCE and mac['apk_source_tree']==SOURCE_TREE
 assert re.fullmatch('[0-9a-f]{40}',mac['final_test_candidate']) and re.fullmatch('[0-9a-f]{40}',mac['final_test_tree'])
 # The existing offline consumer verifies all real raw assertions/dependencies
 # and computes complete Git non-test identity, without relabelling APK source.
 covered_trees={SOURCE_TREE};covered_candidates=set()
 assert mac['source_bridges']
 for bridge in mac['source_bridges']:
  assert bridge['identical'] is True and bridge['non_test_entries']==2684 and re.fullmatch('[0-9a-f]{64}',bridge['non_test_tree_listing_sha256'])
  assert all(re.fullmatch('[0-9a-f]{40}',bridge[k]) for k in ['from_candidate','from_tree','to_candidate','to_tree'])
  assert bridge['from_tree'] in covered_trees,'Unconnected source-identity bridge'
  covered_trees.add(bridge['to_tree']);covered_candidates.add(bridge['to_candidate'])
 assert mac['final_test_tree'] in covered_trees and mac['final_test_candidate'] in covered_candidates
 assert mac['lane_count']==len(mac['lanes'])==12 and mac['public_mode_groups']==40 and mac['actual_binary_executions']==130 and mac['full_tv_touch_objects']==638 and mac['full_tv_touch_links']==2 and mac['project_dependencies_checked']>0
 origins=set()
 for name,row in mac['lanes'].items():
  assert row['passed'] is True and row['source_tree'] in covered_trees
  assert all(re.fullmatch('[0-9a-f]{64}',row[k]) for k in ['plan_sha256','receipt_sha256']) and re.fullmatch('[0-9a-f]{40}',row['review_commit'])
  origin=(int(row['run_id']),int(row['attempt']),int(row['job_id']));assert origin not in origins;origins.add(origin)
  mr=api('actions/runs/'+str(row['run_id'])+'/attempts/'+str(row['attempt']));mj=api('actions/jobs/'+str(row['job_id']))
  assert mr['head_sha']==row['review_commit'] and str(mr['run_attempt'])==str(row['attempt'])
  assert mj['run_id']==int(row['run_id']) and str(mj['run_attempt'])==str(row['attempt']) and mj['head_sha']==row['review_commit'] and mj['name']=='macOS '+name and mj['status']=='completed' and mj['conclusion']=='success'
 validation=(HERE/VALIDATION_NAME).read_bytes();assert digest(validation)==VALIDATION_SHA;v=json.loads(validation)
 assert v['all_passed'] and v['source_commit']==SOURCE and v['source_tree']==SOURCE_TREE and v['review_commit']==REVIEW and v['run_id']==RUN and v['run_attempt']==ATTEMPT and v['plan_sha256']==PLAN and v['job']==expected
 assert v['apk_sha256']==APK_SHA and v['apk_bytes']==APK_BYTES and v['build_receipt_sha256']==BUILD_SHA and v['apk_verification_sha256']==APK_PROOF_SHA and v['source_proof_sha256']==SOURCE_PROOF_SHA
 assert v['previous_results_count_as_fresh'] is False and v['single_exact_private_source'] is True and v['private_overlay_preserved'] is True
 assert v['composition_sha256']==PINS['composition_sha256'] and v['combined_macos_validation_sha256']==PINS['combined_macos_validation_sha256']
 assets={APK_NAME:('application/vnd.android.package-archive',apk),VALIDATION_NAME:('application/json',validation)}
 tag_ok();release=existing_release()
 if release is None:release=target_ok(api('releases','POST',{'tag_name':TAG,'target_commitish':SOURCE,'name':'Nuvio TV/Mobile Test — Logs22','body':NOTES,'draft':True,'prerelease':True}))
 existing=api('releases/'+str(release['id'])+'/assets?per_page=100');assert len(existing)<=2 and {x['name'] for x in existing}<=set(assets),'Unexpected Logs22 release assets'
 if not release['draft']:assert {x['name'] for x in existing}==set(assets),'Published Logs22 release is incomplete'
 by_name={x['name']:x for x in existing};assert len(by_name)==len(existing)
 for name,(mime,data) in assets.items():
  if name in by_name:verify_asset(by_name[name],data);continue
  assert release['draft'],'Do not mutate an existing published release'
  upload=release['upload_url'].split('{',1)[0];parts=urllib.parse.urlsplit(upload);assert parts.scheme=='https' and parts.netloc=='uploads.github.com' and parts.path=='/repos/'+REPO+'/releases/'+str(release['id'])+'/assets'
  with request(upload+'?'+urllib.parse.urlencode({'name':name}),method='POST',data=data,content_type=mime) as response:asset=json.load(response)
  assert asset['name']==name;verify_asset(asset,data)
 final=target_ok(api('releases/'+str(release['id'])));found=api('releases/'+str(release['id'])+'/assets?per_page=100');assert len(found)==2 and {x['name'] for x in found}==set(assets)
 if final['draft']:final=target_ok(api('releases/'+str(release['id']),'PATCH',{'draft':False,'prerelease':True,'make_latest':'false'}))
 assert not final['draft'];tag_ok(required=True)
 receipt={'published':True,'release_id':final['id'],'release_url':final['html_url'],'tag':TAG,'source_commit':SOURCE,'review_commit':REVIEW,'source_run':RUN,'source_run_attempt':ATTEMPT,'fresh_android_job_verified':True,'combined_macos_validation_sha256':PINS['combined_macos_validation_sha256'],'combined_macos_lane_count':mac['lane_count'],'validation_sha256':VALIDATION_SHA,'apk_sha256':APK_SHA,'apk_bytes':APK_BYTES,'release_assets':sorted(assets),'publisher_review':os.environ['GITHUB_SHA'],'no_rebuild_or_test_rerun':True,'no_pr_writes':True,'other_release_assets_untouched':True}
 (work/'publication-receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
