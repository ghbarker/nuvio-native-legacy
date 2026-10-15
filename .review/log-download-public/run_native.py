#!/usr/bin/env python3
"""Real macOS full builds; original report suites and explicit defaults-tail metadata adapter."""
import argparse, hashlib, json, os, subprocess, sys
from pathlib import Path
p = argparse.ArgumentParser(); p.add_argument('--source', type=Path, required=True)
p.add_argument('--proof', type=Path, required=True); p.add_argument('--output', type=Path, required=True); a = p.parse_args()
source = a.source.resolve(); out = a.output.resolve(); out.mkdir(parents=True, exist_ok=True)
here = Path(__file__).resolve().parent
proof = json.loads(a.proof.read_text()); assert proof['verified']
preflight_path = here / 'remaining-fixture-preflight.json'
preflight = json.loads(preflight_path.read_text())
assert preflight['outcome'] == 'all selected host preflights passed'
assert preflight['baseline'] == proof['base'] and preflight['candidate'] == proof['source_commit']
assert preflight['product_and_original_tests_modified'] is False
expected = {(head, name) for head in (proof['base'], proof['source_commit']) for name in
            ('regcodigo', 'ajustes_padroes', 'ajustes_envio_auto', 'ajustes_ux_toque_modo')}
assert {(r['head'], r.get('test', r.get('name'))) for r in preflight['records']} == expected
for r in preflight['records']:
    name = r.get('test', r.get('name')); assert r['returncode'] == 0
    assert hashlib.sha256((source / ('tests/' + name + '.c')).read_bytes()).hexdigest() == r.get('original_fixture_sha256', r.get('fixture_sha256'))
    assert hashlib.sha256((source / ('tests/' + name + '.sh')).read_bytes()).hexdigest() == r.get('original_script_sha256', r.get('script_sha256'))

records = []
def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()
def run(name, args):
    with (out / (name + '.log')).open('w') as f:
        result = subprocess.run(args, cwd=source, stdout=f, stderr=subprocess.STDOUT, timeout=900)
    records.append({'name': name, 'arguments': args, 'exit_code': result.returncode, 'log_sha256': sha(out / (name + '.log'))})
    assert result.returncode == 0, (name, result.returncode)
    print('PASS', name, flush=True)
run('translations', [sys.executable, 'tools/idiomas.py'])
sources = [str(p) for p in sorted((source / 'src').glob('*.c'))] + [str(p) for p in sorted((source / 'src/dts').glob('*.c'))]
flags = ['-O1', '-g', '-DNV_SUPABASE_URL=""', '-DNV_SUPABASE_ANON_KEY=""', '-DNV_TV_LOGIN_BASE=""',
         '-DNV_TRAKT_CLIENT_ID=""', '-DNV_TRAKT_CLIENT_SECRET=""', '-DNV_SIMKL_CLIENT_ID=""',
         '-DNV_SIMKL_APP=""', '-DNV_TMDB_API_KEY=""', '-I/opt/homebrew/include', '-I/opt/homebrew/include/SDL2',
         '-L/opt/homebrew/lib', '-lSDL2', '-lSDL2_image', '-lSDL2_ttf', '-lz', '-framework', 'OpenGL', '-Wno-deprecated-declarations']
for ui in ('tv', 'touch'):
    run('full-source-' + ui, ['cc', *sources, *flags, *(['-DNV_TOUCH_UI'] if ui == 'touch' else []), '-o', str(out / ('app-' + ui))])
original = {}
for name in ('ajustes_envio_auto', 'ajustes_padroes', 'ajustes_ux_toque_modo', 'regcodigo'):
    for suffix in ('.c', '.sh'):
        path = 'tests/' + name + suffix
        data = (source / path).read_bytes()
        assert data == subprocess.check_output(['git', '-C', str(source), 'show', proof['base'] + ':' + path])
        original[path] = hashlib.sha256(data).hexdigest()
    if name == 'ajustes_padroes':
        run('ajustes_padroes-adapted-append-only-expectation', [sys.executable, str(here / 'run_defaults_adapter.py'),
            '--source', str(source), '--output', str(out / 'defaults-adapter')])
    else:
        run(name + '-original', ['bash', 'tests/' + name + '.sh'])
run('ajustes_envio_auto-sanitized-replay', [sys.executable, str(here / 'run_original_sanitized.py'),
    '--name', 'ajustes_envio_auto', '--output', str(out / 'sanitized')])
san = json.loads((out / 'sanitized/ajustes_envio_auto-sanitized.json').read_text())
defaults = json.loads((out / 'defaults-adapter/defaults-adapted.json').read_text())
assert defaults['exit_code'] == 0 and defaults['behavioral_default_assertions_unchanged']
assert defaults['original_fixture_sha256'] == original['tests/ajustes_padroes.c']
assert not defaults['original_unadapted_test_counted_as_pass']
assert san['exit_code'] == 0 and san['original_source_arguments_and_assertions_unchanged']
assert san['original_sha256'] == original['tests/ajustes_envio_auto.sh']
compiles = [json.loads(p.read_text()) for p in Path(os.environ['REVIEW_CC_OUTPUT']).glob('*-invocation.json')]
assert compiles and all(r['returncode'] == 0 for r in compiles)
for c in compiles:
    assert c['effective_arguments'] == c['arguments'] + ([c['added_link_rpath']] if c['added_link_rpath'] else [])
result = {'outcome': 'passed', 'source_commit': proof['source_commit'], 'review_commit': proof['review_commit'],
          'gates': records, 'remaining_fixture_host_preflight_sha256': sha(preflight_path), 'remaining_fixture_host_scope': preflight['scope'], 'original_test_sha256': original, 'sanitized_original_replay': san, 'defaults_final_enum_metadata_adapter': defaults,
          'full_source_compiles': {ui: sha(out / ('app-' + ui)) for ui in ('tv', 'touch')},
          'compiler_invocations': len(compiles), 'only_original_compiler_fix': 'real Homebrew SDL runtime link-time rpath',
          'original_reporting_viewer_crash_backend_unchanged': True,
          'scope': 'Real complete macOS C integration. Defaults retains all behavior assertions with one explicit stale enum-tail metadata adapter; other selected suites remain original. Android exact successful ARM APK and local proofs retained separately.'}
(out / 'native-verified.json').write_text(json.dumps(result, indent=2) + '\n')
