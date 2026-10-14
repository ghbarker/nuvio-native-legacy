#!/usr/bin/env python3
import hashlib
import json
import os
from pathlib import Path
import shutil

proof = json.loads(Path('delivery/proof/update-identity.json').read_text())
assert proof['verified'] and proof['phase'] == 'final' and proof['test_package']
assert proof['source_commit'] == os.environ['PLANNED_SOURCE']
assert proof['package'] == 'space.nuvio.nativelegacy.touch'
assert proof['label'] == 'Nuvio TV/Mobile Test' and proof['version_code'] == 20009
assert proof['version_name'] == '2.0.9-test-preload1'
assert proof['certificate_sha256'] == '862d7ef63a0dbde2929f6f231cae295510726d9ac28f9292496d2a38304be5e6'
assert proof['native_page_packaging'] == 'passed' and proof['abis'] == ['arm64-v8a', 'armeabi-v7a']
files = list(Path('delivery/apk').glob('*-tv-mobile-test.apk'))
assert len(files) == 1
data = files[0].read_bytes()
digest = hashlib.sha256(data).hexdigest()
assert digest == proof['apk_sha256'] and len(data) == proof['apk_bytes']
target = Path('delivery/Nuvio-TV-Mobile-Test-Preload1.apk')
shutil.copyfile(files[0], target)
target.with_suffix('.apk.sha256').write_text(digest + '  ' + target.name + '\n')
Path('delivery/release-notes.txt').write_text(
    'I added source preloading to the compatible TV/Mobile test app.\n\n'
    'The first playable source can prepare while its title is open, and Play adopts the prepared player. '
    'The build retains watched-movie filtering, provider failover, shared exclusion filters, language metadata and account/profile fixes.\n\n'
    'Pinned macOS full builds, native behavior regressions, Android builds for both ARM architectures, '
    '16 KB native packaging, and real Media3 paused-load/adoption instrumentation passed before publication.\n\n'
    'Package space.nuvio.nativelegacy.touch; label Nuvio TV/Mobile Test; versionCode 20009. '
    'The persistent preview signing certificate supports updating previous stable signed test builds while retaining accounts. '
    'Older temporary-key Actions ZIP installs still require the previously documented one-time transfer.\n\n'
    'Product source: ' + proof['source_commit'] + '\n'
    'CI review source: ' + proof['build_commit'] + '\n')
print('Pinned final APK verified:', digest)
