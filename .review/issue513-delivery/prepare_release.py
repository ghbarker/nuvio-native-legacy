#!/usr/bin/env python3
"""Gate publication on new source checks and transparent unchanged-component reuse."""
import hashlib,json,os,shutil,zipfile
from pathlib import Path
here=Path(__file__).resolve().parent;plan=json.loads((here/'plans.json').read_text());assert plan['frozen_sources'] and plan['publish_release']
review=os.environ['GITHUB_SHA'];delivery=Path('delivery');prepared=json.loads((delivery/'materialization/prepared-cases.json').read_text())
proof=json.loads((delivery/'android/update-identity.json').read_text());standard=json.loads((delivery/'android/standard-identity.json').read_text());source=prepared['delivery']['source_commit']
assert source==os.environ['PLANNED_SOURCE'] and proof['source_commit']==source and proof['build_commit']==review
for k,v in plan['identity'].items():assert proof[k]==v,(k,proof.get(k))
assert proof['verified'] and proof['test_package'] and proof['native_page_packaging']=='passed' and proof['abis']==['arm64-v8a','armeabi-v7a']
assert proof['product_delta_only'] and proof['test_only_overlay_unchanged_from_parent']
assert proof['feature_changed_paths']==['src/detail.c','src/streams.c']
assert proof['log_file_provider']['original_updater_path_only'] and proof['log_file_provider']['nonexported']
assert standard['verified'] and standard['source_commit']==source and standard['build_commit']==review and not standard['test_package']
for mode in ('false','true'):
 p=json.loads((delivery/'android'/('local-logs-'+mode+'-verified.json')).read_text())
 assert p['outcome']=='passed' and p['source_commit']==source and p['review_commit']==review,p
jni=json.loads((delivery/'android/product-jni-verified.json').read_text());assert jni['outcome']=='passed' and jni['source_commit']==source and jni['review_commit']==review
native=[]
for case in prepared['matrix']['include']:
 root=delivery/('native-'+case['id']);p=json.loads((root/'native/native-gates-verified.json').read_text());src=json.loads((root/'source-verified.json').read_text())
 assert p['outcome']=='passed' and p['source_commit']==case['source_commit'] and p['review_commit']==review
 assert p['source_unchanged'] and p['assertions_passed']==272 and p['fixture_executions']==8
 assert set(p['full_source_compiles'])=={'tv','touch'} and all(v['bytes']>0 for v in p['full_source_compiles'].values())
 assert src['verified'] and src['source_commit']==p['source_commit'] and src['base']==case['base'] and src['build_commit']==review
 native.append({'case':case['id'],'source_commit':p['source_commit'],'assertions_passed':272,'proof_sha256':hashlib.sha256((root/'native/native-gates-verified.json').read_bytes()).hexdigest()})
reuse=json.loads((delivery/'runtime/player-component-reuse.json').read_text());assert reuse['outcome']=='passed' and reuse['new_product_source_commit']==source and reuse['new_review_commit']==review
assert reuse['required_runtime_contracts']==reuse['xml_tests_passed']==20 and reuse['skipped']==reuse['failures']==0
assert reuse['old_validation_proof_unmodified'] and reuse['new_dispatcher_validation_required_separately']
assert reuse['validation_origin']['review_commit']=='1eac5860820e6e8b142140ceccd054ea02956793' and reuse['validation_origin']['source_commit']==plan['source_origins']['existing_test_app_base']
files=list((delivery/'apk').glob('*-tv-mobile-test.apk'));assert len(files)==1
data=files[0].read_bytes();digest=hashlib.sha256(data).hexdigest();assert proof['apk_sha256']==digest and proof['apk_bytes']==len(data)
target=delivery/plan['release']['apk'];shutil.copyfile(files[0],target);target.with_suffix('.apk.sha256').write_text(digest+'  '+target.name+'\n')
receipt={'outcome':'passed','source_commit':source,'review_commit':review,'issue':plan['issue'],'apk_sha256':digest,'apk_bytes':len(data),
 'native_validation':native,'assertions_passed_total':544,'android_both_arm_cores':'newly compiled and verified',
 'unchanged_player_component_validation':reuse,'product_changes':['src/detail.c','src/streams.c'],'source_origins':plan['source_origins'],
 'scope':'actual C dispatcher/queue/list ownership regressions, full macOS TV/touch builds, real signed ARM32/ARM64 APK; unchanged20player/download runtime contracts reused with original provenance'}
(delivery/'validation.json').write_text(json.dumps(receipt,indent=2)+'\n')
with zipfile.ZipFile(delivery/'validation-proofs.zip','w',zipfile.ZIP_DEFLATED) as z:
 for root in ('android','native-standalone-issue513','native-combined-delivery','runtime','materialization'):
  for path in (delivery/root).rglob('*.json'):z.write(path,str(path.relative_to(delivery)))
 z.write(delivery/'validation.json','validation.json')
(delivery/'release-notes.txt').write_text('I fixed episode cards bypassing manual source selection when a remembered source exists. Selecting an episode now opens Sources when manual selection is enabled. Primary Resume and automatic next episode keep their existing behavior.\n\nI also guarded Sources against an older queued episode response or replaced list, so a click cannot select another episode’s link.\n\nThis test update keeps the accepted subtitle track-name layout, original Send/View logs and separate Download action, playback, profile isolation and failover behavior. Package and persistent signer match Logs6; versionCode20017 allows Update without resetting accounts. Test-only diagnostics remain excluded from feature PRs.\n\nBoth upstream-based and combined candidates passed new actual C TV/mobile normal+ASan/UBSan tests and complete macOS TV/touch compiles. Android ARM32/ARM64, native16KB packaging, compiled JNI and signature were checked on this APK. The20unchanged isolated player/download runtime contracts retain the original passing run provenance; they are not presented as a new whole-app runtime execution.\n\nIssue: '+plan['issue']+'\nProduct source: '+source+'\nReview: '+review+'\n')
print('Verified compatible issue513 test APK:',digest)
