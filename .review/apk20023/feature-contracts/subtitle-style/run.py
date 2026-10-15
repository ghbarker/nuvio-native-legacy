#!/usr/bin/env python3
"""Replay unchanged #475 Style assertions on caller-selected exact source."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import platform
import re
import subprocess

HERE = Path(__file__).resolve().parent


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def git(source, *args):
    return subprocess.check_output(['git', '-C', str(source), *args], text=True).strip()


def execute(command, log, env=None):
    result = subprocess.run(command, text=True, capture_output=True, env=env, timeout=90)
    log.write_text(result.stdout + result.stderr)
    return result


def run_one(source, output, cc, touch, sanitized, private):
    output.mkdir(parents=True)
    darwin = platform.system() == 'Darwin'
    flags = ['-O1', '-g', '-std=gnu11', '-I' + str(HERE / 'sdk'), '-I' + str(source / 'src'), '-include', str(HERE / 'sdk-extra.h'), '-DFAIXAS_SOURCE="' + str(source / 'src/faixas.c') + '"']
    if not darwin:
        flags += ['-ffunction-sections', '-fdata-sections']
    if touch:
        flags += ['-DNV_TOUCH_UI=1']
    if private:
        flags += ['-DNV_TEST_LOCAL_LOGS=1']
    san = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-fno-sanitize-recover=all'] if sanitized else []
    flags += san
    command = [cc, *flags, '-MMD', '-MF', str(output / 'fixture.d'), '-c', str(HERE / 'style-controls.c'), '-o', str(output / 'fixture.o')]
    compile_result = execute(command, output / 'compile.log')
    assert compile_result.returncode == 0, compile_result.stderr
    link = [cc, str(output / 'fixture.o'), '-Wl,-dead_strip' if darwin else '-Wl,--gc-sections', '-lm', '-o', str(output / 'test'), *san]
    probe = execute(link, output / 'initial-link.log')
    pattern = r'^\s*"([^"]+)", referenced from:\s*$' if darwin else r"undefined reference to [`']([^'`]+)['`]"
    symbols = sorted(set(re.findall(pattern, probe.stderr, re.M)))
    assert probe.returncode == 0 or symbols, probe.stderr
    assert all(re.fullmatch(r'[A-Za-z_][A-Za-z_0-9]*', symbol) for symbol in symbols), symbols
    traps = '#include <stdio.h>\n#include <stdlib.h>\n'
    for index, symbol in enumerate(symbols):
        traps += 'void trap' + str(index) + '(void) __asm__(' + json.dumps(symbol) + ');void trap' + str(index) + '(void){fputs(' + json.dumps('UNEXPECTED dependency: ' + symbol + '\n') + ',stderr);abort();}\n'
    (output / 'traps.c').write_text(traps)
    result = execute([cc, *flags, '-c', str(output / 'traps.c'), '-o', str(output / 'traps.o')], output / 'traps-compile.log')
    assert result.returncode == 0, result.stderr
    link.insert(1, str(output / 'traps.o'))
    result = execute(link, output / 'link.log')
    assert result.returncode == 0, result.stderr
    env = dict(os.environ, ASAN_OPTIONS='detect_leaks=0:halt_on_error=1', UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
    result = execute([str(output / 'test')], output / 'run.log', env)
    dependencies = []
    for item in (output / 'fixture.d').read_text().replace('\\\n', ' ').split(':', 1)[1].split():
        path = Path(item).resolve()
        if path.is_relative_to(source):
            dependencies.append({'path': str(path.relative_to(source)), 'sha256': sha(path)})
    assert dependencies and any(x['path'] == 'src/faixas.c' for x in dependencies)
    return {'passed': result.returncode == 0 and result.stdout.strip() == 'PASS actual style controls 274 checks' and 'UNEXPECTED dependency' not in result.stderr, 'returncode': result.returncode, 'platform': 'mobile' if touch else 'tv', 'sanitizers': ['address', 'undefined'] if sanitized else [], 'private': private, 'unreached_abort_boundaries': symbols, 'traps_fired': 'UNEXPECTED dependency' in result.stderr, 'stdout': result.stdout, 'stderr': result.stderr, 'compile_command': command, 'link_command': link, 'production_dependencies': sorted(dependencies, key=lambda x: x['path']), 'log_sha256': {p.name: sha(p) for p in output.glob('*.log')}}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--cc', required=True)
    parser.add_argument('--private', action='store_true')
    parser.add_argument('--sanitize', action='store_true')
    parser.add_argument('--platform', choices=['all', 'tv', 'mobile'], default='all')
    args = parser.parse_args()
    source, output = args.source.resolve(), args.output.resolve()
    assert not output.exists(), 'Use a fresh output directory'
    assert not git(source, 'status', '--porcelain'), 'Refuse uncommitted source changes'
    commit, tree = git(source, 'rev-parse', 'HEAD'), git(source, 'rev-parse', 'HEAD^{tree}')
    origin = json.loads((HERE / 'fixture-origin.json').read_text())
    assert sha(HERE / 'style-controls.c') == origin['assertions_copied_without_modification']['style-controls.c']
    output.mkdir(parents=True)
    groups = []
    for touch in (False, True):
        mode = 'mobile' if touch else 'tv'
        if args.platform != 'all' and args.platform != mode:
            continue
        groups.append(run_one(source, output / (mode + ('-asan' if args.sanitize else '-normal')), args.cc, touch, args.sanitize, args.private))
    assert git(source, 'rev-parse', 'HEAD') == commit and git(source, 'rev-parse', 'HEAD^{tree}') == tree and not git(source, 'status', '--porcelain')
    result = {'passed': all(x['passed'] for x in groups), 'source_commit': commit, 'source_tree': tree, 'source_clean_after': True, 'fixture_sha256': sha(HERE / 'style-controls.c'), 'fixture_origin_sha256': sha(HERE / 'fixture-origin.json'), 'runner_sha256': sha(__file__), 'private': args.private, 'sanitizers': ['address', 'undefined'] if args.sanitize else [], 'groups': groups, 'scope': origin['scope'], 'actual_platform': platform.system(), 'logs_sha256': {str(path.relative_to(output)): sha(path) for path in sorted(output.rglob('*.log'))}, 'compiler': args.cc, 'compiler_version': subprocess.check_output([args.cc, '--version'], text=True).splitlines()[0]}
    (output / 'receipt.json').write_text(json.dumps(result, indent=2) + '\n')
    print('PASS' if result['passed'] else 'FAIL', 'unchanged #475 Style assertions', len(groups) * 274)
    assert result['passed']


if __name__ == '__main__':
    main()
