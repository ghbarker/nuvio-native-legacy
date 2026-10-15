#!/usr/bin/env python3
"""Run the unchanged maintainer #526 regressions with the real macOS SDK."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from common import source_binding

TEST_HASHES = {
    'tests/synopsis_review.c': '70ce6a18c3114fb962551ae4850e8787e43fedc87bf1f97288964155beec921c',
    'tests/synopsis_review.sh': '49dabb38b11172faa9cd50dd8ef6a154a34e1a2128d92b2a1abbeec0fe5dc9cd',
}


def sha(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b''):
            h.update(chunk)
    return h.hexdigest()


CC_WRAPPER = r'''import hashlib,json,os,pathlib,subprocess,sys
argv=sys.argv[1:]
compiler=json.loads(os.environ['NV526_COMPILER_JSON'])
sanitize=os.environ['NV526_SANITIZE']=='1'
flags=['-fsanitize=address,undefined','-fno-omit-frame-pointer'] if sanitize else []
link_flags=json.loads(os.environ['NV526_LINK_FLAGS_JSON'])
command=compiler+flags+link_flags+argv
record={'command':command,'sanitizers_injected':flags,'link_flags_injected':link_flags}
result=subprocess.run(command)
record['exit']=result.returncode
if result.returncode==0 and '-o' in argv:
    binary=pathlib.Path(argv[argv.index('-o')+1])
    h=hashlib.sha256()
    with binary.open('rb') as f:
        for chunk in iter(lambda:f.read(1024*1024),b''):h.update(chunk)
    record['binary_sha256']=h.hexdigest()
    record['binary_bytes']=binary.stat().st_size
    symbols=subprocess.run(['nm','-u',str(binary)],capture_output=True,text=True)
    record['nm_exit']=symbols.returncode
    record['asan_runtime_symbol']='asan_init' in symbols.stdout
    record['ubsan_runtime_symbol']='ubsan_handle' in symbols.stdout
with open(os.environ['NV526_COMPILER_LOG'],'a') as f:f.write(json.dumps(record)+'\n')
sys.exit(result.returncode)
'''


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--source', required=True)
    parser.add_argument('--proof', required=True)
    parser.add_argument('--output', required=True)
    parser.add_argument('--cc', default='cc')
    args = parser.parse_args()
    source, out = Path(args.source).resolve(), Path(args.output).resolve()
    assert not out.is_relative_to(source), 'proof output must be outside source'
    plan, proof, manifest = source_binding(source, Path(args.proof))
    assert int(proof['candidate_pr']) == 526
    assert all(sha(source / path) == expected for path, expected in TEST_HASHES.items())
    compiler = shlex.split(args.cc)
    assert compiler, 'missing real compiler'
    resolved = shutil.which(compiler[0])
    assert resolved, compiler[0]
    compiler[0] = resolved
    sdl3_prefix = Path(subprocess.check_output(['brew', '--prefix', 'sdl3'], text=True).strip())
    sdl3_lib = sdl3_prefix / 'lib'
    assert sdl3_lib.is_dir(), sdl3_lib
    link_flags = ['-Wl,-rpath,' + str(sdl3_lib)]
    out.mkdir(parents=True, exist_ok=True)
    results = []
    for sanitize in (False, True):
        label = 'asan-ubsan' if sanitize else 'normal'
        run = out / label
        run.mkdir(parents=True, exist_ok=True)
        wrappers = run / 'compiler-wrapper'
        wrappers.mkdir(exist_ok=True)
        wrapper = wrappers / 'cc'
        wrapper.write_text('#!' + sys.executable + '\n' + CC_WRAPPER)
        wrapper.chmod(0o700)
        invocations = run / 'compile-invocations.jsonl'
        assert not invocations.exists(), 'use a fresh proof output directory'
        env = dict(os.environ)
        env['PATH'] = str(wrappers) + os.pathsep + env.get('PATH', '')
        env['NV526_COMPILER_JSON'] = json.dumps(compiler)
        env['NV526_COMPILER_LOG'] = str(invocations)
        env['NV526_LINK_FLAGS_JSON'] = json.dumps(link_flags)
        env['NV526_SANITIZE'] = '1' if sanitize else '0'
        if sanitize:
            env['ASAN_OPTIONS'] = ':'.join(filter(None, [env.get('ASAN_OPTIONS', ''),
                                                     'detect_leaks=0' if sys.platform == 'darwin' else '',
                                                     'halt_on_error=1']))
            env['UBSAN_OPTIONS'] = ':'.join(filter(None, [env.get('UBSAN_OPTIONS', ''), 'halt_on_error=1:print_stacktrace=1']))
        log = run / 'synopsis-review.log'
        with log.open('w') as stream:
            proc = subprocess.run(['bash', 'tests/synopsis_review.sh'], cwd=source,
                                  env=env, stdout=stream, stderr=subprocess.STDOUT,
                                  timeout=1800)
        raw = log.read_text(errors='replace')
        calls = [json.loads(line) for line in invocations.read_text().splitlines()] if invocations.exists() else []
        record = {'name': label, 'command': ['bash', 'tests/synopsis_review.sh'],
                  'returncode': proc.returncode, 'log': str(log.relative_to(out)),
                  'log_sha256': sha(log), 'compile_invocations': str(invocations.relative_to(out)),
                  'compile_invocations_sha256': sha(invocations) if invocations.exists() else None,
                  'compile_calls': calls, 'case_passes': raw.splitlines().count('synopsis_review: PASS'),
                  'test_assertions_unchanged': True}
        results.append(record)
        (run / 'receipt.json').write_text(json.dumps(record, indent=2) + '\n')
        assert proc.returncode == 0, (label, proc.returncode, str(log))
        assert record['case_passes'] == 2, (label, 'both setting and sheet cases must pass')
        assert len(calls) == 1 and calls[0]['exit'] == 0 and calls[0].get('binary_sha256'), (label, calls)
        assert calls[0]['link_flags_injected'] == link_flags
        if sanitize:
            assert calls[0]['sanitizers_injected'] == ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
            assert calls[0]['nm_exit'] == 0 and calls[0]['asan_runtime_symbol'] and calls[0]['ubsan_runtime_symbol']
        else:
            assert calls[0]['sanitizers_injected'] == []
        assert all(sha(source / path) == expected for path, expected in TEST_HASHES.items())
        print('PASS #526 ' + label + ': unchanged setting + playback-sheet regressions', flush=True)
    source_binding(source, Path(args.proof))
    receipt = {'success': True, 'candidate_pr': 526, 'source_commit': proof['source_commit'],
               'tree': proof['tree'], 'base': proof['base'], 'manifest_sha256': proof['manifest_sha256'],
               'review_commit': proof['review_commit'], 'test_hashes': TEST_HASHES,
               'entry_sha256': sha(__file__), 'runs': results, 'sdl3_prefix': str(sdl3_prefix),
               'link_flags_injected': link_flags,
               'behavior': 'Maintainer Normal/Grande compatibility, preview migration, hero24/32 typography and playback episode reader',
               'scope': 'Real macOS SDK; no fake SDK; original test source and shell preserved'}
    (out / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')


if __name__ == '__main__':
    main()
