"""Publish only the exact already-approved Logs18 APK and validation JSON."""
from pathlib import Path
import hashlib,json,os,urllib.error,urllib.parse,urllib.request,zipfile
REPO='ghbarker/nuvio-native-legacy';API='https://api.github.com/repos/'+REPO
REVIEW='707b8d6846b70b4a6c30b35381f30a070d085553';REVIEW_TREE='742038dccda92eb315d6565b3b891801f0615626'
SOURCE='412d6bdc2a9eb6b3483626f765833e27fb82299d';SOURCE_TREE='c3277e06125065ea4c5144d67bc88cba7b3dd1b3'
PLAN='2971b573be6f26f0944ead56a085063c60979e0922a7fa08e21f968b1f5e4187';RUN=38814136744;ATTEMPT=1
JOBS={116694430577:'Focused artwork quality (linux)',116694430684:'Focused artwork quality (macos)',116694430703:'APK20028 compatible test APK (ARM32/ARM64)',116698052729:'All scoped Logs18 gates'}
ANDROID={'id':12010970407,'name':'apk20028-android-'+REVIEW,'bytes':54358565,'sha256':'a3cd1e21938ce5160fd4cfe6e5b3dfcc1010dc2bf9578a29cf1863e21e2fd59f'}
AGGREGATE={'id':12010028139,'name':'apk20028-all-gates-'+REVIEW,'bytes':1501,'sha256':'183e8d50903cb77c7c89ff4ade6b011f94fc28693a1689315f01b4f93f6dc41e'}
APK_NAME='Nuvio-TV-Mobile-Test-Logs18.apk';APK_BYTES=59005344;APK_SHA='2c7d7155350b2a2db4202fa49229e56c250af09308a14a85e379aa68d5f2e0b7'
VALIDATION_NAME='Logs18-validation.json';VALIDATION_SHA='c4420a422f835748d3f9cf26be9da34f4899992f14f27f075ebacdcde7249e57'
BUILD_SHA='b17f219765bef39c27cfa0523810ed98023b54708556712d08dcb04159d5020b';APK_PROOF_SHA='cd1e0139618cc3d177f70a3023d9bffdc8ceb815fd5ddcb625100f5990d7b7e3';SOURCE_PROOF_SHA='9aa960abada3bb565cbe6472e919633cac409f60d79342df76287d66c7683fd8'
PACKAGE='space.nuvio.nativelegacy.touch';CERT='862d7ef63a0dbde2929f6f231cae295510726d9ac28f9292496d2a38304be5e6'
TAG='tv-mobile-test-logs-v18';KNOWN_DRAFT=None
NOTES="Logs18 makes native full-screen Metahub artwork use the larger version when Image quality is set to High, preserving the selected picture. This is the focused test for blurry title/recommendation backgrounds in #503. The Logs17 removed-addon row-settings fix (#504/#365) is retained unchanged.\n\nThe package and persistent signing key match the earlier test builds, so Update retains accounts and settings.\n\nTo test:\n1. Install this APK over Nuvio TV/Mobile Test and choose Update.\n2. In Settings, find Image quality, select High and confirm. Under Posters and trailers → Trailers, turn Auto-play trailer Off temporarily. Use Settings search if the category differs on your device.\n3. Search for The End of Oak Street, open its title page, and wait at least 10 seconds. Check the background picture.\n4. Open the affected title through Recommendations and wait at least 10 seconds again. Check whether its background is clear.\n5. Download a fresh log immediately after both checks and share it with the result. Include which route was blurry, if either one still is.\n\nYou can restore Auto-play trailer after checking the still pictures.\n\nValidation: [all four scoped CI jobs passed](https://github.com/ghbarker/nuvio-native-legacy/actions/runs/38814136744). Fresh native/browser artwork-policy checks passed normally and under ASan/UBSan on Linux and macOS. The actual ARM32/ARM64 APK passed package, persistent signing key, JNI/DEX and 16 KB page checks. Private image diagnostics were checked separately on Linux.\n\nSHA-256: `2c7d7155350b2a2db4202fa49229e56c250af09308a14a85e379aa68d5f2e0b7`\n"
class SafeRedirect(urllib.request.HTTPRedirectHandler):
 def redirect_request(self,req,fp,code,msg,headers,newurl):
  result=super().redirect_request(req,fp,code,msg,headers,newurl)
  if result is not None and urllib.parse.urlsplit(req.full_url).netloc!=urllib.parse.urlsplit(newurl).netloc:result.remove_header('Authorization')
  return result
opener=urllib.request.build_opener(SafeRedirect())
def digest(data):return hashlib.sha256(data).hexdigest()
def request(url,method='GET',data=None,accept='application/vnd.github+json',content_type='application/json',allow404=False):
 headers={'Authorization':'Bearer '+os.environ['GITHUB_TOKEN'],'Accept':accept,'X-GitHub-Api-Version':'2022-11-28','User-Agent':'Logs18-approved-publisher'}
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

FROZEN_PINS_READY=True
ROOT_PUBLICATION_APPROVED=True

def main():
 assert FROZEN_PINS_READY and ROOT_PUBLICATION_APPROVED,'Publication remains closed until actual byte pins and root review'
 assert os.environ.get('GITHUB_TOKEN')and os.environ['GITHUB_REPOSITORY']==REPO
 work=Path('build/logs18-publication');work.mkdir(parents=True,exist_ok=False)
 run=api('actions/runs/'+str(RUN));assert run['head_sha']==REVIEW and run['run_attempt']==ATTEMPT and run['status']=='completed'and run['conclusion']=='success'
 for job_id,name in JOBS.items():
  job=api('actions/jobs/'+str(job_id));assert job['run_id']==RUN and job['run_attempt']==ATTEMPT and job['name']==name and job['status']=='completed'and job['conclusion']=='success'
 assert api('git/commits/'+REVIEW)['tree']['sha']==REVIEW_TREE
 assert api('git/commits/'+SOURCE)['tree']['sha']==SOURCE_TREE
 android=work/'android.zip';aggregate=work/'aggregate.zip';download(ANDROID,android);download(AGGREGATE,aggregate)
 with zipfile.ZipFile(aggregate)as z:validation=member(z,'all-gates-verified.json')
 assert digest(validation)==VALIDATION_SHA;verified=json.loads(validation);assert verified['all_passed']and verified['review_commit']==REVIEW and verified['run_id']==RUN and verified['run_attempt']==ATTEMPT and verified['plan_sha256']==PLAN
 assert verified['sources']['test']['commit']==SOURCE and verified['sources']['test']['tree']==SOURCE_TREE and verified['identity']['package']==PACKAGE and verified['identity']['certificate_sha256']==CERT and verified['identity']['version_code']==20028
 assert {x['job']for x in verified['jobs']}=={'focused-linux','focused-macos','android'}and not verified['previous_passes_count_as_fresh']
 assert verified['sources']['product']['commit']=='4b216b49cf0f5030c47168c458a49fec27836a1a'and verified['sources']['product']['tree']=='24204199e0b2d4dfee6eb0649b95b81cb8e77918'
 with zipfile.ZipFile(android)as z:
  apk=member(z,'android-test/Nuvio-2.0.28-test-logs18-tv-mobile-test.apk');build=member(z,'android-test/build-receipt.json');proof=member(z,'android-test/android-verified.json');source=member(z,'test-source.json')
 assert len(apk)==APK_BYTES and digest(apk)==APK_SHA and digest(build)==BUILD_SHA and digest(proof)==APK_PROOF_SHA and digest(source)==SOURCE_PROOF_SHA
 b=json.loads(build);p=json.loads(proof);s=json.loads(source);a=next(x for x in verified['jobs']if x['job']=='android');assert a['apk_sha256']==APK_SHA and a['apk_bytes']==APK_BYTES and a['proof_sha256']==BUILD_SHA and a['verification_sha256']==APK_PROOF_SHA
 assert b['verified']and b['layer']=='test'and b['source_commit']==SOURCE and b['review_commit']==REVIEW and b['apk_sha256']==APK_SHA and b['plan_sha256']==PLAN and b['source_restored_clean']and b['fresh_build_of_both_arms']and not b['prior_apk_proofs_reused']
 assert p['outcome']=='passed'and p['apk_sha256']==APK_SHA and p['package']==PACKAGE and p['version_code']==20028 and p['certificate_sha256']==CERT and p['native_16kb_pages']=='passed'and{x['abi']for x in p['actual_arm_compiles']}=={'arm64-v8a','armeabi-v7a'}
 assert s['verified']and s['source_clean']and not s['review_only']and s['original_tests_tree_unchanged']and s['source_commit']==SOURCE and s['tree']==SOURCE_TREE and s['review_commit']==REVIEW
 assets={APK_NAME:('application/vnd.android.package-archive',apk),VALIDATION_NAME:('application/json',validation)}
 tag_ok();release=existing_release()
 if release is None:release=target_ok(api('releases','POST',{'tag_name':TAG,'target_commitish':SOURCE,'name':'Nuvio TV/Mobile Test — Logs18','body':NOTES,'draft':True,'prerelease':True}))
 existing=api('releases/'+str(release['id'])+'/assets?per_page=100');assert len(existing)<=2 and {x['name']for x in existing}<=set(assets),'Unexpected release assets'
 if not release['draft']:assert {x['name']for x in existing}==set(assets),'Published release is incomplete'
 by_name={x['name']:x for x in existing};assert len(by_name)==len(existing),'Duplicate release assets'
 for name,(mime,data)in assets.items():
  if name in by_name:verify_asset(by_name[name],data);continue
  assert release['draft'],'Do not mutate an existing published release'
  upload=release['upload_url'].split('{',1)[0];parts=urllib.parse.urlsplit(upload);assert parts.scheme=='https'and parts.netloc=='uploads.github.com'and parts.path=='/repos/'+REPO+'/releases/'+str(release['id'])+'/assets'
  with request(upload+'?'+urllib.parse.urlencode({'name':name}),method='POST',data=data,content_type=mime)as response:asset=json.load(response)
  assert asset['name']==name;verify_asset(asset,data)
 final=target_ok(api('releases/'+str(release['id'])));found=api('releases/'+str(release['id'])+'/assets?per_page=100');assert len(found)==2 and{x['name']for x in found}==set(assets)
 if final['draft']:final=target_ok(api('releases/'+str(release['id']),'PATCH',{'draft':False,'prerelease':True,'make_latest':'false'}))
 assert not final['draft'];tag_ok(required=True)
 receipt={'published':True,'release_id':final['id'],'release_url':final['html_url'],'tag':TAG,'source_commit':SOURCE,'review_commit':REVIEW,'source_run':RUN,'source_run_attempt':ATTEMPT,'all_four_source_jobs_verified':True,'aggregate_sha256':VALIDATION_SHA,'apk_sha256':APK_SHA,'apk_bytes':APK_BYTES,'release_assets':sorted(assets),'publisher_review':os.environ['GITHUB_SHA'],'no_rebuild_or_test_rerun':True,'no_pr_writes':True}
 (work/'publication-receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
