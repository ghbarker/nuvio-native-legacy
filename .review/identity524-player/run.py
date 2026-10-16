#!/usr/bin/env python3
"""Record the unchanged full-source #524 player causality baseline.

A green diagnostic means the specific functional baseline RED was reproduced;
it never means the product is fixed. No SDK, network, or source changes.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import resource
import shlex
import subprocess
import sys
import time
import traceback


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def record_json(path, value):
    Path(path).write_text(json.dumps(value, indent=2) + '\n')


def compiler(argv):
    root = Path(os.environ['IDENTITY524_PROOF'])
    phase = len(list(root.glob('compiler-*.json'))) + 1
    assert phase in (1, 2, 3)
    assert ('-c' in argv) == (phase < 3)
    target = Path(argv[argv.index('-o') + 1]).resolve()
    direct = {str(Path(arg).resolve()): digest(arg) for arg in argv
              if arg.endswith(('.c', '.o')) and Path(arg).is_file()}
    stdout, stderr = root / f'compiler-{phase}.stdout', root / f'compiler-{phase}.stderr'
    start = time.time_ns()
    with stdout.open('wb') as out, stderr.open('wb') as err:
        got = subprocess.run(['/usr/bin/clang', *argv], stdout=out, stderr=err)
    facts = {'phase': phase, 'command': ['/usr/bin/clang', *argv],
             'cwd': os.getcwd(), 'started_ns': start, 'finished_ns': time.time_ns(),
             'returncode': got.returncode, 'direct_inputs': direct,
             'stdout_sha256': digest(stdout), 'stderr_sha256': digest(stderr)}
    if got.returncode == 0:
        facts.update({'output_sha256': digest(target), 'output_bytes': target.stat().st_size,
                      'elf': target.read_bytes()[:4] == b'\x7fELF'})
        if phase == 3:
            real = target.with_name(target.name + '.actual')
            target.rename(real)
            # Observe each real process status; preserve every script assertion.
            target.write_text('#!/bin/bash\nexec ' + shlex.quote(sys.executable) + ' ' +
                shlex.quote(str(Path(__file__).resolve())) + ' --record-run ' +
                shlex.quote(str(real)) + ' "$@"\n')
            target.chmod(0o755)
    record_json(root / f'compiler-{phase}.json', facts)
    sys.stdout.buffer.write(stdout.read_bytes()); sys.stderr.buffer.write(stderr.read_bytes())
    return got.returncode


def runtime(binary, argv):
    root = Path(os.environ['IDENTITY524_PROOF'])
    assert len(argv) == 1 and argv[0] in ('known-series-control', 'movie-control', 'late-series')
    case = argv[0]
    output = root / (case + '.log')
    limits = {name: list(resource.getrlimit(value)) for name, value in
              [('address_space', resource.RLIMIT_AS), ('core', resource.RLIMIT_CORE)]}
    assert resource.getrlimit(resource.RLIMIT_AS)[0] == resource.RLIM_INFINITY
    started = time.time_ns()
    got = subprocess.run([binary, *argv], stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                         timeout=60)
    output.write_bytes(got.stdout)
    record_json(root / (case + '.json'), {'command': [binary, *argv],
        'binary_sha256': digest(binary), 'returncode': got.returncode,
        'started_ns': started, 'finished_ns': time.time_ns(), 'runtime_limits': limits,
        'log_sha256': digest(output), 'data_directory': os.environ.get('NUVIO_DADOS')})
    sys.stdout.buffer.write(got.stdout)
    return got.returncode


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--source', type=Path, required=True)
    ap.add_argument('--out', type=Path, required=True)
    args = ap.parse_args()
    source, output = args.source.resolve(), args.out.resolve()
    output.mkdir(parents=True, exist_ok=True)
    plan_bytes = Path(__file__).with_name('plan.json').read_bytes()
    plan = json.loads(plan_bytes)
    assert plan['launch_enabled'] is True and plan['root_reviewed'] is True
    git = lambda *xs: subprocess.check_output(['git', '-C', str(source), *xs])
    review = git('rev-parse', 'HEAD').decode().strip()
    assert git('rev-parse', 'HEAD^').decode().strip() == plan['source_commit']
    assert git('rev-parse', plan['source_commit'] + '^{tree}').decode().strip() == plan['source_tree']
    assert set(git('diff-tree', '--no-commit-id', '--name-only', '-r', 'HEAD').decode().splitlines()) == set(plan['recipe_paths'])
    inputs = {path: hashlib.sha256(git('show', plan['source_commit'] + ':' + path)).hexdigest()
              for path in plan['source_inputs']}
    assert inputs == plan['source_inputs']
    for path, expected in inputs.items(): assert digest(source / path) == expected
    before = git('status', '--porcelain', '--untracked-files=all').decode()
    assert not before
    receipt = {'source_commit': plan['source_commit'], 'source_tree': plan['source_tree'],
        'review_commit': review, 'workflow_run_id': os.environ['GITHUB_RUN_ID'],
        'workflow_run_attempt': os.environ['GITHUB_RUN_ATTEMPT'],
        'plan_sha256': hashlib.sha256(plan_bytes).hexdigest(),
        'inputs': inputs, 'groups': [], 'baseline_reproduced': False, 'product_fixed': False}
    record_json(output / 'receipt.json', receipt)
    for mode in ('normal', 'asan'):
        proof = output / mode; proof.mkdir()
        env = dict(os.environ, IDENTITY524_PROOF=str(proof), SANITIZE='1' if mode == 'asan' else '0',
                   SDL_VIDEODRIVER='dummy', ASAN_OPTIONS='detect_leaks=0:halt_on_error=1',
                   UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
        bindir = proof / 'bin'; bindir.mkdir()
        cc = bindir / 'cc'
        cc.write_text('#!/bin/bash\nexec ' + shlex.quote(sys.executable) + ' ' +
                      shlex.quote(str(Path(__file__).resolve())) + ' --record-cc "$@"\n')
        cc.chmod(0o755); env['PATH'] = str(bindir) + ':' + env['PATH']
        got = subprocess.run(['bash', 'tests/player_identity524.sh'], cwd=source, env=env,
                             stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=360)
        log = proof / 'script.log'; log.write_bytes(got.stdout)
        group = {'mode': mode, 'script_returncode': got.returncode, 'log_sha256': digest(log),
                 'expected_functional_red': False}
        receipt['groups'].append(group); record_json(output / 'receipt.json', receipt)
        try:
            text = got.stdout.decode(errors='replace')
            assert got.returncode == 1, 'Baseline must exit exactly1 for functional failure'
            assert not re.search(r'AddressSanitizer|UndefinedBehaviorSanitizer|runtime error:|LeakSanitizer|Assertion .* failed|Segmentation fault|Aborted', text)
            assert text.count('PASS known-series-control\n') == 1
            assert text.count('PASS movie-control\n') == 1 and text.count('FAIL late-series\n') == 1
            records = [json.loads((proof / f'compiler-{n}.json').read_text()) for n in (1, 2, 3)]
            assert len(list(proof.glob('compiler-*.json'))) == 3
            for n, record in enumerate(records, 1):
                assert record['returncode'] == 0 and record['elf']
                assert ('-fsanitize=address,undefined' in record['command']) == (mode == 'asan')
                assert ('-c' in record['command']) == (n < 3)
                for path, expected in record['direct_inputs'].items():
                    original = Path(path)
                    if original.suffix == '.c':
                        assert expected == inputs[str(original.relative_to(source))]
            assert set(records[0]['direct_inputs']) == {str(source / 'src/video.c')}
            assert set(records[1]['direct_inputs']) == {str(source / 'src/player.c')}
            expected_link_sources = {str(p) for p in source.glob('src/*.c') if p.name not in ('main.c', 'player.c', 'video.c')}
            expected_link_sources |= {str(p) for p in source.glob('src/dts/*.c')}
            expected_link_sources.add(str(source / 'tests/player_identity524.c'))
            assert {p for p in records[2]['direct_inputs'] if p.endswith('.c')} == expected_link_sources
            objects = {Path(p).name: value for p, value in records[2]['direct_inputs'].items() if p.endswith('.o')}
            assert objects == {'video.o': records[0]['output_sha256'], 'player.o': records[1]['output_sha256']}
            for case in ('known-series-control', 'movie-control', 'late-series'):
                fact = json.loads((proof / (case + '.json')).read_text())
                assert fact['returncode'] == (1 if case == 'late-series' else 0)
                assert fact['binary_sha256'] == records[-1]['output_sha256']
                expected_id = 'tt1629757' if case == 'movie-control' else 'tt9054364'
                case_text = (proof / (case + '.log')).read_text()
                assert case_text.count(f'boundaries parental=1 id={expected_id} refresh=1\n') == 1
            late = (proof / 'late-series.log').read_text()
            assert 'player episode 0:0; catalog type=series' in late
            assert 'series bare-movie progress exists=1 type=movie episode=0:0' in late
            payloads = [json.loads(line[len('stats payload '):]) for line in late.splitlines() if line.startswith('stats payload ')]
            assert len(payloads) == 1 and payloads[0]['movie']['ids']['imdb'] == 'tt9054364'
            assert 'show' not in payloads[0] and 'episode' not in payloads[0]
            assert not git('status', '--porcelain', '--untracked-files=all').decode()
            group['expected_functional_red'] = True
        except Exception as exc:
            group['validation_error'] = {'type': type(exc).__name__, 'message': str(exc), 'traceback': traceback.format_exc()}
        record_json(output / 'receipt.json', receipt)
    receipt['baseline_reproduced'] = len(receipt['groups']) == 2 and all(group['expected_functional_red'] and not group.get('validation_error') for group in receipt['groups'])
    receipt['source_clean_before_after'] = not git('status', '--porcelain', '--untracked-files=all').decode()
    receipt['raw_files_sha256'] = {str(p.relative_to(output)): digest(p)
        for p in output.rglob('*') if p.is_file() and p.name != 'receipt.json'}
    record_json(output / 'receipt.json', receipt)
    assert receipt['baseline_reproduced'] and receipt['source_clean_before_after'], 'Baseline diagnostic failed; inspect both preserved mode receipts'
    print('BASELINE_REPRODUCED: controls pass; late series persists bare movie and emits movie stats; normal+ASan')


if __name__ == '__main__':
    if len(sys.argv) > 1 and sys.argv[1] == '--record-cc':
        raise SystemExit(compiler(sys.argv[2:]))
    if len(sys.argv) > 2 and sys.argv[1] == '--record-run':
        raise SystemExit(runtime(sys.argv[2], sys.argv[3:]))
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    main()
