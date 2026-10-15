#!/usr/bin/env python3
"""Compile unmodified baseline/candidate C and compare every old Settings slot."""
import argparse
import platform
import hashlib
import json
from pathlib import Path
import subprocess

HERE = Path(__file__).resolve().parent
parser=argparse.ArgumentParser();parser.add_argument('--baseline',required=True,type=Path);parser.add_argument('--source',required=True,type=Path);parser.add_argument('--output',required=True,type=Path);parser.add_argument('--cc',default='/usr/bin/cc');args=parser.parse_args()
BASELINE=args.baseline.resolve()
CANDIDATE=args.source.resolve()
OUT=args.output.resolve()
OUT.mkdir(parents=True,exist_ok=False)
records = []
def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()
def run(name, command):
    result = subprocess.run(command, text=True, capture_output=True)
    (OUT / (name + '.log')).write_text(result.stdout + result.stderr)
    records.append({'name': name, 'arguments': command, 'exit_code': result.returncode,
                    'output_sha256': sha(OUT / (name + '.log'))})
    assert result.returncode == 0, result.stderr
    return result.stdout

sdk = ['-include', str(HERE / 'c-compat.h'), '-I' + str(HERE / 'c-sdk')]
outputs = {}
for mode, defines in [('desktop-tv', []), ('desktop-touch', ['-DNV_TOUCH_UI']),
                      ('android-tv', ['-DNV_ANDROID']), ('android-mobile', ['-DNV_ANDROID', '-DNV_TOUCH_UI'])]:
    for name, source in [('baseline', BASELINE), ('candidate', CANDIDATE)]:
        target = OUT / (name + '-' + mode)
        common = [args.cc, '-std=gnu11', '-O1', '-g', '-ffunction-sections', '-fdata-sections',
                  '-Werror=implicit-function-declaration', *defines, *sdk, '-I' + str(source / 'src')]
        run(name + '-' + mode + '-compile', [*common,
            '-DAJUSTES_SOURCE="' + str(source / 'src/ajustes.c') + '"', str(HERE / 'settings-tables.c'),
            ('-Wl,-dead_strip' if platform.system()=='Darwin' else '-Wl,--gc-sections'), '-lm', '-o', str(target)])
        outputs[name, mode] = run(name + '-' + mode + '-execute', [str(target)]).splitlines()
        run(name + '-' + mode + '-android-c', [*common, '-fsyntax-only', str(source / 'src/android.c')])
    old, new = outputs['baseline', mode], outputs['candidate', mode]
    if mode.startswith('android'):
        assert new[:-1] == old and new[-1].endswith('\t-baixarRegistro\t0\tBaixar registro'), mode
    else:
        assert new == old, mode
    print('PASS all existing Settings IDs, keys, defaults and labels unchanged:', mode, len(old))

for sanitizer in (False, True):
    target = OUT / ('jni-asan' if sanitizer else 'jni-normal')
    flags = ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer']+(['-fno-pie','-no-pie'] if platform.system()=='Linux' else []) if sanitizer else []
    command = [args.cc, '-std=gnu11', '-O1', '-g', '-DNV_ANDROID', '-DNV_TOUCH_UI',
               '-ffunction-sections', '-fdata-sections', *flags, *sdk, '-I' + str(CANDIDATE / 'src'),
               '-DANDROID_SOURCE="' + str(CANDIDATE / 'src/android.c') + '"', str(HERE / 'jni-bridge.c'),
               ('-Wl,-dead_strip' if platform.system()=='Darwin' else '-Wl,--gc-sections'), '-pthread', '-o', str(target)]
    run(target.name + '-compile', command)
    result = run(target.name + '-execute', [str(target)])
    assert len(result.splitlines()) == 9
    print('PASS actual C JNI bridge, signature, null/error paths and reference cleanup:', target.name)

assert all((BASELINE / path).read_bytes() == (CANDIDATE / path).read_bytes()
           for path in ('src/avisos.c', 'src/registro.c', 'src/recomenda.c', 'src/queda.c',
                        'android/app/src/main/AndroidManifest.xml', 'android/app/build.gradle.kts'))
report = {'outcome': 'passed', 'baseline': 'e3e21a590120ca283bcf7a96df1813ff5041eb66',
          'records': records, 'settings_modes': list(dict.fromkeys(k[1] for k in outputs)),
          'original_reporting_viewer_crash_backend_and_identity_unchanged': True,
          'settings_source_sha256': sha(CANDIDATE / 'src/ajustes.c'),
          'android_source_sha256': sha(CANDIDATE / 'src/android.c'),
          'sdk_scope': 'public OpenJDK JNI headers; SDL/Android/GLES declaration doubles, no device or ARM execution'}
(OUT / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
