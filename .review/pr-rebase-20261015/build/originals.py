#!/usr/bin/env python3
"""Run the retained legacy regression gates with isolated compatibility imports."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
from verify_source import plan, verify, REVIEW

GROUPS = {
    'profile-state': ['homeestado', 'ordemperfil', 'ajustes_perfil', 'perfilsel'],
    'source-prefs': ['legmemoria', 'fontevolta', 'fontecache_vod', 'addons_legendas', 'i18n'],
    'failover': ['addons_proveniencia', 'ajustes_failover', 'fontefalha', 'fontefalha_worker', 'fontefalha_playback'],
    'providers': ['debrid', 'fontepref_auto', 'fontesmortas_orcamento', 'player_android_retomada', 'ajustes_ux_toque_modo', 'fonte_ordem_addon'],
    'catalog': [],
}
p = argparse.ArgumentParser()
p.add_argument('--group', choices=list(GROUPS), required=True)
p.add_argument('--output', type=Path, required=True)
a = p.parse_args()
root = Path.cwd().resolve()
value, case = plan('pr493')
proof = verify(root, value, case)
out = a.output.resolve()
out.mkdir(parents=True, exist_ok=True)
companions = out / 'legacy-companions'
helper = REVIEW / 'fixtures/pr493/legacy'
subprocess.run([sys.executable, str(helper / 'make_legacy_companions.py'), str(root), str(companions)], check=True)
ccbin = out / 'cc-bin'
ccbin.mkdir(exist_ok=True)
compiler = shutil.which('cc')
assert compiler
shutil.copyfile(helper / 'legacy-cc.py', ccbin / 'cc')
(ccbin / 'cc').chmod(0o755)
env = dict(os.environ, SANITIZE='1', FONTEFALHA_SANITIZE='1', ASAN_OPTIONS='detect_leaks=0:halt_on_error=1',
    UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1', TMPDIR='/tmp',
    PROFILE_LEGACY_COMPANIONS=str(companions), PROFILE_LEGACY_REAL_CC=compiler,
    PATH=str(ccbin) + os.pathsep + os.environ['PATH'])
commands = [('/bin/bash', 'tests/' + name + '.sh') for name in GROUPS[a.group]]
if a.group == 'catalog':
    modules = sorted(str(p) for p in Path('src').glob('*.c')) + sorted(str(p) for p in Path('src/dts').glob('*.c'))
    modules = [p for p in modules if p not in ('src/main.c', 'src/vertudo.c')]
    commands = [('cc', *modules, 'tests/colecao_exibicao.c', '-Isrc', '-o', str(out / 'catalog-display'), '-O1', '-g',
        '-I/opt/homebrew/include', '-I/opt/homebrew/include/SDL2', '-L/opt/homebrew/lib', '-lSDL2', '-lSDL2_image',
        '-lSDL2_ttf', '-lz', '-framework', 'OpenGL', '-Wno-deprecated-declarations', '-Wno-macro-redefined'),
        (str(out / 'catalog-display'),)]
runs = []
for index, command in enumerate(commands):
    name = GROUPS[a.group][index] if a.group != 'catalog' else ('catalog-build' if index == 0 else 'catalog-run')
    logfile = out / (name + '.log')
    with logfile.open('w') as log:
        result = subprocess.run(command, env=env, stdout=log, stderr=subprocess.STDOUT, timeout=1200)
    if result.returncode:
        print('\n'.join(logfile.read_text(errors='replace').splitlines()[-100:]), flush=True)
        raise SystemExit('Legacy regression failed: ' + name)
    runs.append({'name': name, 'status': 'passed', 'command': command, 'exit_code': result.returncode})
    verify(root, value, case)
    print('Original regression PASS:', name, flush=True)
manifest = json.loads((companions / 'manifest.json').read_text())
assert all(f['assertions_unchanged'] for f in manifest['files'])
proof.update({'outcome': 'passed', 'group': a.group, 'gates': runs,
    'original_product_test_bytes_unchanged': True, 'legacy_companion_manifest': manifest,
    'legacy_adapter_scope': 'External reduced-fixture imports plus documented setup and upstream #205 ordering vectors; every assertion expression retained'})
(out / 'verified.json').write_text(json.dumps(proof, indent=2) + '\n')
