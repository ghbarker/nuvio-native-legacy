#!/usr/bin/env python3
"""Verify additive local download opt-in and preserved original reporting in real ARM builds."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shlex
import runpy

p = argparse.ArgumentParser()
p.add_argument('--enabled', choices=['true', 'false'], required=True)
p.add_argument('--output', type=Path, required=True)
p.add_argument('--source-proof', type=Path, default=Path('build/review/source-verified.json'))
a = p.parse_args()
expected = a.enabled == 'true'
here = Path(__file__).resolve().parent
original_helper = here / 'test-local-logs/verify-original.py'
if not original_helper.is_file(): original_helper = here.parent / 'test-local-logs/verify-original.py'
original = runpy.run_path(str(original_helper))['check'](Path.cwd())
proof = json.loads(a.source_proof.read_text())
assert proof['verified'] and proof['case'] == 'combined-delivery'
configs = list(Path('android/app/build/generated').rglob('BuildConfig.java'))
fields = []
for config in configs:
    content = config.read_text()
    match = re.search(r'public static final boolean NV_TEST_LOCAL_LOGS\s*=\s*(true|false)\s*;', content)
    if match:
        fields.append((config, match.group(1) == 'true'))
assert len(fields) == 1, 'Expected one generated debug test-log BuildConfig field'
config, actual = fields[0]
assert actual == expected, 'Generated Kotlin test-log opt-in differs from requested build'
verified = []
for abi in ['arm64-v8a', 'armeabi-v7a']:
    selected = []
    for commands in Path('android/app').rglob('compile_commands.json'):
        if abi not in commands.parts:
            continue
        cache = commands.parent / 'CMakeCache.txt'
        if not cache.is_file():
            continue
        cache_body = cache.read_text(errors='replace')
        marker = re.search(r'^NV_TEST_LOCAL_LOGS:[^=\n]+=(ON|OFF)\s*$', cache_body, re.M)
        if not marker or (marker.group(1) == 'ON') != expected:
            continue
        main = []
        endpoint_configured = set()
        for command in json.loads(commands.read_text()):
            argv = command.get('arguments') or shlex.split(command['command'])
            if not any('CMakeFiles/main.dir/' in arg for arg in argv):
                continue
            rec = [arg.split('=', 1)[1] for arg in argv if arg.startswith('-DNV_REC_URL=')]
            assert len(rec) == 1, 'Original report endpoint macro must be passed through'
            endpoint_configured.add(bool(rec[0].replace(chr(92), '').strip(chr(34))))
            enabled = '-DNV_TEST_LOCAL_LOGS=1' in argv
            assert enabled == expected, 'Actual ARM C compilation disagrees with test-only opt-in'
            main.append(Path(command['file']).name)
        assert {'app.c', 'posplay.c', 'avisos.c', 'android.c'} <= set(main), 'Missing real diagnostic/upload/Activity C compilation'
        assert endpoint_configured == {True}, 'Original Send logs requires the configured report endpoint in both actual ARM builds'
        selected.append({'report_endpoint_configured': endpoint_configured.pop(), 'commands_sha256': hashlib.sha256(commands.read_bytes()).hexdigest(),
                         'main_c_sources': len(main), 'cmake_opt_in': marker.group(1),
                         'native_macro_enabled': expected})
    assert selected, 'No matching actual compile_commands proof for ' + abi
    verified.append({'abi': abi, 'builds': selected})
result = {'outcome': 'passed', 'source_commit': proof['source_commit'],
          'review_commit': proof['build_commit'], 'case': proof['case'],
          'kotlin_opt_in': actual, 'native_macro_enabled': expected,
          'build_config_sha256': hashlib.sha256(config.read_bytes()).hexdigest(),
          'arm_builds': verified, 'test_only': True,
          'activity_saf_phone_test_pending': True, 'original_reporting_contract': original}
a.output.parent.mkdir(parents=True, exist_ok=True)
a.output.write_text(json.dumps(result, indent=2) + '\n')
print('Verified additive Download logs opt-in:', a.enabled, 'real Kotlin and both ARM cores; original reporting preserved')
