#!/usr/bin/env python3
"""Validate the actual standard ARM APK and real Android Kotlin/JNI compilation."""
import argparse, hashlib, json, re, shlex, subprocess, zipfile
from pathlib import Path
from dex_methods import apk_methods
p = argparse.ArgumentParser()
for name in ('source', 'proof', 'output'): p.add_argument('--' + name, type=Path, required=True)
for name in ('aapt', 'apksigner'): p.add_argument('--' + name, required=True)
a = p.parse_args(); source = a.source.resolve(); here = Path(__file__).resolve().parent
proof = json.loads(a.proof.read_text()); assert proof['verified']
manifest = json.loads((here / 'source-manifest.json').read_text())
assert proof['source_commit'] == manifest['head']
def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()
for e in manifest['paths']: assert sha(source / e['path']) == e['sha256']
files = list((source / 'build/android').glob('*-android-debug.apk')); assert len(files) == 1
apk = files[0]
badging = subprocess.check_output([a.aapt, 'dump', 'badging', str(apk)], text=True)
version = json.loads((source / 'deploy/app/appinfo.json').read_text())['version']
x, y, z = map(int, version.split('.')); code = x * 10000 + y * 100 + z
assert "package: name='space.nuvio.nativelegacy'" in badging
assert "versionName='" + version + "'" in badging and "versionCode='" + str(code) + "'" in badging
assert 'Nuvio TV/Mobile Test' not in badging
signing = subprocess.check_output([a.apksigner, 'verify', '--verbose', '--print-certs', str(apk)], text=True)
certificate = re.search(r'Signer #1 certificate SHA-256 digest: ([0-9a-f]{64})', signing); assert certificate
subprocess.run(['python3', str(source / 'tools/android-apk-pages.py'), str(apk)], check=True)
with zipfile.ZipFile(apk) as archive:
    names = set(archive.namelist())
    assert all('lib/' + abi + '/libmain.so' in names for abi in ('arm64-v8a', 'armeabi-v7a'))
classes = source / 'android/app/build/tmp/kotlin-classes/debug'
descriptors = {
    'NuvioActivity': [('baixarRegistro', '(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V'),
                      ('onActivityResult', '(IILandroid/content/Intent;)V')],
    'RegistroDownload': [('abrir', '(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V'),
                         ('resultado', '(IILandroid/content/Intent;)Z'), ('fechar', '()V')],
    'RegistroDownload$Companion': [('documento', '()Landroid/content/Intent;'),
                                   ('lerFim', '(Ljava/io/File;)Ljava/lang/String;'),
                                   ('limpar', '(Ljava/lang/String;)Ljava/lang/String;')]
}
compiled = {}; dex_hashes = None
for klass, required in descriptors.items():
    name = 'space.nuvio.nativelegacy.' + klass
    file = classes / (name.replace('.', '/') + '.class'); assert file.is_file()
    text = subprocess.check_output(['javap', '-s', '-p', '-classpath', str(classes), name], text=True)
    actual, hashes = apk_methods(apk, 'L' + name.replace('.', '/') + ';')
    dex_hashes = hashes
    for method, descriptor in required:
        assert re.search(r'\b' + re.escape(method) + r'\([^\n]*\);\s*descriptor:\s*' + re.escape(descriptor), text)
        assert (method, descriptor) in actual and not (actual[method, descriptor] & 0x8)
    compiled[name] = {'class_sha256': sha(file), 'methods': required,
                      'javap_sha256': hashlib.sha256(text.encode()).hexdigest()}
bridge = (source / 'src/android.c').read_text()
new_signature = re.findall(r'GetMethodID\(env,\s*cls,\s*"baixarRegistro",\s*"([^"]+)"\)', bridge)
assert new_signature == [descriptors['NuvioActivity'][0][1]]
player_bridge = (source / 'src/video_android.c').read_text()
lookups = re.findall(r'GetStaticMethodID\(env,\s*gCls,\s*"([^"]+)",\s*"([^"]+)"\)', player_bridge)
assert lookups and len(set(lookups)) == len(lookups)
player, _ = apk_methods(apk, 'Lspace/nuvio/nativelegacy/NvPlayer;')
for signature in lookups: assert signature in player and player[signature] & 0x8
private, _ = apk_methods(apk, 'Lspace/nuvio/nativelegacy/TestLocalLogs;'); assert not private
arms = []
for abi in ('arm64-v8a', 'armeabi-v7a'):
    found = []
    for file in (source / 'android/app').rglob('compile_commands.json'):
        if abi not in file.parts: continue
        required = set()
        for entry in json.loads(file.read_text()):
            path = Path(entry['file']).resolve()
            if path not in {source / 'src/android.c', source / 'src/ajustes.c'}: continue
            argv = entry.get('arguments') or shlex.split(entry['command'])
            assert any(x in argv for x in ('-DNV_ANDROID', '-DNV_ANDROID=1'))
            assert any(x in argv for x in ('-DNV_TOUCH_UI', '-DNV_TOUCH_UI=1'))
            assert not any('NV_TEST_LOCAL_LOGS' in x for x in argv)
            required.add(path.name)
        if required == {'android.c', 'ajustes.c'}: found.append({'compile_commands_sha256': sha(file), 'sources': sorted(required)})
    assert found, 'No actual complete ARM compilation proof for ' + abi
    arms.append({'abi': abi, 'builds': found})
result = {'outcome': 'passed', 'source_commit': proof['source_commit'], 'review_commit': proof['review_commit'],
          'apk_sha256': sha(apk), 'apk_bytes': apk.stat().st_size, 'package': 'space.nuvio.nativelegacy',
          'version': version, 'version_code': code, 'certificate_sha256': certificate.group(1),
          'signer_scope': 'CI-generated standard debug signature, not an installable update for the private test app',
          'native_16kb_pages': 'passed', 'compiled_android_classes': compiled,
          'new_activity_jni_descriptor': new_signature[0], 'all_player_jni_lookups': lookups,
          'apk_dex_sha256': dex_hashes, 'actual_arm_compiles': arms,
          'scope': 'Actual complete Android Kotlin/API and ARM32/ARM64 compilation plus real APK DEX/JNI/package validation; no emulator or physical-device execution claimed.',
          'release_or_backend_actions': False}
a.output.parent.mkdir(parents=True, exist_ok=True); a.output.write_text(json.dumps(result, indent=2) + '\n')
print('PASS actual Android Activity/helper/JNI,', len(lookups), 'player lookups, both ARM cores and 16KB APK:', result['apk_sha256'])
