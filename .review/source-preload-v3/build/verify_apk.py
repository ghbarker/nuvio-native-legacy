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
    assert "versionCode='20015'" in badging and "versionName='2.0.15-test-logs5'" in badging
log_provider = None
if args.test:
    # Verify the compiled authority after the existing compatible-app identity
    # transform, rather than trusting only the source manifest replacement.
    manifest_xml = subprocess.check_output([args.aapt, 'dump', 'xmltree', str(apk), 'AndroidManifest.xml'], text=True)
    lines = manifest_xml.splitlines()
    providers = []
    for index, line in enumerate(lines):
        match = re.match(r'^(\s*)E: provider\b', line)
        if not match:
            continue
        indent = len(match.group(1))
        end = next((i for i in range(index + 1, len(lines))
                    if re.match(r'^\s*E:', lines[i]) and len(lines[i]) - len(lines[i].lstrip()) <= indent), len(lines))
        providers.append('\n'.join(lines[index:end]))
    authority = package + '.atualizacao'
    matches = [block for block in providers if re.search(r'android:authorities[^\n]*="' + re.escape(authority) + r'"', block)]
    assert len(matches) == 1, ('Compiled FileProvider authority mismatch', authority)
    block = matches[0]
    assert 'androidx.core.content.FileProvider' in block
    assert re.search(r'android:exported[^\n]*\(type\s+0x12\)0x0\b', block)
    assert re.search(r'android:grantUriPermissions[^\n]*\(type\s+0x12\)0x(?:ffffffff|1)\b', block)
    assert 'android.support.FILE_PROVIDER_PATHS' in block
    paths_xml = subprocess.check_output([args.aapt, 'dump', 'xmltree', str(apk), 'res/xml/caminhos_atualizacao.xml'], text=True)
    assert len(re.findall(r'E: cache-path\b', paths_xml)) == 1
    assert 'name="test_logs"' in paths_xml and 'path="test-local-logs/"' in paths_xml
    assert not re.search(r'E: (?:root-path|external-path|external-cache-path)\b', paths_xml)
    log_provider = {'authority': authority, 'nonexported': True,
                    'read_grants_supported': True, 'bounded_test_cache_path': True}
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
    proof.update(label='Nuvio TV/Mobile Test', version_code=20015, version_name='2.0.15-test-logs5')
    proof['test_local_logs_enabled'] = source['case'] == 'combined-delivery'
    proof['log_file_provider'] = log_provider
output = Path('build/review/update-identity.json' if args.test else 'build/review/standard-identity.json')
output.write_text(json.dumps(proof, indent=2) + '\n')
print('Verified', package, 'both ARM cores, persistent certificate, 16 KB packaging:', proof['apk_sha256'])
