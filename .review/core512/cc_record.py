#!/usr/bin/env python3
"""Bind actual source dependencies, compiler output, and every real execution."""
import hashlib
import json
import os
from pathlib import Path
import resource
import shlex
import subprocess
import sys
import time


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def record(path, value):
    Path(path).write_text(json.dumps(value, indent=2) + '\n')


def git(*args):
    return subprocess.check_output(['git', *args], text=True).strip()


def execute(binary, args):
    out = Path(os.environ['CORE512_CC_PROOF'])
    number = len(list(out.glob('runtime-*.json'))) + 1
    stem = 'runtime-%02d' % number
    limits = {name: list(resource.getrlimit(value)) for name, value in
              [('address_space', resource.RLIMIT_AS), ('core', resource.RLIMIT_CORE)]}
    assert limits['address_space'][0] == resource.RLIM_INFINITY
    started = time.time_ns()
    got = subprocess.run([binary, *args], stdout=subprocess.PIPE,
                         stderr=subprocess.STDOUT, timeout=90)
    log = out / (stem + '.log')
    log.write_bytes(got.stdout)
    record(out / (stem + '.json'), {
        'command': [binary, *args], 'arguments': args,
        'binary_sha256': sha(binary), 'returncode': got.returncode,
        'started_ns': started, 'finished_ns': time.time_ns(),
        'runtime_limits': limits, 'log_sha256': sha(log)})
    sys.stdout.buffer.write(got.stdout)
    return got.returncode


def compile_actual(args):
    root = Path(os.environ['CORE512_SOURCE']).resolve()
    out = Path(os.environ['CORE512_CC_PROOF'])
    out.mkdir(parents=True, exist_ok=True)
    assert not (out / 'receipt.json').exists(), 'only one actual compile expected'
    assert '-o' in args and not any(a in ('-c', '-E', '-S') for a in args)
    assert not any(a.startswith('-D') and 'NDEBUG' in a for a in args)
    compiler = os.environ['CORE512_REAL_CC']
    sdl = shlex.split(subprocess.check_output(['pkg-config', '--cflags', 'sdl2'], text=True))
    command = [compiler, *args, *sdl]
    index = args.index('-o')
    binary = Path(args[index + 1]).resolve()
    dependency_command = [compiler, *args[:index], *args[index + 2:], *sdl, '-MM']
    facts = {'original_arguments': args, 'command': command,
             'dependency_command': dependency_command, 'source_review': git('rev-parse', 'HEAD'),
             'local_candidate_provenance': os.environ['CORE512_LOCAL_CANDIDATE'],
             'real_compiler': str(Path(compiler).resolve()), 'real_compiler_sha256': sha(compiler),
             'compiler_version': subprocess.check_output([compiler, '--version'], text=True),
             'sdl_pkg_config': sdl}
    dependencies = subprocess.run(dependency_command, stdout=subprocess.PIPE,
                                  stderr=subprocess.STDOUT, text=True)
    (out / 'dependencies.log').write_text(dependencies.stdout)
    facts['dependency_exit_code'] = dependencies.returncode
    assert dependencies.returncode == 0, dependencies.stdout
    paths = set()
    for line in dependencies.stdout.replace('\\\n', ' ').splitlines():
        if ':' not in line:
            continue
        for token in shlex.split(line.split(':', 1)[1]):
            path = Path(token)
            path = (path if path.is_absolute() else root / path).resolve()
            if path.is_file() and path.is_relative_to(root):
                paths.add(path.relative_to(root).as_posix())
    assert any(name.endswith('.c') for name in paths), 'no actual project sources in compiler dependencies'
    facts['dependencies'] = {}
    for name in sorted(paths):
        blob = git('rev-parse', facts['source_review'] + ':' + name)
        assert git('hash-object', name) == blob, 'changed compiler dependency: ' + name
        facts['dependencies'][name] = {'git_blob': blob, 'sha256': sha(root / name)}
    facts['dependency_log_sha256'] = sha(out / 'dependencies.log')
    started = time.time_ns()
    got = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    (out / 'compile.log').write_text(got.stdout)
    facts.update(returncode=got.returncode, started_ns=started, finished_ns=time.time_ns(),
                 compile_log_sha256=sha(out / 'compile.log'))
    if got.returncode == 0:
        assert binary.is_file() and binary.read_bytes()[:4] == b'\x7fELF'
        facts.update(binary_sha256=sha(binary), binary_bytes=binary.stat().st_size, elf=True)
        actual = binary.with_name(binary.name + '.actual')
        binary.rename(actual)
        binary.write_text('#!/bin/sh\nexec ' + shlex.quote(sys.executable) + ' ' +
                          shlex.quote(str(Path(__file__).resolve())) + ' --run ' +
                          shlex.quote(str(actual)) + ' "$@"\n')
        binary.chmod(0o755)
    record(out / 'receipt.json', facts)
    sys.stdout.write(got.stdout)
    return got.returncode


if __name__ == '__main__':
    if len(sys.argv) > 2 and sys.argv[1] == '--run':
        raise SystemExit(execute(sys.argv[2], sys.argv[3:]))
    raise SystemExit(compile_actual(sys.argv[1:]))
