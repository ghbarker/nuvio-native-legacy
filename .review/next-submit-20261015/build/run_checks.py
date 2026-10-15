#!/usr/bin/env python3
"""Actual macOS app builds and unchanged original guards on one pinned tree."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
from verify_source import plan, verify

p = argparse.ArgumentParser()
p.add_argument('--suite', choices=['core', 'fontepref', 'fontepref_file', 'player_retido'], required=True)
p.add_argument('--output', type=Path, required=True)
a = p.parse_args()
source = Path.cwd().resolve()
output = a.output.resolve()
output.mkdir(parents=True, exist_ok=True)
review = Path(__file__).resolve().parents[1]
value = plan()
source_proof = verify(source, value)
passed = []

def run(label, command, env=None):
    path = output / (label + '.log')
    with path.open('w') as log:
        result = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, env=env)
    if result.returncode:
        print('\n'.join(path.read_text(errors='replace').splitlines()[-80:]), flush=True)
        raise SystemExit(f'{label} failed with exit {result.returncode}; see {path}')
    passed.append(label)
    print(label + ': PASS', flush=True)

san_env = dict(os.environ, SANITIZE='1', ASAN_OPTIONS='detect_leaks=0:halt_on_error=1', UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
modules = sorted(str(path) for path in Path('src').glob('*.c')) + sorted(str(path) for path in Path('src/dts').glob('*.c'))
if a.suite == 'core':
    flags = ['-O1', '-DNV_SUPABASE_URL=""', '-DNV_SUPABASE_ANON_KEY=""', '-DNV_TV_LOGIN_BASE=""',
             '-DNV_TRAKT_CLIENT_ID=""', '-DNV_TRAKT_CLIENT_SECRET=""', '-DNV_SIMKL_CLIENT_ID=""',
             '-DNV_SIMKL_APP=""', '-DNV_TMDB_API_KEY=""', '-I/opt/homebrew/include',
             '-I/opt/homebrew/include/SDL2', '-L/opt/homebrew/lib', '-lSDL2', '-lSDL2_image',
             '-lSDL2_ttf', '-lz', '-framework', 'OpenGL', '-Wno-deprecated-declarations']
    for mode in ('tv', 'mobile'):
        run('build-' + mode, ['cc'] + modules + (['-DNV_TOUCH_UI=1'] if mode == 'mobile' else []) + ['-o', str(output / ('app-' + mode))] + flags)
    for name in ('player_recuo', 'posplay_recuo'):
        run(name + '-original', ['bash', 'tests/' + name + '.sh'])
    run('creditosauto-original', ['bash', 'tests/creditosauto.sh'], dict(os.environ, SANITIZE='0'))
    run('creditosauto-original-asan', ['bash', 'tests/creditosauto.sh'], san_env)
    run('targeted-generic-next', [sys.executable, str(review / 'targeted/run.py'), '--root', str(source),
        '--helpers', str(review), '--output', str(output / 'targeted'), '--mode', 'both'])
else:
    run(a.suite + '-original-asan', ['bash', 'tests/' + a.suite + '.sh'], san_env)

# The compiler wrapper keeps every original flag and adds only SDL3's rpath.
records = [json.loads(path.read_text()) for path in Path(os.environ['REVIEW_CC_OUTPUT']).glob('*-invocation.json')]
assert records
for record in records:
    assert record['returncode'] == 0, record
    assert record['effective_arguments'] == record['arguments'] + ([record['added_link_rpath']] if record['added_link_rpath'] else [])
    if record['added_link_rpath']:
        assert record['added_link_rpath'].startswith('-Wl,-rpath,')

original_compile_proofs = []
def original_compile(name, omitted, sanitized):
    matches = [record for record in records if 'tests/' + name + '.c' in record['arguments']]
    assert len(matches) == 1, (name, len(matches))
    record = matches[0]
    compiled = set(record['arguments']) & set(modules)
    assert compiled == set(modules) - omitted, (name, compiled ^ (set(modules) - omitted))
    assert ('-fsanitize=address,undefined' in record['arguments']) == sanitized
    assert record['added_link_rpath']
    original_compile_proofs.append({'original_script': 'tests/' + name + '.sh',
        'full_source_files_compiled': len(compiled), 'original_flags_unchanged': True,
        'sanitizers': ['address', 'undefined'] if sanitized else [], 'only_added_flag': record['added_link_rpath']})

if a.suite == 'core':
    original_compile('player_recuo', {'src/main.c', 'src/player.c'}, False)
    original_compile('posplay_recuo', {'src/main.c'}, False)
    credits = [record for record in records if 'tests/creditosauto.c' in record['arguments']]
    assert len(credits) == 2 and sum('-fsanitize=address,undefined' in record['arguments'] for record in credits) == 1
    report = json.loads((output / 'targeted/report.json').read_text())
    assert report['schema'] == 1 and report['passed'] is True and report['baseline_reproduction'] is False
    assert Path(report['source_root']).resolve() == source
    hashes = {path: hashlib.sha256((source / path).read_bytes()).hexdigest() for path in report['source_hashes']}
    assert set(hashes) == {'src/player.c', 'src/player.h', 'src/posplay.c', 'src/posplay.h', 'src/creditosauto.h',
                           'src/proximo.c', 'src/proximo.h', 'src/fontefalha.c', 'src/velocidade.c', 'src/relogio.c'}
    assert report['source_hashes'] == hashes
    assert len(report['runs']) == 4
    assert {(run['case'], run['mode']) for run in report['runs']} == {
        ('seek_gestures', 'normal'), ('seek_gestures', 'asan'), ('posplay_contracts', 'normal'), ('posplay_contracts', 'asan')}
    for result in report['runs']:
        assert result['passed'] is True and result['build_exit_code'] == result['exit_code'] == result['expected_exit_code'] == 0
        assert Path(result['source_root']).resolve() == source and result['source_hashes'] == hashes
        assert ('-fsanitize=address,undefined' in result['build_command']) == (result['mode'] == 'asan')
        assert ('-fsanitize=address,undefined' in result['sanitizer_flags']) == (result['mode'] == 'asan')
        assert '-DNV_ANDROID' in result['build_command']
    assert len(passed) == 7 and (output / 'app-tv').is_file() and (output / 'app-mobile').is_file()
else:
    omitted = {'src/main.c', 'src/fontepref.c'} if a.suite != 'player_retido' else {'src/main.c', 'src/player.c', 'src/video.c'}
    original_compile(a.suite, omitted, True)
    assert passed == [a.suite + '-original-asan']

verify(source, value)
proof = {'outcome': 'passed', 'suite': a.suite, 'source_commit': value['source_commit'],
         'review_commit': os.environ.get('GITHUB_SHA'), 'source_scope': source_proof['source_scope'],
         'gates': passed, 'original_test_sha256': source_proof['original_test_sha256'],
         'original_compile_proofs': original_compile_proofs,
         'compiler_invocations_verified': len(records),
         'targeted_cases': 4 if a.suite == 'core' else 0,
         'normal_and_supported_asan_only': True}
(output / 'verified.json').write_text(json.dumps(proof, indent=2) + '\n')
print('All exact-source checks passed:', a.suite, value['source_commit'])
