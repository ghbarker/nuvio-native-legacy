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
assert 'image-download' in native['gates']
assert 'seek-regression' in native['gates']
assert len(native['seek_regression_actual_regressions']) == 6
assert {(run['case'], run['mode']) for run in native['seek_regression_actual_regressions']} == {
    ('seek_gestures', 'normal'), ('seek_gestures', 'asan'),
    ('seek_controller', 'normal'), ('seek_controller', 'asan'),
    ('window_reentry', 'normal'), ('window_reentry', 'asan')}
assert all(run['passed'] is True for run in native['seek_regression_actual_regressions'])
assert {'player_recuo-original', 'posplay_recuo-original', 'creditosauto-original', 'creditosauto-original-asan'} <= set(native['gates'])
assert len(native['actual_original_seek_guards']) == 3
Path('delivery/proof/media3-verified.json').write_text(json.dumps(media3, indent=2) + '\n')
Path('delivery/proof/native-gates-verified.json').write_text(json.dumps(native, indent=2) + '\n')
assert proof['source_commit'] == os.environ['PLANNED_SOURCE']
assert proof['package'] == 'space.nuvio.nativelegacy.touch'
assert proof['label'] == 'Nuvio TV/Mobile Test' and proof['version_code'] == 20015
assert proof['version_name'] == '2.0.15-test-logs5'
assert proof['log_file_provider']['authority'] == 'space.nuvio.nativelegacy.touch.atualizacao'
assert proof['log_file_provider']['nonexported'] and proof['log_file_provider']['read_grants_supported'] and proof['log_file_provider']['bounded_test_cache_path']
assert proof['certificate_sha256'] == '862d7ef63a0dbde2929f6f231cae295510726d9ac28f9292496d2a38304be5e6'
assert proof['native_page_packaging'] == 'passed' and proof['abis'] == ['arm64-v8a', 'armeabi-v7a']
files = list(Path('delivery/apk').glob('*-tv-mobile-test.apk'))
assert len(files) == 1
data = files[0].read_bytes()
digest = hashlib.sha256(data).hexdigest()
assert digest == proof['apk_sha256'] and len(data) == proof['apk_bytes']
target = Path('delivery/Nuvio-TV-Mobile-Test-Logs5.apk')
shutil.copyfile(files[0], target)
target.with_suffix('.apk.sha256').write_text(digest + '  ' + target.name + '\n')
Path('delivery/release-notes.txt').write_text(
    'I added View logs, Share logs and Save logs to the TV/Mobile test build. '
    'Open Settings > About and help > Test logs. View opens a scrollable, selectable read-only log; '
    'Share opens the Android chooser with a redacted text file; Save keeps the existing phone file picker.\n\n'
    'Playback, automatic Next, episode-date handling and the artwork fix are unchanged from Logs4. '
    'The file contains redacted current/previous session logs and the current app logcat snapshot. '
    'Automatic and manual upstream log uploads remain disabled. '
    'These diagnostic actions are test-only, excluded from all clean feature branches and PRs.\n\n'
    'Pinned macOS full builds and all retained native regressions passed. '
    'Actual Android ARM32/ARM64 builds, 16 KB packaging, the same persistent signing certificate and compiled FileProvider authority passed verification. '
    'All 19 existing Android contracts and three new log-action contracts passed before publication. '
    'External app chooser behavior still needs the phone test.\n\n'
    'Package space.nuvio.nativelegacy.touch; label Nuvio TV/Mobile Test; versionCode 20015. '
    'Choose Update to keep accounts and settings.\n\n'
    'Product source: ' + proof['source_commit'] + '\n'
    'CI review source: ' + proof['build_commit'] + '\n')
print('Pinned final diagnostic APK verified:', digest)
