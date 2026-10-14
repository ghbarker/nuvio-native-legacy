#!/usr/bin/env python3
"""Actual VOD cache/addons, all current assertions plus actor-cache races."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('--source', '--root', dest='root', type=Path, required=True)
parser.add_argument('--output', type=Path, required=True)
parser.add_argument('--sdk', type=Path, default=Path(__file__).resolve().parent / '../controller/sdk')
parser.add_argument('--sanitize', choices=['0', '1'], default=os.environ.get('SANITIZE', '0'))
args = parser.parse_args()
bundle = Path(__file__).resolve().parent
source = args.root.resolve()
out = args.output.resolve() / ('asan-ubsan' if args.sanitize == '1' else 'normal')
out.mkdir(parents=True, exist_ok=True)
binary = out / 'cache-epoch'
command = [os.environ.get('CC', 'cc'), '-std=gnu11', '-O1', '-g', '-pthread',
           '-Wall', '-Wextra', '-Wno-unused-function', '-Wno-unused-parameter',
           '-ffunction-sections', '-fdata-sections', '-DNV_TIZEN', '-DNV_GLES_NO_MAC',
           '-I' + str(source / 'src'), '-I' + str(args.sdk.resolve()),
           '-DSOURCE_FONTECACHE="' + str(source / 'src/fontecache.c') + '"']
if args.sanitize == '1':
    command += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
    if platform.system() == 'Linux':
        command += ['-fno-pie', '-no-pie']
command += [str(bundle / 'tests/vod-epoch.c'), str(bundle / 'tests/epoch.c')]
command += [str(source / 'src' / name) for name in ['addons.c', 'fontefalha.c', 'addonstats.c', 'js.c']]
command += ['-Wl,-dead_strip' if platform.system() == 'Darwin' else '-Wl,--gc-sections',
            '-lm', '-o', str(binary)]
paths = [source / 'src' / name for name in
         ['fontecache.c', 'fontecache.h', 'perfisessao.h', 'addons.c', 'addons.h',
          'fontefalha.c', 'fontefalha.h', 'addonstats.c', 'js.c', 'streams.h', 'perfis.h']]
paths += [bundle / 'tests' / name for name in ['vod-epoch.c', 'epoch.c', 'current-vod-fixture.inc']]
hashes = lambda: {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}
before = hashes()
build = subprocess.run(command, capture_output=True, text=True)
(out / 'build.log').write_text(build.stdout + build.stderr)
proof = {'root': str(source), 'host': platform.system(), 'sanitize': args.sanitize,
         'build_command': command, 'build_exit': build.returncode, 'source_sha256': before,
         'original_fixture_assertions_retained': True,
         'typed_fixture_epoch_producers': True,
         'scenarios': ['A->B->A old lookup/store rejected', 'new actor miss then current store/hit',
                       'stale store cannot replace fresh result', 'epoch changes inside store preparation'],
         'limitations': ['CPU declaration-only SDL/GLES boundaries; no whole application or GPU test.']}
if build.returncode == 0:
    environment = os.environ.copy()
    if args.sanitize == '1':
        environment.setdefault('ASAN_OPTIONS', 'detect_leaks=0:halt_on_error=1')
        environment.setdefault('UBSAN_OPTIONS', 'halt_on_error=1:print_stacktrace=1')
    run = subprocess.run([str(binary)], capture_output=True, text=True, env=environment, timeout=30)
    (out / 'run.log').write_text(run.stdout + run.stderr)
    proof.update(run_exit=run.returncode, run_command=[str(binary)])
    print(run.stdout, end='')
    if run.returncode:
        print(run.stderr)
proof['source_inputs_unchanged'] = hashes() == before
(out / 'evidence.json').write_text(json.dumps(proof, indent=2) + '\n')
assert proof['source_inputs_unchanged'], 'Fixture or actual source changed during check'
assert proof.get('run_exit') == 0, 'Cache epoch check failed; see build/run logs'
