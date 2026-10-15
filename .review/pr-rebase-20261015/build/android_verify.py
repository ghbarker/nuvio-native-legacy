#!/usr/bin/env python3
"""Verify the standard production-tool APK and both actual ARM application cores."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import zipfile
from verify_source import plan, verify

p = argparse.ArgumentParser()
p.add_argument('--case', required=True)
p.add_argument('--output', type=Path, required=True)
a = p.parse_args()
root = Path.cwd().resolve()
value, case = plan(a.case)
proof = verify(root, value, case)
files = list(Path('build/android').glob('*-android-debug.apk'))
assert len(files) == 1, files
apk = files[0]
sdk = Path(__import__('os').environ['ANDROID_HOME'])
badging = subprocess.check_output([str(sdk / 'build-tools/35.0.0/aapt'), 'dump', 'badging', str(apk)], text=True)
assert re.search(r"^package: name='space.nuvio.nativelegacy'", badging, re.M)
subprocess.run([str(sdk / 'build-tools/35.0.0/apksigner'), 'verify', '--verbose', str(apk)], check=True)
abis = ['arm64-v8a', 'armeabi-v7a']
with zipfile.ZipFile(apk) as archive:
    names = set(archive.namelist())
    for abi in abis:
        for lib in ('libmain.so', 'libSDL2.so', 'libcurl.so', 'libjpeg.so', 'libwebp.so', 'libffmpegJNI.so'):
            assert 'lib/' + abi + '/' + lib in names, (abi, lib)
    assert any(name.startswith('assets/art/') for name in names)
    assert any(name.startswith('assets/fonts/') for name in names)
subprocess.run([sys.executable, 'tools/android-apk-pages.py', str(apk)], check=True)
a.output.mkdir(parents=True, exist_ok=True)
proof.update({'outcome': 'passed', 'package': 'space.nuvio.nativelegacy', 'abis': abis,
    'production_tool': 'bash tools/android.sh', 'product_source_modified_for_build': False,
    'apk_sha256': hashlib.sha256(apk.read_bytes()).hexdigest(), 'apk_bytes': apk.stat().st_size,
    'native_page_packaging': 'passed', 'standard_debug_signature_valid': True})
(a.output / 'verified.json').write_text(json.dumps(proof, indent=2) + '\n')
print('Standard production-tool APK and both ARM cores passed:', a.case, flush=True)
