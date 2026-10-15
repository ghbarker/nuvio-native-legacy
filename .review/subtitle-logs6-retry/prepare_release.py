#!/usr/bin/env python3
"""Bind new ARM APK proofs to unchanged-source prior validation transparently."""
import hashlib,json,os,shutil,sys
from pathlib import Path

OLD='03c7ac63bb73f2448099f20e11da62728193ec72'
SOURCE='04612e42fc58776d947062e908d32ce7322e58cf'
proof=json.loads(Path('delivery/proof/update-identity.json').read_text())
assert proof['verified'] and proof['phase']=='final' and proof['test_package']
assert proof['source_commit']==SOURCE==os.environ['PLANNED_SOURCE']
assert proof['build_commit']==os.environ['GITHUB_SHA'] and proof['case']=='combined-delivery'
origin=json.loads(Path('delivery/reused/validation-origin.json').read_text())
assert origin['outcome']=='passed' and origin['source_unchanged'] and origin['original_validation_proofs_unmodified']
assert origin['validation_origin']=={'run_id':38665838503,'review_commit':OLD}
assert origin['retry_review_commit']==os.environ['GITHUB_SHA']
assert {x['case'] for x in origin['checks']}=={'standalone-subtitles','combined-delivery','media3'}
for item in origin['checks']:
 assert item['job_conclusion']=='success' and item['validation_review_commit']==OLD
 name='media3-verified.json' if item['case']=='media3' else 'native-gates-verified.json'
 path=Path('delivery/reused')/item['case']/name
 assert hashlib.sha256(path.read_bytes()).hexdigest()==item['proof_sha256']
 previous=json.loads(path.read_text())
 assert previous['source_commit']==item['source_commit'] and previous['review_commit']==OLD
native=json.loads(Path('delivery/reused/combined-delivery/native-gates-verified.json').read_text())
media3=json.loads(Path('delivery/reused/media3/media3-verified.json').read_text())
standalone=json.loads(Path('delivery/reused/standalone-subtitles/native-gates-verified.json').read_text())
for gate in (native,media3):
 assert gate['outcome']=='passed' and gate['source_commit']==SOURCE and gate['review_commit']==OLD
 assert gate['case']=='combined-delivery' and gate['source_scope']==proof['scope']=='complete combined product candidate'
assert standalone['source_commit']=='039b50d4f6d9eaea05504d906e77b2b3b6538a56'
assert len(standalone['subtitle_visual_captures'])==2
sys.path.insert(0,'.review/subtitle-logs6/build')
from media3_contracts import load_contracts
manifest,required,startup,classes,manifest_sha=load_contracts(Path('.review/subtitle-logs6/android/fixture-manifest.json'))
assert set(media3['required_tests'])==required and required<=set(media3['tests_seen'])
assert media3['required_test_classes']==classes and media3['fixture_manifest_sha256']==manifest_sha
assert len(native['subtitle_fixture_records'])==6 and all(x['passed'] for x in native['subtitle_fixture_records'])
assert native['original_log_behavior']['original_reporting_preserved'] and native['original_log_behavior']['original_live_viewer_preserved']
for enabled in ('false','true'):
 config=json.loads(Path('delivery/proof/local-logs-'+enabled+'-verified.json').read_text())
 assert config['outcome']=='passed' and config['source_commit']==SOURCE and config['review_commit']==proof['build_commit']
 assert config['kotlin_opt_in']==(enabled=='true') and config['native_macro_enabled']==(enabled=='true')
 assert {a['abi'] for a in config['arm_builds']}=={'arm64-v8a','armeabi-v7a'}
 assert all(b['report_endpoint_configured'] for a in config['arm_builds'] for b in a['builds'])
assert proof['package']=='space.nuvio.nativelegacy.touch' and proof['label']=='Nuvio TV/Mobile Test'
assert proof['version_code']==20016 and proof['version_name']=='2.0.16-test-logs6'
assert proof['certificate_sha256']=='862d7ef63a0dbde2929f6f231cae295510726d9ac28f9292496d2a38304be5e6'
assert proof['native_page_packaging']=='passed' and proof['abis']==['arm64-v8a','armeabi-v7a']
assert proof['log_file_provider']['original_updater_path_only']
files=list(Path('delivery/apk').glob('*-tv-mobile-test.apk'));assert len(files)==1
data=files[0].read_bytes();digest=hashlib.sha256(data).hexdigest()
assert digest==proof['apk_sha256'] and len(data)==proof['apk_bytes']
target=Path('delivery/Nuvio-TV-Mobile-Test-Logs6.apk');shutil.copy2(files[0],target)
target.with_suffix('.apk.sha256').write_text(digest+'  '+target.name+'\n')
for src,dst in [('combined-delivery/native-gates-verified.json','native-gates-verified.json'),('standalone-subtitles/native-gates-verified.json','standalone-native-gates-verified.json'),('media3/media3-verified.json','media3-verified.json'),('validation-origin.json','validation-origin.json')]:
 shutil.copy2(Path('delivery/reused')/src,Path('delivery/proof')/dst)
Path('delivery/release-notes.txt').write_text(
 'I kept the original Send logs, live View logs and saved automatic-upload setting, and added Download logs under Settings > About and help. '
 'Download opens the Android file picker and saves a redacted text file. The original public report service is restored; existing build configuration overrides are preserved.\n\n'
 'Subtitle language stays on the first line, with the embedded track name underneath when supplied. Playback, automatic Next, profile isolation and provider failover remain as in Logs5. '
 'The extra download action and test diagnostics remain excluded from clean feature branches and PRs.\n\n'
 'Both ARM builds, original-report configuration, persistent signature and 16 KB packaging passed in this build. '
 'The unchanged product commits reuse successful macOS TV/Mobile full builds, normal/ASan/UBSan subtitle and original-log checks, six UI captures and all 20 Android playback/download contracts from run 38665838503. '
 'validation-origin.json records the original validation review and this APK build review separately. No validation was relabeled as a new test run.\n\n'
 'Package space.nuvio.nativelegacy.touch; label Nuvio TV/Mobile Test; versionCode 20016. Choose Update to keep accounts and settings.\n\n'
 'Product source: '+SOURCE+'\nAPK build review: '+proof['build_commit']+'\nOriginal validation review: '+OLD+'\n')
print('Verified new ARM APK and unchanged-source successful validation:',digest)
