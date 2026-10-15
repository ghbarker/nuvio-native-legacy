#!/usr/bin/env python3
"""Verify the newly built standard APK and source-derived JNI descriptors."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import zipfile
from dex_static import apk_static_methods
from verify_source import verify

p = argparse.ArgumentParser()
p.add_argument('--case')
p.add_argument('--output', type=Path, required=True)
a = p.parse_args()
proof = verify(Path.cwd())
files = list(Path('build/android').glob('*-android-debug.apk'))
assert len(files) == 1, files
apk = files[0]
sdk = Path(os.environ['ANDROID_HOME']) / 'build-tools/35.0.0'
badging = subprocess.check_output([str(sdk / 'aapt'), 'dump', 'badging', str(apk)], text=True)
assert re.search(r"^package: name='space.nuvio.nativelegacy'", badging, re.M)
subprocess.run([str(sdk / 'apksigner'), 'verify', '--verbose', str(apk)], check=True)
abis = ['arm64-v8a', 'armeabi-v7a']
with zipfile.ZipFile(apk) as archive:
    names = set(archive.namelist())
    for abi in abis:
        for library in ['libmain.so', 'libSDL2.so', 'libcurl.so', 'libjpeg.so', 'libwebp.so', 'libffmpegJNI.so']:
            assert 'lib/' + abi + '/' + library in names, (abi, library)
subprocess.run([sys.executable, 'tools/android-apk-pages.py', str(apk)], check=True)
bridge = Path('src/video_android.c').read_text()
assert '"space.nuvio.nativelegacy.NvPlayer"' in bridge
lookups = re.findall(r'GetStaticMethodID\(env,\s*gCls,\s*"([^\"]+)",\s*"([^\"]+)"\)', bridge)
assert lookups and len(lookups) == len(set(lookups))
actual, dex_hash = apk_static_methods(apk)
assert set(lookups) <= actual, ('Missing compiled static JNI methods', set(lookups) - actual)
proof.update(outcome='passed', package='space.nuvio.nativelegacy', abis=abis,
             production_tool='bash tools/android.sh', product_source_modified_for_build=False,
             apk_sha256=hashlib.sha256(apk.read_bytes()).hexdigest(), apk_bytes=apk.stat().st_size,
             native_page_packaging='passed', standard_debug_signature_valid=True,
             static_jni_descriptors=dict(lookups), dex_sha256=dex_hash)
a.output.mkdir(parents=True, exist_ok=True)
(a.output / 'verified.json').write_text(json.dumps(proof, indent=2) + '\n')
print('Actual standard APK ARM32/ARM64, 16KB and source-derived static JNI descriptors passed')
