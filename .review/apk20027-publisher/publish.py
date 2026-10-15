"""Publish only the exact already-approved Logs17 APK and validation JSON."""
from pathlib import Path
import hashlib,json,os,urllib.error,urllib.parse,urllib.request,zipfile
REPO='ghbarker/nuvio-native-legacy';API='https://api.github.com/repos/'+REPO
REVIEW='509149a039de5d398335e33c4e45a07dd03e2551';REVIEW_TREE='177b968406b6e6f16e6d162965644648a8721b65'
SOURCE='813b65211ef6b0da05b0c7ad1462900c2f4d5596';SOURCE_TREE='11ba36380eed6e91f14f3f795a4d2496ec4ad16b'
PLAN='e0e94c8c4b3cfb9508a8c86e1e41f17feb17af2e2db61a4afc8233cf7e8fd2b9';RUN=38810104375;ATTEMPT=1
JOBS={116680887119:'APK20027 compatible test APK (ARM32/ARM64)',116680887567:'Focused removed-addon rows (macos)',116680887696:'Focused removed-addon rows (linux)',116683454636:'All scoped Logs17 gates'}
ANDROID={'id':12008770762,'name':'apk20027-android-'+REVIEW,'bytes':54354316,'sha256':'3c139bc78b51999d8344f06ee1cbc05706128183f6a4763f8dfbadc9f0453135'}
AGGREGATE={'id':12009020176,'name':'apk20027-all-gates-'+REVIEW,'bytes':1271,'sha256':'b3d2b833160633c75a887927b28e77471d2b0e82121bae37a4e0d683f5ba1d7e'}
APK_NAME='Nuvio-TV-Mobile-Test-Logs17.apk';APK_BYTES=59001572;APK_SHA='e6aaa965a6304269e348d1aabb651bf71df61d1dcba3172a4ddf6c24e4e6b37b'
VALIDATION_NAME='Logs17-validation.json';VALIDATION_SHA='944eda5b6a2d6ca5a004563ac77f663307e0d309576520298430dc90583049b8'
BUILD_SHA='f6ee252843bbde11425e32a997d680579725aae27dd1a5db9348af5ce336ae75';APK_PROOF_SHA='494dd7d305eed2c7cf0a0ae0a29aef3610de750099caf0cf06959f9c622a227e';SOURCE_PROOF_SHA='cd2c69ef2f0b356d0a880503b64029763b4ee0d086f0fb0f4fa885020036c3d8'
PACKAGE='space.nuvio.nativelegacy.touch';CERT='862d7ef63a0dbde2929f6f231cae295510726d9ac28f9292496d2a38304be5e6'
TAG='tv-mobile-test-logs-v17';KNOWN_DRAFT=414298266
NOTES='Logs17 adds the shared fix for issues #504 and #365: Home row settings hide catalogs belonging to confirmed removed addons while preserving saved row preferences for reinstalling the addon. Installed disabled addons remain selectable; unknown addon identities defer cleanup.\n\nInstall this APK over Nuvio TV/Mobile Test using Android’s Update button. The package and signing key match Logs16, so accounts and settings are retained.\n\nTo test:\n1. Use an addon that supplies Home catalog rows. Open Settings → Home rows, change one row’s style or size, and put another row under Not on Home.\n2. Remove that test addon from your Nuvio account, then refresh the app’s addons/catalogs and let discovery finish.\n3. Reopen Settings → Home rows. Its catalogs should appear in neither On Home nor Not on Home.\n4. Reinstall the same addon. The saved row choices and order should return.\n5. Disable and re-enable the addon without removing it. Its rows should remain available under Not on Home while disabled and return when enabled.\n\nValidation: [all four scoped CI jobs passed](https://github.com/ghbarker/nuvio-native-legacy/actions/runs/38810104375). This includes fresh 20-case normal/ASan/UBSan checks on the exact product/test sources, macOS row tests, and the actual ARM32/ARM64 APK with package, persistent certificate, JNI/DEX and 16 KB page checks. Playback/JVM code is unchanged from Logs16; those suites were not repeated.\n\nSHA-256: `e6aaa965a6304269e348d1aabb651bf71df61d1dcba3172a4ddf6c24e4e6b37b`\n\nThe separate blurry recommendation backdrop report (#503) is still under investigation; Logs17 does not add another fix for it.\n'
class SafeRedirect(urllib.request.HTTPRedirectHandler):
 def redirect_request(self,req,fp,code,msg,headers,newurl):
  result=super().redirect_request(req,fp,code,msg,headers,newurl)
  if result is not None and urllib.parse.urlsplit(req.full_url).netloc!=urllib.parse.urlsplit(newurl).netloc:result.remove_header('Authorization')
  return result
opener=urllib.request.build_opener(SafeRedirect())
def digest(data):return hashlib.sha256(data).hexdigest()
def request(url,method='GET',data=None,accept='application/vnd.github+json',content_type='application/json',allow404=False):
 headers={'Authorization':'Bearer '+os.environ['GITHUB_TOKEN'],'Accept':accept,'X-GitHub-Api-Version':'2022-11-28','User-Agent':'Logs17-approved-publisher'}
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
 with request(API+'/actions/artifacts/'+str(spec['id'])+'/zip',accept='application/octet-stream')as response,out.open('xb')as target:
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
 assert os.environ.get('GITHUB_TOKEN')and os.environ['GITHUB_REPOSITORY']==REPO
 work=Path('build/logs17-publication');work.mkdir(parents=True,exist_ok=False)
 run=api('actions/runs/'+str(RUN));assert run['head_sha']==REVIEW and run['run_attempt']==ATTEMPT and run['status']=='completed'and run['conclusion']=='success'
 for job_id,name in JOBS.items():
  job=api('actions/jobs/'+str(job_id));assert job['run_id']==RUN and job['run_attempt']==ATTEMPT and job['name']==name and job['status']=='completed'and job['conclusion']=='success'
 assert api('git/commits/'+REVIEW)['tree']['sha']==REVIEW_TREE
 assert api('git/commits/'+SOURCE)['tree']['sha']==SOURCE_TREE
 android=work/'android.zip';aggregate=work/'aggregate.zip';download(ANDROID,android);download(AGGREGATE,aggregate)
 with zipfile.ZipFile(aggregate)as z:validation=member(z,'all-gates-verified.json')
 assert digest(validation)==VALIDATION_SHA;verified=json.loads(validation);assert verified['all_passed']and verified['review_commit']==REVIEW and verified['run_id']==RUN and verified['run_attempt']==ATTEMPT and verified['plan_sha256']==PLAN
 assert verified['sources']['test']['commit']==SOURCE and verified['sources']['test']['tree']==SOURCE_TREE and verified['identity']['package']==PACKAGE and verified['identity']['certificate_sha256']==CERT and verified['identity']['version_code']==20027
 assert {x['job']for x in verified['jobs']}=={'focused-linux','focused-macos','android'}and not verified['previous_passes_count_as_fresh']
 with zipfile.ZipFile(android)as z:
  apk=member(z,'android-test/Nuvio-2.0.27-test-logs17-tv-mobile-test.apk');build=member(z,'android-test/build-receipt.json');proof=member(z,'android-test/android-verified.json');source=member(z,'test-source.json')
 assert len(apk)==APK_BYTES and digest(apk)==APK_SHA and digest(build)==BUILD_SHA and digest(proof)==APK_PROOF_SHA and digest(source)==SOURCE_PROOF_SHA
 b=json.loads(build);p=json.loads(proof);s=json.loads(source);a=next(x for x in verified['jobs']if x['job']=='android');assert a['apk_sha256']==APK_SHA and a['apk_bytes']==APK_BYTES and a['proof_sha256']==BUILD_SHA and a['verification_sha256']==APK_PROOF_SHA
 assert b['verified']and b['layer']=='test'and b['source_commit']==SOURCE and b['review_commit']==REVIEW and b['apk_sha256']==APK_SHA and b['plan_sha256']==PLAN and b['source_restored_clean']and b['fresh_build_of_both_arms']and not b['prior_apk_proofs_reused']
 assert p['outcome']=='passed'and p['apk_sha256']==APK_SHA and p['package']==PACKAGE and p['version_code']==20027 and p['certificate_sha256']==CERT and p['native_16kb_pages']=='passed'and{x['abi']for x in p['actual_arm_compiles']}=={'arm64-v8a','armeabi-v7a'}
 assert s['verified']and s['source_clean']and not s['review_only']and s['original_tests_tree_unchanged']and s['source_commit']==SOURCE and s['tree']==SOURCE_TREE and s['review_commit']==REVIEW
 assets={APK_NAME:('application/vnd.android.package-archive',apk),VALIDATION_NAME:('application/json',validation)}
 tag_ok();release=existing_release()
 if release is None:release=target_ok(api('releases','POST',{'tag_name':TAG,'target_commitish':SOURCE,'name':'Nuvio TV/Mobile Test — Logs17','body':NOTES,'draft':True,'prerelease':True}))
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
