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
Path('delivery/proof/media3-verified.json').write_text(json.dumps(media3, indent=2) + '\n')
Path('delivery/proof/native-gates-verified.json').write_text(json.dumps(native, indent=2) + '\n')
assert proof['source_commit'] == os.environ['PLANNED_SOURCE']
assert proof['package'] == 'space.nuvio.nativelegacy.touch'
assert proof['label'] == 'Nuvio TV/Mobile Test' and proof['version_code'] == 20010
assert proof['version_name'] == '2.0.10-test-preload2'
assert proof['certificate_sha256'] == '862d7ef63a0dbde2929f6f231cae295510726d9ac28f9292496d2a38304be5e6'
assert proof['native_page_packaging'] == 'passed' and proof['abis'] == ['arm64-v8a', 'armeabi-v7a']
files = list(Path('delivery/apk').glob('*-tv-mobile-test.apk'))
assert len(files) == 1
data = files[0].read_bytes()
digest = hashlib.sha256(data).hexdigest()
assert digest == proof['apk_sha256'] and len(data) == proof['apk_bytes']
target = Path('delivery/Nuvio-TV-Mobile-Test-Preload2.apk')
shutil.copyfile(files[0], target)
target.with_suffix('.apk.sha256').write_text(digest + '  ' + target.name + '\n')
Path('delivery/release-notes.txt').write_text(
    'I updated source preloading in the compatible TV/Mobile test app.\n\n'
    'Enable the local video-preload setting to prepare a playable source while its title is open and reuse its buffer on Play. '
    'The same setting prepares the next episode near the credits while the current episode keeps playing. '
    'The build retains watched-movie filtering, provider failover, shared exclusion filters, language metadata and account/profile fixes.\n\n'
    'Pinned macOS full builds, native behavior regressions, Android builds for both ARM architectures, '
    '16 KB native packaging, and all original Media3 contracts plus first-audio/buffer startup instrumentation passed before publication. '
    'Those startup checks used the pinned emulator fixtures; physical-device startup confirmation is pending. '
    'V1 physical-device feedback prompted these changes.\n\n'
    'Package space.nuvio.nativelegacy.touch; label Nuvio TV/Mobile Test; versionCode 20010. '
    'The persistent preview signing certificate supports updating previous stable signed test builds while retaining accounts. '
    'Older temporary-key Actions ZIP installs still require the previously documented one-time transfer.\n\n'
    'Product source: ' + proof['source_commit'] + '\n'
    'CI review source: ' + proof['build_commit'] + '\n')
print('Pinned final APK verified:', digest)
