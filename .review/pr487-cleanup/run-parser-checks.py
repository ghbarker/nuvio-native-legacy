#!/usr/bin/env python3
"""Compile actual parser sources with review fixtures outside the PR tree."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import shlex
import shutil
import subprocess
import tempfile

ap = argparse.ArgumentParser()
ap.add_argument('--root', '--repo', dest='repo', type=Path, default=Path.cwd())
ap.add_argument('--sdk', type=Path, help='Optional local SDL2 header declaration directory')
ap.add_argument('--source', type=Path)
ap.add_argument('--fixture', type=Path, default=Path(__file__).with_name('stream_faixas_parse.c'))
ap.add_argument('--log-prefix', default='parser')
ap.add_argument('--output-dir', type=Path, default=Path(__file__).resolve().parent)
ap.add_argument('--sanitizer', action='store_true')
args = ap.parse_args()
logs = args.output_dir.resolve()
logs.mkdir(parents=True, exist_ok=True)
repo = args.repo.resolve()
parser = (args.source or repo / 'src/stream_parse.c').resolve()
fixture = args.fixture.resolve()
inputs = [fixture, parser, repo / 'src/js.c', repo / 'src/badges.c', repo / 'src/streams.h']
flags = ['-std=gnu11', '-O1', '-g', '-Isrc', '-Wall', '-Wextra', '-ffunction-sections', '-fdata-sections']
if shutil.which('pkg-config'):
    sdk = subprocess.run(['pkg-config', '--cflags', 'sdl2'], stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
    if sdk.returncode == 0:
        flags += shlex.split(sdk.stdout.decode())
if args.sdk:
    flags += ['-I' + str(args.sdk.resolve())]
flags += shlex.split(os.environ.get('CFLAGS', ''))
if platform.system() == 'Darwin':
    flags += ['-I/opt/homebrew/include', '-I/opt/homebrew/include/SDL2']
    linker = ['-Wl,-dead_strip']
else:
    linker = ['-Wl,--gc-sections']
sanitizer = args.sanitizer or os.environ.get('SANITIZE') == '1'
if sanitizer:
    flags += ['-fsanitize=address,undefined,float-cast-overflow', '-fno-omit-frame-pointer', '-fno-sanitize-recover=all']
env = dict(os.environ)
env['ASAN_OPTIONS'] = env.get('ASAN_OPTIONS', 'detect_leaks=0')
with tempfile.TemporaryDirectory(prefix='pr487-parser-') as build:
    binary = str(Path(build) / 'test')
    command = ['cc', *flags, str(fixture), str(parser), 'src/js.c', 'src/badges.c', *linker, '-lm', '-o', binary]
    built = subprocess.run(command, cwd=repo, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, code = built.stdout, built.returncode
    if not code:
        ran = subprocess.run([binary], cwd=repo, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        output += ran.stdout
        code = ran.returncode
    (logs / (args.log_prefix + '.log')).write_bytes(output)
    evidence = {
        'source_hashes_sha256': {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs},
        'compile_command': command,
        'sanitizer': sanitizer,
        'asan_options': env['ASAN_OPTIONS'],
        'exit_code': code,
        'output': output.decode(errors='replace'),
        'sdk_support': 'SDL2 declarations only; actual parser, js and badge code',
    }
    (logs / (args.log_prefix + '.json')).write_text(json.dumps(evidence, indent=2) + '\n')
    print(output.decode(errors='replace'), end='')
    print(f'{args.log_prefix}: exit {code}')
raise SystemExit(1 if code else 0)
