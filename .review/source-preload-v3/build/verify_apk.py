#!/usr/bin/env python3
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import zipfile

CERT = '862d7ef63a0dbde2929f6f231cae295510726d9ac28f9292496d2a38304be5e6'
parser = argparse.ArgumentParser()
parser.add_argument('--test', action='store_true')
parser.add_argument('--aapt', required=True)
parser.add_argument('--apksigner', required=True)
parser.add_argument('--proof', default='build/review/source-verified.json')
args = parser.parse_args()
pattern = '*-tv-mobile-test.apk' if args.test else '*-android-debug.apk'
matches = list(Path('build/android').glob(pattern))
assert len(matches) == 1, (pattern, matches)
apk = matches[0]
badging = subprocess.check_output([args.aapt, 'dump', 'badging', str(apk)], text=True)
package = 'space.nuvio.nativelegacy.touch' if args.test else 'space.nuvio.nativelegacy'
assert re.search(r"^package: name='" + re.escape(package) + "'", badging, re.M)
if args.test:
    assert re.search(r"^application-label:'Nuvio TV/Mobile Test'", badging, re.M)
    assert "versionCode='20012'" in badging and "versionName='2.0.12-test-logs2'" in badging
signing = subprocess.check_output([args.apksigner, 'verify', '--verbose', '--print-certs', str(apk)], text=True)
assert 'Signer #1 certificate SHA-256 digest: ' + CERT in signing
with zipfile.ZipFile(apk) as archive:
    names = set(archive.namelist())
for abi in ('arm64-v8a', 'armeabi-v7a'):
    assert 'lib/' + abi + '/libmain.so' in names, abi
subprocess.run([sys.executable, 'tools/android-apk-pages.py', str(apk)], check=True)
source = json.loads(Path(args.proof).read_text())
assert source['verified']
if source['case'] == 'combined-delivery':
    log_build = Path('build/review/test-log-build-' + ('touch' if args.test else 'standard') + '.json')
    subprocess.run([sys.executable, str(Path(__file__).with_name('verify_log_build.py')),
                    '--enabled', 'true' if args.test else 'false', '--output', str(log_build),
                    '--source-proof', args.proof], check=True)
proof = {**source, 'apk_sha256': hashlib.sha256(apk.read_bytes()).hexdigest(),
         'apk_bytes': apk.stat().st_size, 'apk_name': apk.name, 'package': package,
         'abis': ['arm64-v8a', 'armeabi-v7a'], 'certificate_sha256': CERT,
         'native_page_packaging': 'passed', 'test_package': args.test}
if args.test:
    proof.update(label='Nuvio TV/Mobile Test', version_code=20012, version_name='2.0.12-test-logs2')
    proof['test_local_logs_enabled'] = source['case'] == 'combined-delivery'
output = Path('build/review/update-identity.json' if args.test else 'build/review/standard-identity.json')
output.write_text(json.dumps(proof, indent=2) + '\n')
print('Verified', package, 'both ARM cores, persistent certificate, 16 KB packaging:', proof['apk_sha256'])
