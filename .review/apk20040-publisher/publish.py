"""Publish the exact private Logs30 installer-correction test APK after actual Android proof checks."""
from pathlib import Path
import hashlib,io,json,os,re,urllib.error,urllib.parse,urllib.request,zipfile
HERE=Path(__file__).resolve().parent
PINS=json.loads((HERE/'pins.json').read_text())
REPO='ghbarker/nuvio-native-legacy';API='https://api.github.com/repos/'+REPO
REVIEW=PINS['review_commit'];REVIEW_TREE=PINS['review_tree']
SOURCE=PINS['source_commit'];SOURCE_TREE=PINS['source_tree']
PLAN=PINS['plan_sha256'];RUN=PINS['run_id'];ATTEMPT=PINS['run_attempt']
SOURCE_UNITS=PINS['source_units_per_abi']
ANDROID=PINS['apk_artifact'];PROOF=PINS['proof_artifact']
APK_NAME='Nuvio-TV-Mobile-Test-Logs30.apk';APK_BYTES=PINS['apk_bytes'];APK_SHA=PINS['apk_sha256']
VALIDATION_NAME='Logs30-validation.json';VALIDATION_SHA=PINS['validation_sha256']
BUILD_SHA=PINS['build_receipt_sha256'];APK_PROOF_SHA=PINS['apk_verification_sha256'];SOURCE_PROOF_SHA=PINS['source_proof_sha256']
PACKAGE='space.nuvio.nativelegacy.touch';CERT='862d7ef63a0dbde2929f6f231cae295510726d9ac28f9292496d2a38304be5e6'
TAG='tv-mobile-test-logs-v30';KNOWN_DRAFT=None
NOTES=(HERE/'notes.md').read_text()
class SafeRedirect(urllib.request.HTTPRedirectHandler):
 def redirect_request(self,req,fp,code,msg,headers,newurl):
  result=super().redirect_request(req,fp,code,msg,headers,newurl)
  if result is not None and urllib.parse.urlsplit(req.full_url).netloc!=urllib.parse.urlsplit(newurl).netloc:result.remove_header('Authorization')
  return result
opener=urllib.request.build_opener(SafeRedirect())
def digest(data):return hashlib.sha256(data).hexdigest()
def request(url,method='GET',data=None,accept='application/vnd.github+json',content_type='application/json',allow404=False):
 headers={'Authorization':'Bearer '+os.environ['GITHUB_TOKEN'],'Accept':accept,'X-GitHub-Api-Version':'2022-11-28','User-Agent':'Logs30-approved-publisher'}
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

def verify_private_overlay(c):
 paths=c['private_overlay']['paths'];exception='src/posplay.c'
 assert {name for name,x in paths.items() if x['original_added_statements_preserved'] is not True}=={exception}
 assert SOURCE=='06ba8068be86299592cb94e77355266ebb57c09f' and SOURCE_TREE=='0b54962bd0c45cdf131c100e8bc26e4b68eb2951'
 assert paths[exception]=={'original_added_statements_preserved':False,'blob':'e0532c7c6e94d153a8b33c773de8e28184b8ec83','special_seasons_source_updated':True,'preservation_scope':'Diagnostic block retained; its pending-pair bit now accepts explicit season0 as required by the feature.'}
 assert c['inputs']['logs26_source_integration_sha256']==c['accepted_logs26_port']['source_integration_receipt_sha256']=='36c2b24f1e64858e7b25840e65687ad3255c5bf527a46285f4a97bd1e5581ade'
 assert c['inputs']['private_special_seasons_control_sha256']==c['accepted_logs26_port']['private_helper_control_sha256']=='54d59bd5701d72dc2484e4edc45cb84eb697e27df48f5039a246781024995d17'
 assert 'Private posplay uses equivalent explicit pair predicates in place, retaining tested actual getter bytes, rather than importing new public convenience helpers.' in c['accepted_logs26_port']['private_compatibility']['semantic_exceptions']

def verify_manifest_queries(q):
 assert PINS['require_actual_merged_manifest_queries_verified'] is True
 assert q['verified'] is True and q['actual_merged_apk_manifest'] is True and q['external_http_https_separate'] is True
 raw=q['query_dump'];assert isinstance(raw,str) and 0<len(raw.encode())<256*1024
 assert digest(raw.encode())==q['query_dump_sha256']==PINS['manifest_query_dump_sha256']
 assert re.fullmatch('[0-9a-f]{64}',q['full_manifest_dump_sha256'])
 assert q['multiple_scheme_installer_error']=='INSTALL_PARSE_FAILED_MANIFEST_MALFORMED (-108)'
 assert q['aosp_rule_source']=='https://github.com/aosp-mirror/platform_frameworks_base/blob/android-16.0.0_r1/core/java/com/android/internal/pm/pkg/parsing/ParsingPackageUtils.java#L1894'
 roots=[];stack=[]
 for line in raw.splitlines():
  element=re.match(r'^(\s*)E:\s+([^\s(]+)',line)
  if element:
   indent=len(element.group(1));node={'tag':element.group(2),'attrs':{},'children':[],'indent':indent}
   while stack and stack[-1]['indent']>=indent:stack.pop()
   (stack[-1]['children'] if stack else roots).append(node);stack.append(node)
  elif stack:
   attribute=re.match(r'^\s*A:\s+android:([^\s(=]+)(?:\([^)]*\))?="([^"]*)"',line)
   if attribute:
    key,value=attribute.groups();assert key not in stack[-1]['attrs'];stack[-1]['attrs'][key]=value
 assert len(roots)==1 and roots[0]['tag']=='queries'
 records=[];packages=[]
 for child in roots[0]['children']:
  if child['tag']=='package':assert child['attrs'].get('name');packages.append(child['attrs']['name']);continue
  if child['tag']!='intent':continue
  actions={x['attrs']['name'] for x in child['children'] if x['tag']=='action' and x['attrs'].get('name')}
  data=[x['attrs'] for x in child['children'] if x['tag']=='data']
  schemes={x['scheme'] for x in data if 'scheme' in x};types={x['mimeType'] for x in data if 'mimeType' in x};hosts=[x['host'] for x in data if 'host' in x]
  assert actions or schemes or types,'Merged query has neither action nor data'
  assert len(actions)<=1 and len(schemes)<=1 and len(types)<=1 and len(hosts)<=1,'Merged query exceeds Android parser cardinality'
  categories=sorted({x['attrs']['name'] for x in child['children'] if x['tag']=='category' and x['attrs'].get('name')})
  records.append({'categories':categories,'actions':sorted(actions),'schemes':sorted(schemes),'mime_types':sorted(types),'hosts':sorted(hosts)})
 assert len(records)==q['intent_count']==6 and records==q['intents']
 assert sorted(set(packages))==q['packages'] and {'com.mxtech.videoplayer.ad','com.mxtech.videoplayer.pro'}<=set(packages)
 assert {tuple(x['schemes']) for x in records if x['actions']==['android.intent.action.VIEW'] and x['mime_types']==['video/*']}=={('http',),('https',)}
 return q

def main():
 assert PINS['frozen_pins_ready'] is True and PINS['root_publication_approved'] is True,'Publication remains closed pending actual build byte pins and root review'
 assert os.environ.get('GITHUB_TOKEN') and os.environ['GITHUB_REPOSITORY']==REPO
 for name in ['review_commit','review_tree','source_commit','source_tree']:assert re.fullmatch('[0-9a-f]{40}',PINS[name]),name
 for name in ['plan_sha256','apk_sha256','validation_sha256','build_receipt_sha256','apk_verification_sha256','source_proof_sha256','composition_sha256']:assert re.fullmatch('[0-9a-f]{64}',PINS[name]),name
 for spec in [ANDROID,PROOF]:assert spec['id']>0 and spec['bytes']>0 and re.fullmatch('[0-9a-f]{64}',spec['sha256'])
 composition=(HERE/'composition-manifest.json').read_bytes();assert digest(composition)==PINS['composition_sha256'];c=json.loads(composition)
 assert c['format']==3 and c['final_source_tree']==SOURCE_TREE and c['remote_source_only_commit']==SOURCE
 assert list(c['sources'])==['test'] and c['sources']['test']['commit']==SOURCE and c['sources']['test']['tree']==SOURCE_TREE
 assert c['inputs']['poster_grid_public_product']==PINS['poster_grid_public_product']
 assert c['inputs']['search_see_all_public_product']==PINS['search_see_all_public_product']=='9bda0803597eb4f71daffd38b3c670025d894a7c'
 assert c['logs30_port']['source_integration_receipt_sha256']==PINS['source_integration_receipt_sha256']
 assert 'installer query correction' in c['source_purpose'] and c['logs30_port']['manifest_query_fix'] and not c['logs30_port']['fonts_included']
 assert c['logs30_port']['parent']=='09d7e68845c1e1d7d5466a7bca8d1b2a4f20c134' and c['logs30_port']['product_commit']=='4d97f89aba208c3596f05d289954f7dad9d7a167'
 assert c['logs30_port']['public_tests_unchanged'] is False and c['logs30_port']['all_previous_test_entries_preserved'] and c['logs30_port']['previous_test_entries']==1574 and c['logs30_port']['private_diagnostics_unchanged']
 assert c['logs30_port']['product_changed_paths']==['android/app/build.gradle.kts','android/app/src/main/AndroidManifest.xml']
 assert c['logs30_port']['test_changed_paths']==['tests/android_manifest_queries.py','tests/android_manifest_queries.sh']
 assert c['accepted_logs29_foundation']['known_installer_manifest_defect'] and not c['accepted_logs29_foundation']['user_tested'] and c['accepted_logs29_foundation']['prior_results_reference_only'] and c['accepted_logs29_foundation']['prior_results_count_as_fresh'] is False
 assert isinstance(SOURCE_UNITS,int) and SOURCE_UNITS>0
 assert c['private_overlay']['preserved'] and c['private_overlay']['never_pr'] and not c['remaining_conflicts']
 verify_private_overlay(c)
 work=Path('build/logs30-publication');work.mkdir(parents=True,exist_ok=False)
 run=api('actions/runs/'+str(RUN));assert run['head_sha']==REVIEW and run['run_attempt']==ATTEMPT and run['status']=='completed' and run['conclusion']=='success'
 expected=PINS['job'];job=api('actions/jobs/'+str(expected['id']));assert job['run_id']==RUN and job['run_attempt']==ATTEMPT and job['name']==expected['name'] and job['status']=='completed' and job['conclusion']=='success'
 assert api('git/commits/'+REVIEW)['tree']['sha']==REVIEW_TREE
 source_commit=api('git/commits/'+SOURCE);assert source_commit['tree']['sha']==SOURCE_TREE and [x['sha'] for x in source_commit['parents']]==c['sources']['test']['parents']
 review_commit=api('git/commits/'+REVIEW);assert SOURCE in [x['sha'] for x in review_commit['parents']],'Reviewed build must retain the exact source-only parent'
 android=work/'apk.zip';proofzip=work/'proof.zip';download(ANDROID,android);download(PROOF,proofzip)
 with zipfile.ZipFile(android) as z:apk=member(z,'Nuvio-2.0.40-test-logs30-tv-mobile-test.apk')
 with zipfile.ZipFile(proofzip) as z:
  build=member(z,'android-test/build-receipt.json');proof=member(z,'android-test/android-verified.json');source=member(z,'test-source.json')
  before=member(z,'android-test/workspace/before.json');after=member(z,'android-test/workspace/after.json')
  unit=member(z,'android-test/unit-tests.json')
  facts={arm['abi']:[(item,member(z,'android-test/'+item['facts_file'])) for item in arm['builds']] for arm in json.loads(proof)['actual_arm_compiles']}
 assert len(apk)==APK_BYTES and digest(apk)==APK_SHA
 assert digest(build)==BUILD_SHA and digest(proof)==APK_PROOF_SHA and digest(source)==SOURCE_PROOF_SHA
 b=json.loads(build);p=json.loads(proof);s=json.loads(source)
 assert b['verified'] and b['layer']=='test' and b['source_commit']==SOURCE and b['tree']==SOURCE_TREE and b['review_commit']==REVIEW and b['apk_sha256']==APK_SHA and b['plan_sha256']==PLAN and b['source_restored_clean'] and b['fresh_build_of_both_arms'] and not b['prior_apk_proofs_reused']
 assert b['source_proof_sha256']==SOURCE_PROOF_SHA and b['verification_sha256']==APK_PROOF_SHA
 assert digest(unit)==b['unit_tests_sha256']==PINS['unit_tests_sha256'] and b['android_junit_required_passed'] and b['host_c_groups_passed']==4
 u=json.loads(unit);assert u['passed'] and u['source_commit']==SOURCE and u['tree']==SOURCE_TREE and u['review_commit']==REVIEW
 assert u['task']==':app:testDebugUnitTest' and u['unfiltered'] and u['task_executed'] and u['returncode']==0 and u['required_tests']==6
 assert u['required_class']=='space.nuvio.nativelegacy.PlayerExternoContratoTest'
 assert set(u['required_methods'])=={'ordinaryStreamsUseOneImplicitSystemChooserTarget','requiredHeadersUseOnlyExplicitVerifiedMxComponents','absentOrUnsupportedHeaderPlayersNeverCreateGenericBase','malformedHeadersRefuseRatherThanSilentlyDropCredentials','urlsAndCapacityAreValidatedWithoutRewritingSignedQuery','resumeUsesIntMillisecondsWithoutOutputOnlyVlcExtras'}
 import xml.etree.ElementTree as ET
 found=[]
 for r in u['reports']:
  assert digest(r['xml'].encode())==r['sha256'];x=ET.fromstring(r['xml']);assert x.tag=='testsuite'
  assert all(int(x.get(k,'0'))==0 for k in ('failures','errors','skipped'))
  assert int(x.get('tests','-1'))==r['tests']==len(x.findall('testcase'))>0
  assert not x.findall('.//failure') and not x.findall('.//error') and not x.findall('.//skipped')
  if x.get('name')==u['required_class']:found.extend(z.get('name') for z in x.findall('testcase'))
 assert sorted(found)==u['required_methods'] and len(found)==6 and sum(x['tests'] for x in u['reports'])==u['tests']
 assert re.fullmatch('[0-9a-f]{64}',u['compiled_test_class_sha256']) and re.fullmatch('[0-9a-f]{64}',u['test_source_sha256'])
 assert len(u['host_c_groups'])==u['host_c_groups_required']==4
 assert {(x['script'],x['sanitize']) for x in u['host_c_groups']}=={(script,mode) for script in ('tests/playerexterno.sh','tests/playerexterno_integracao.sh') for mode in (False,True)}
 for group in u['host_c_groups']:
  assert group['returncode']==0 and digest(group['output'].encode())==group['output_sha256']
  assert len(group['markers'])==(8 if group['script']=='tests/playerexterno.sh' else 6)
  assert all(group['output'].splitlines().count(marker)==1 for marker in group['markers'])
 manifest_test=u['manifest_query_source_tests']
 assert manifest_test['passed'] is True and manifest_test['returncode']==0 and manifest_test['parser_cases']==PINS['source_manifest_parser_cases']==14 and manifest_test['source_query_intents']==6
 assert manifest_test['script']=='tests/android_manifest_queries.sh' and manifest_test['script_sha256']==PINS['manifest_query_script_sha256'] and manifest_test['python_sha256']==PINS['manifest_query_python_sha256']
 assert digest(manifest_test['output'].encode())==manifest_test['output_sha256'] and manifest_test['output'].splitlines()[-1]=='android_manifest_queries: 14 parser cases passed; 6 query intents valid'
 manifest_queries=verify_manifest_queries(p['manifest_queries'])
 assert p['external_player_activity_jni_instance_methods_verified'] and len(p['external_player_activity_jni_methods'])==6

 assert digest(before)==b['workspace_before_sha256'] and digest(after)==b['workspace_after_sha256']
 for phase,raw in [('before',before),('after',after)]:
  w=json.loads(raw);assert w['outcome']=='passed' and w['phase']==phase and w['source_commit']==SOURCE and w['source_tree']==SOURCE_TREE
 assert json.loads(after)['before_sha256']==digest(before)
 for abi,items in facts.items():
  assert items
  for item,raw in items:
   f=json.loads(raw);assert digest(raw)==item['facts_sha256'] and f['abi']==abi and f['source_abi_build_bound'] and f['sysroot_bound'] and f['private_native_opt_in'] and f['compiler']['ndk_version']=='27.2.12479018'
   assert item['source_abi_compiler_bound'] and item['private_native_opt_in'] and item['report_endpoint_configured'] and len(f['source_relative_units'])==item['main_sources']==SOURCE_UNITS and len(set(f['source_relative_units']))==SOURCE_UNITS
 assert p['outcome']=='passed' and p['source_commit']==SOURCE and p['tree']==SOURCE_TREE and p['review_commit']==REVIEW and p['plan_sha256']==PLAN and p['apk_sha256']==APK_SHA and p['apk_bytes']==APK_BYTES and p['package']==PACKAGE and p['version_code']==20040 and p['version_name']=='2.0.40-test-logs30' and p['certificate_sha256']==CERT and p['native_16kb_pages']=='passed' and {x['abi'] for x in p['actual_arm_compiles']}=={'arm64-v8a','armeabi-v7a'} and p['private_log_opt_in'] and p['legacy_jni_overloads_preserved'] and p['file_provider']=={'authority':'space.nuvio.nativelegacy.touch.atualizacao','nonexported':True,'updater_path_only':True}
 assert s['verified'] and s['source_clean'] and not s['review_only'] and s['tests_tree_preserved'] is False and s['existing_test_entries_preserved'] and s['reviewed_tests_delta_verified'] and s['materialization_mode']=='checkout' and s['source_frozen'] and s['tests_tree']==c['tests_tree'] and s['source_commit']==SOURCE and s['expected_remote_source_commit']==SOURCE and s['tree']==SOURCE_TREE and s['review_commit']==REVIEW and s['source_binding']=='exact-git-tree'
 assert s['manifest_sha256']==c['sources']['test']['manifest_sha256']
 assert len(p['all_player_jni_lookups'])>=19 and len(p['all_player_jni_lookups'])==len(set(tuple(x) for x in p['all_player_jni_lookups']))
 assert p['compiled_android_classes'] and p['apk_dex_sha256']
 marker_abis=[]
 with zipfile.ZipFile(io.BytesIO(apk)) as z:
  for name,h in p['apk_dex_sha256'].items():assert digest(z.read(name))==h
  for abi in ['armeabi-v7a','arm64-v8a']:
   assert b'[detail-ui-test] phase=' in z.read('lib/'+abi+'/libmain.so'),'Private diagnostic marker absent in '+abi
   marker_abis.append(abi)
 validation=(HERE/VALIDATION_NAME).read_bytes();assert digest(validation)==VALIDATION_SHA;v=json.loads(validation)
 assert v['all_passed'] and v['source_commit']==SOURCE and v['source_tree']==SOURCE_TREE and v['review_commit']==REVIEW and v['run_id']==RUN and v['run_attempt']==ATTEMPT and v['plan_sha256']==PLAN and v['job']==expected
 assert v['apk_sha256']==APK_SHA and v['apk_bytes']==APK_BYTES and v['build_receipt_sha256']==BUILD_SHA and v['apk_verification_sha256']==APK_PROOF_SHA and v['source_proof_sha256']==SOURCE_PROOF_SHA
 assert v['previous_results_count_as_fresh'] is False and v['single_exact_private_source'] is True and v['private_overlay_preserved'] is True
 assert v['composition_sha256']==PINS['composition_sha256']
 assert v['android_build_and_apk_checks_passed'] and v['private_test_apk'] is True and v['includes_search_window_fix'] is True and v['includes_player_control_relocation'] is True and v['includes_special_seasons'] is True and v['includes_poster_grid_update'] is True and v['includes_search_see_all'] is True and not v['public_product_fix_claimed'] and not v['macos_validation_included'] and not v['emulator_results_included']
 assert v['includes_external_player'] is True and v['includes_font_import'] is False and v['android_unit_tests_passed'] and v['host_c_state_integration_passed']
 assert v['unit_tests_sha256']==digest(unit) and v['actual_android_junit_tests']==u['tests']==6 and v['actual_host_c_groups']==4 and v['actual_external_activity_jni_methods']==6
 assert v['includes_installer_manifest_query_fix'] is True and v['actual_merged_apk_manifest_queries_verified'] is True and v['actual_manifest_query_intents']==manifest_queries['intent_count']==6
 assert v['manifest_query_dump_sha256']==manifest_queries['query_dump_sha256'] and v['source_manifest_parser_cases']==14 and v['source_manifest_query_tests_passed'] is True
 assert v['actual_package_manager_installation_claimed'] is False and v['android16_parser_rule_verification_only'] is True and v['publication_requires_actual_merged_manifest_queries_verified'] is True
 assert v['logs29_foundation_user_test_claimed'] is False and v['logs29_foundation_known_installer_defect'] is True
 assert v['native_diagnostic_marker_required_for_publication'] and marker_abis==['armeabi-v7a','arm64-v8a']
 assert re.fullmatch('[0-9a-f]{64}',v['diagnostics']['patch_sha256'])
 assert c['private_overlay']['new_diagnostics']['patch_sha256']==v['diagnostics']['patch_sha256'] and c['private_overlay']['new_diagnostics']['build_gate']=='NV_TEST_LOCAL_LOGS'
 assets={APK_NAME:('application/vnd.android.package-archive',apk),VALIDATION_NAME:('application/json',validation)}
 tag_ok();release=existing_release()
 if release is None:release=target_ok(api('releases','POST',{'tag_name':TAG,'target_commitish':SOURCE,'name':'Nuvio TV/Mobile Test — Logs30','body':NOTES,'draft':True,'prerelease':True}))
 existing=api('releases/'+str(release['id'])+'/assets?per_page=100');assert len(existing)<=2 and {x['name'] for x in existing}<=set(assets),'Unexpected Logs30 release assets'
 if not release['draft']:assert {x['name'] for x in existing}==set(assets),'Published Logs30 release is incomplete'
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
 receipt={'published':True,'release_id':final['id'],'release_url':final['html_url'],'tag':TAG,'source_commit':SOURCE,'review_commit':REVIEW,'source_run':RUN,'source_run_attempt':ATTEMPT,'fresh_android_job_verified':True,'private_test_apk':True,'includes_search_window_fix':True,'includes_player_control_relocation':True,'includes_special_seasons':True,'includes_poster_grid_update':True,'includes_search_see_all':True,'includes_external_player':True,'includes_font_import':False,'actual_android_junit_tests':6,'actual_host_c_groups':4,'actual_activity_instance_jni_methods':6,'public_product_fix_claimed':False,'native_diagnostic_marker_abis':marker_abis,'macos_fresh_result_claimed':False,'validation_sha256':VALIDATION_SHA,'apk_sha256':APK_SHA,'apk_bytes':APK_BYTES,'release_assets':sorted(assets),'publisher_review':os.environ['GITHUB_SHA'],'no_rebuild_or_test_rerun':True,'no_pr_writes':True,'other_release_assets_untouched':True}
 (work/'publication-receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
