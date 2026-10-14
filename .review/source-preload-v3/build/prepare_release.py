#!/usr/bin/env python3
import hashlib
import json
import os
from pathlib import Path
import shutil
from media3_contracts import load_contracts, ORIGINAL_TESTS

proof = json.loads(Path('delivery/proof/update-identity.json').read_text())
assert proof['verified'] and proof['phase'] == 'final' and proof['test_package']
media3 = json.loads(Path('delivery/media3/media3-verified.json').read_text())
native = json.loads(Path('delivery/native/native/native-gates-verified.json').read_text())
for gate in (media3, native):
    assert gate['outcome'] == 'passed'
    assert gate['source_commit'] == os.environ['PLANNED_SOURCE']
    assert gate['case'] == proof['case'] == 'combined-delivery'
    assert gate['review_commit'] == proof['build_commit']
manifest, required, startup, classes, manifest_sha = load_contracts(Path(__file__).parent.parent / 'android/fixture-manifest.json')
assert set(media3['required_tests']) == required and required <= set(media3['tests_seen'])
assert set(media3['original_v1_contracts']) == ORIGINAL_TESTS and set(media3['startup_tests']) == startup
assert media3['required_test_classes'] == classes and media3['fixture_manifest_sha256'] == manifest_sha
for klass, names in classes.items():
    assert set(names) <= set(media3['test_classes_seen'].get(klass, [])), klass
assert media3['source_scope'] == native['source_scope'] == proof['scope'] == 'complete combined product candidate'
assert len(native['actual_original_full_source_asan']) == 3
assert {'release-date', 'release-date-asan'} <= set(native['gates'])
Path('delivery/proof/media3-verified.json').write_text(json.dumps(media3, indent=2) + '\n')
Path('delivery/proof/native-gates-verified.json').write_text(json.dumps(native, indent=2) + '\n')
assert proof['source_commit'] == os.environ['PLANNED_SOURCE']
assert proof['package'] == 'space.nuvio.nativelegacy.touch'
assert proof['label'] == 'Nuvio TV/Mobile Test' and proof['version_code'] == 20012
assert proof['version_name'] == '2.0.12-test-logs2'
assert proof['certificate_sha256'] == '862d7ef63a0dbde2929f6f231cae295510726d9ac28f9292496d2a38304be5e6'
assert proof['native_page_packaging'] == 'passed' and proof['abis'] == ['arm64-v8a', 'armeabi-v7a']
files = list(Path('delivery/apk').glob('*-tv-mobile-test.apk'))
assert len(files) == 1
data = files[0].read_bytes()
digest = hashlib.sha256(data).hexdigest()
assert digest == proof['apk_sha256'] and len(data) == proof['apk_bytes']
target = Path('delivery/Nuvio-TV-Mobile-Test-Logs2.apk')
shutil.copyfile(files[0], target)
target.with_suffix('.apk.sha256').write_text(digest + '  ' + target.name + '\n')
Path('delivery/release-notes.txt').write_text(
    'I fixed episode release-date handling so localized metadata, including English dates, no longer blocks next-episode preloading when Preload video is on. '
    'The app keeps the original machine date for playback decisions and formats a separate date for display.\n\n'
    'This update contains that date fix and retains the local diagnostic logging. The separately investigated credits changes are excluded. '
    'Open Settings > About and help > Save log to phone, choose Downloads and save, then attach the text file in our chat. '
    'The file contains redacted current/previous session logs and the current app logcat snapshot. '
    'Test-only state lines show the credits marker, countdown, seek and post-credits gates.\n\n'
    'Automatic and manual upstream log uploads are disabled in this test build. '
    'This diagnostic overlay is excluded from the clean feature branch and will never be included in its PR.\n\n'
    'Pinned macOS full builds, native regressions, actual Android ARM32/ARM64 builds, 16 KB packaging, '
    '15 original playback contracts and 4 actual Android local-log contracts passed before publication. '
    'The new episode-date regressions passed in normal and ASan/UBSan runs using the actual metadata and Next code. '
    'The Supernatural transition still needs a device retest.\n\n'
    'Package space.nuvio.nativelegacy.touch; label Nuvio TV/Mobile Test; versionCode 20012. '
    'This APK uses the persistent signing certificate to update the stable test app and keep its accounts.\n\n'
    'Product source: ' + proof['source_commit'] + '\n'
    'CI review source: ' + proof['build_commit'] + '\n')
print('Pinned final diagnostic APK verified:', digest)
