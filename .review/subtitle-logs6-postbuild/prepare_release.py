#!/usr/bin/env python3
"""Publish only real fresh runtime passes, exact APKs and honest reused Mac proof."""
import hashlib,json,os,shutil,sys
from pathlib import Path

OLD='03c7ac63bb73f2448099f20e11da62728193ec72';BUILD='b1f00ebd3b7ca660a6160fe8fa2d53e8e6e80e12'
SOURCE='04612e42fc58776d947062e908d32ce7322e58cf';REVIEW=os.environ['GITHUB_SHA']
root=Path('delivery');proof=json.loads((root/'proof/update-identity.json').read_text())
assert proof['verified'] and proof['phase']=='final' and proof['test_package']
assert proof['source_commit']==SOURCE==os.environ['PLANNED_SOURCE'] and proof['build_commit']==BUILD
assert proof['case']=='combined-delivery' and proof['package']=='space.nuvio.nativelegacy.touch'
assert proof['label']=='Nuvio TV/Mobile Test' and proof['version_code']==20016 and proof['version_name']=='2.0.16-test-logs6'
assert proof['certificate_sha256']=='862d7ef63a0dbde2929f6f231cae295510726d9ac28f9292496d2a38304be5e6'
assert proof['native_page_packaging']=='passed' and proof['abis']==['arm64-v8a','armeabi-v7a']
assert proof['log_file_provider']['original_updater_path_only']
origin=json.loads((root/'reused-native/validation-origin.json').read_text())
assert origin['outcome']=='passed' and origin['source_unchanged'] and origin['original_validation_proofs_unmodified']
assert origin['validation_origin']=={'run_id':38665838503,'review_commit':OLD}
assert origin['postbuild_validation_review_commit']==REVIEW
assert {x['case'] for x in origin['checks']}=={'standalone-subtitles','combined-delivery'}
for item in origin['checks']:
 path=root/'reused-native'/item['case']/'native-gates-verified.json'
 assert hashlib.sha256(path.read_bytes()).hexdigest()==item['proof_sha256']
 previous=json.loads(path.read_text())
 assert item['job_conclusion']=='success' and previous['source_commit']==item['source_commit'] and previous['review_commit']==OLD
native=json.loads((root/'reused-native/combined-delivery/native-gates-verified.json').read_text())
standalone=json.loads((root/'reused-native/standalone-subtitles/native-gates-verified.json').read_text())
assert native['outcome']=='passed' and native['source_commit']==SOURCE and native['review_commit']==OLD
assert standalone['source_commit']=='039b50d4f6d9eaea05504d906e77b2b3b6538a56' and len(standalone['subtitle_visual_captures'])==2
android=json.loads((root/'android-postbuild-verified.json').read_text())
assert android['outcome']=='passed' and android['validation_review_commit']==REVIEW and android['apk_build_review_commit']==BUILD
assert android['arm_compiler_rerun'] is False and len(android['cases'])==2
assert {c['case']:c['actual_c_lookup_count'] for c in android['cases']}=={'standalone-subtitles':13,'combined-delivery':19}
assert all(c['outcome']=='passed' and c['actual_apk_reverified'] for c in android['cases'])
for enabled in ('false','true'):
 config=json.loads((root/'proof'/('local-logs-'+enabled+'-verified.json')).read_text())
 assert config['outcome']=='passed' and config['source_commit']==SOURCE and config['review_commit']==BUILD
 assert config['kotlin_opt_in']==(enabled=='true') and config['native_macro_enabled']==(enabled=='true')
 assert {a['abi'] for a in config['arm_builds']}=={'arm64-v8a','armeabi-v7a'}
 assert all(b['report_endpoint_configured'] for a in config['arm_builds'] for b in a['builds'])
mediafiles=list((root/'media3-artifact').rglob('media3-verified.json'));assert len(mediafiles)==1
media=json.loads(mediafiles[0].read_text())
assert media['outcome']=='passed' and media['source_commit']==SOURCE and media['review_commit']==REVIEW and media['case']=='combined-delivery'
sys.path.insert(0,'.review/subtitle-logs6/build')
from media3_contracts import load_contracts
manifest,required,startup,classes,manifest_sha=load_contracts(Path('.review/subtitle-logs6/android/fixture-manifest.json'))
assert set(media['required_tests'])==required and required<=set(media['tests_seen'])
assert media['required_test_classes']==classes and media['fixture_manifest_sha256']==manifest_sha
assert len(required)==20
files=list((root/'apk').glob('*-tv-mobile-test.apk'));assert len(files)==1
data=files[0].read_bytes();digest=hashlib.sha256(data).hexdigest()
assert digest==proof['apk_sha256'] and len(data)==proof['apk_bytes']
target=root/'Nuvio-TV-Mobile-Test-Logs6.apk';shutil.copy2(files[0],target)
target.with_suffix('.apk.sha256').write_text(digest+'  '+target.name+'\n')
shutil.copy2(mediafiles[0],root/'proof/media3-verified.json')
shutil.copy2(root/'reused-native/combined-delivery/native-gates-verified.json',root/'proof/native-gates-verified.json')
shutil.copy2(root/'reused-native/standalone-subtitles/native-gates-verified.json',root/'proof/standalone-native-gates-verified.json')
shutil.copy2(root/'android-postbuild-verified.json',root/'proof/android-postbuild-verified.json')
receipt={'outcome':'passed','product_source_commit':SOURCE,'apk_build':{'run_id':38666973073,'review_commit':BUILD,'apk_sha256':digest},
 'postbuild_and_fresh_runtime':{'run_id':int(os.environ['GITHUB_RUN_ID']),'review_commit':REVIEW,'runtime_contracts_passed':20,'actual_dex_c_lookups':{'standalone':13,'combined':19}},
 'native_validation_origin':origin,'original_failed_checks_not_counted_as_passes':True,
 'replaced_checks':['Incorrect clean-source preload-only JNI expectation replaced by actual-source APK DEX verification','Incomplete download IntentFilter monitor corrected in test fixture; all20 runtime contracts rerun']}
(root/'proof/validation-origin.json').write_text(json.dumps(receipt,indent=2)+'\n')
(root/'release-notes.txt').write_text(
 'I kept the original Send logs, live View logs and saved automatic-upload setting, and added Download logs under Settings > About and help. '
 'Download opens the Android file picker and saves a redacted text file. The official public report service is configured; existing build overrides are preserved.\n\n'
 'Subtitle language remains on the first line, with the embedded track name underneath when supplied. Playback, automatic Next, profile isolation and provider failover remain as in Logs5. '
 'The extra download action and test diagnostics stay excluded from clean feature branches and PRs.\n\n'
 'Both ARM builds, signature and 16 KB packaging passed. A fresh check verified every actual APK JNI lookup (13 in clean subtitle source,19 in the combined test app). '
 'All20 real Android playback/download contracts passed after correcting the test monitor. The same source commits retain successful macOS TV/Mobile full builds, subtitle and original-log normal/ASan/UBSan checks, and six reviewed captures. '
 'validation-origin.json records build, fresh runtime/postbuild validation and reused macOS reviews separately; failed original checks are not counted as passes.\n\n'
 'Package space.nuvio.nativelegacy.touch; label Nuvio TV/Mobile Test; versionCode 20016. Choose Update to keep accounts and settings.\n\n'
 'Product source: '+SOURCE+'\nAPK build review: '+BUILD+'\nFresh validation review: '+REVIEW+'\n')
print('Verified exact built APK, actual DEX bridge, fresh20runtime contracts and successful source-bound macOS checks:',digest)
