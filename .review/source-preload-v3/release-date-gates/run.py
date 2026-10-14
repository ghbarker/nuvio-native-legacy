#!/usr/bin/env python3
"""Linux/macOS meaningful date regressions against explicit, immutable source."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('--root', required=True, type=Path)
parser.add_argument('--output', required=True, type=Path)
parser.add_argument('--baseline', action='store_true')
parser.add_argument('--metadata-sdk', type=Path)
parser.add_argument('--controller-sdk', type=Path)
args = parser.parse_args()
fixtures = Path(__file__).resolve().parent
source = args.root.resolve()
output = args.output.resolve()
output.mkdir(parents=True, exist_ok=True)
metadata_sdk = args.metadata_sdk or fixtures.parent/'profile/sdk'
controller_sdk = args.controller_sdk or fixtures.parent/'controller/sdk'
legacy_review = fixtures.parents[1] / 'final-build-v3'
if not metadata_sdk.exists():
    metadata_sdk = legacy_review/'profile/sdk'
if not controller_sdk.exists():
    controller_sdk = legacy_review/'controller/sdk'
flags = ['-std=gnu11', '-O1', '-g', '-ffunction-sections', '-fdata-sections',
         '-Werror=implicit-function-declaration', '-I'+str(source/'src'), '-pthread']
if not args.baseline:
    flags += ['-DNV_DATE_RAW_FIELD=1']
if os.environ.get('SANITIZE', '0') == '1':
    flags += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
    if platform.system() == 'Linux':
        flags += ['-fno-pie', '-no-pie']
link = ['-Wl,-dead_strip' if platform.system() == 'Darwin' else '-Wl,--gc-sections', '-lm']
env = os.environ.copy()
env.setdefault('ASAN_OPTIONS', 'detect_leaks=0:halt_on_error=1')
env.setdefault('UBSAN_OPTIONS', 'halt_on_error=1:print_stacktrace=1')
receipt = {'source': str(source), 'baseline': args.baseline,
           'sanitize': os.environ.get('SANITIZE', '0'), 'results': []}

def execute(command, name, expected=0):
    p = subprocess.run(command, capture_output=True, text=True, env=env, timeout=45)
    (output/(name+'.log')).write_text(p.stdout+p.stderr)
    receipt['results'].append({'name': name, 'command': command, 'exit': p.returncode,
                              'expected': expected})
    print(p.stdout+p.stderr, end='', flush=True)
    if p.returncode != expected:
        (output/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
        raise SystemExit(p.returncode or 1)

bridge = output/'metadata_bridge.o'
execute([os.environ.get('CC','cc')]+flags+['-I'+str(metadata_sdk),'-c',
    str(fixtures/'metadata_bridge.c'),'-o',str(bridge)], 'metadata_bridge.build')
shared = [str(source/'src'/name) for name in ['idioma.c','proximo.c','js.c','metaprov.c']]
cases = [('date_joined', ['fontefalha.c'])]
if not args.baseline:
    cases.insert(0, ('date_metadata', []))
for name, modules in cases:
    binary = output/name
    execute([os.environ.get('CC','cc')]+flags+['-I'+str(controller_sdk),
        str(fixtures/(name+'.c')),str(bridge)]+shared+
        [str(source/'src'/module) for module in modules]+link+['-o',str(binary)], name+'.build')
    execute([str(binary)]+(['--baseline-control'] if args.baseline else []), name+'.run',
            42 if args.baseline else 0)

if not args.baseline:
    binary = output/'original_proximo'
    execute([os.environ.get('CC','cc')]+flags+['-I'+str(metadata_sdk),
        str(source/'tests/proximo.c')]+link+['-o',str(binary)], 'original_proximo.build')
    execute([str(binary)], 'original_proximo.run')
receipt['hashes'] = {str(path.relative_to(source)): hashlib.sha256(path.read_bytes()).hexdigest()
    for path in [source/'src'/n for n in ['catalogo.h','descoberta.c','idioma.c','proximo.c','posplay.c','proximopreparo.c']]}
receipt['status'] = 'PASS'
receipt['limits'] = [
    'Actual discovery/i18n, proximo policies, posplay and asynchronous Next controller; source/resolver/native boundary doubles.',
    'No phone/decoder performance claim; verifies eligible dates schedule preload and preserve same prepared token at automatic handoff.',
    'ASan/UBSan enabled when SANITIZE=1; leak detection disabled because managed ptrace blocks LeakSanitizer.'
]
(output/'receipt.json').write_text(json.dumps(receipt, indent=2)+'\n')
