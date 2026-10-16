#!/usr/bin/env python3
"""Require the frozen #524 lookup product and inherited metadata suite all green."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import resource
import shlex
import shutil
import subprocess
import sys
import time
import traceback


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def git(*args):
    return subprocess.check_output(['git', *args], text=True).strip()


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def save(path, value):
    Path(path).write_text(json.dumps(value, indent=2) + '\n')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    root = Path.cwd().resolve()
    kit = Path(__file__).resolve().parent
    out = Path(args.output).resolve()
    out.mkdir(parents=True, exist_ok=True)
    plan = json.loads((kit / 'plan.json').read_text())
    require(plan['root_approved'] and plan['execution_enabled'], 'root launch gates are closed')
    require(sha(kit / 'plan.json') == os.environ['IDENTITY524_POSITIVE_PLAN_SHA256'], 'plan digest mismatch')
    head = git('rev-parse', 'HEAD')
    require(head == os.environ['GITHUB_SHA'], 'workflow source mismatch')
    require(os.environ['GITHUB_REF'] == 'refs/heads/' + plan['branch'], 'workflow branch mismatch')
    require(git('rev-parse', 'HEAD^') == plan['source_commit'], 'review parent mismatch')
    require(git('rev-parse', plan['source_commit'] + '^{tree}') == plan['source_tree'], 'source tree mismatch')
    require(git('rev-parse', plan['source_commit'] + '^') == plan['product_commit'], 'separate tests parent mismatch')
    require(git('rev-parse', plan['product_commit'] + '^') == plan['base'], 'production base mismatch')
    require(set(git('diff', '--name-only', plan['source_commit'], 'HEAD').splitlines()) == set(plan['review_paths']), 'unexpected review delta')
    require(set(git('diff', '--name-only', plan['base'], plan['product_commit']).splitlines()) == set(plan['product_paths']), 'unexpected production delta')
    require(set(git('diff', '--name-only', plan['product_commit'], plan['source_commit']).splitlines()) == set(plan['test_paths']), 'unexpected tests delta')
    require(not git('status', '--porcelain', '--untracked-files=all'), 'dirty source before execution')
    for name, expected in plan['recipe_sha256'].items():
        require(sha(kit / name) == expected, 'recipe digest mismatch: ' + name)
    for name, fact in plan['source_origins'].items():
        require(git('rev-parse', 'HEAD:' + name) == fact['blob'] and sha(root / name) == fact['sha256'], 'source provenance mismatch: ' + name)
    real_cc = shutil.which('clang')
    require(real_cc is not None, 'real clang missing')
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    require(resource.getrlimit(resource.RLIMIT_AS)[0] == resource.RLIM_INFINITY, 'unexpected address-space limit')
    bindir = out / 'toolchain'
    bindir.mkdir()
    cc = bindir / 'cc'
    cc.write_text('#!/bin/sh\nexec ' + shlex.quote(sys.executable) + ' ' + shlex.quote(str(kit / 'cc_record.py')) + ' "$@"\n')
    cc.chmod(0o755)
    groups, errors = [], []
    deadline = time.monotonic() + 900
    receipt = {'source_commit': plan['source_commit'], 'source_tree': plan['source_tree'],
               'product_commit': plan['product_commit'], 'base': plan['base'], 'review_commit': head,
               'workflow_run_id': os.environ['GITHUB_RUN_ID'], 'workflow_run_attempt': os.environ['GITHUB_RUN_ATTEMPT'],
               'branch': plan['branch'], 'plan_sha256': sha(kit / 'plan.json'),
               'all_checks_passed': False, 'full_player_validated': False,
               'scope': 'Typed on-demand title lookup and inherited metadata controls only',
               'groups': groups, 'errors': errors}
    save(out / 'verified.json', receipt)
    for sanitized in (False, True):
        mode = 'asan' if sanitized else 'normal'
        for suite in ('title_identity524', 'detalheanime'):
            label = suite + '-' + mode
            directory = out / label
            directory.mkdir()
            env = dict(os.environ, PATH=str(bindir) + os.pathsep + os.environ['PATH'], CC=str(cc),
                       IDENTITY524_SOURCE=str(root), IDENTITY524_SOURCE_COMMIT=plan['source_commit'],
                       IDENTITY524_CC_PROOF=str(directory / 'compiler'), IDENTITY524_REAL_CC=real_cc,
                       SANITIZE='1' if sanitized else '0', SDL_VIDEODRIVER='dummy',
                       ASAN_OPTIONS='detect_leaks=0:halt_on_error=1',
                       UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1', PYTHONDONTWRITEBYTECODE='1')
            command = ['bash', 'tests/' + suite + '.sh']
            group = {'label': label, 'command': command, 'sanitized': sanitized, 'verified': False}
            groups.append(group)
            started = time.monotonic()
            log = ''
            try:
                require(deadline > time.monotonic(), 'total validation budget exhausted')
                try:
                    got = subprocess.run(command, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                         text=True, timeout=min(240, deadline - time.monotonic()))
                    log = got.stdout
                    group['script_returncode'] = got.returncode
                except subprocess.TimeoutExpired as exc:
                    log = exc.stdout or ''
                    if isinstance(log, bytes):
                        log = log.decode(errors='replace')
                    group['timed_out'] = True
                    raise AssertionError('bounded public-script timeout')
                finally:
                    (directory / 'run.log').write_text(log)
                    group['log_sha256'] = sha(directory / 'run.log')
                require(got.returncode == 0, 'public script failed: ' + str(got.returncode))
                require(not re.search(r'ERROR: (?:AddressSanitizer|LeakSanitizer)|UndefinedBehaviorSanitizer|runtime error:|Assertion .* failed|Segmentation fault|Aborted', log), 'runtime assertion or sanitizer diagnostic')
                proof = directory / 'compiler'
                compiler = json.loads((proof / 'receipt.json').read_text())
                group['compiler_receipt_sha256'] = sha(proof / 'receipt.json')
                require(compiler['returncode'] == 0 and compiler['dependency_exit_code'] == 0 and compiler['elf'], 'real compile/link failed')
                require(compiler['source_review'] == head and compiler['source_commit'] == plan['source_commit'], 'compiler source mismatch')
                require(('-fsanitize=address,undefined' in compiler['command']) == sanitized, 'sanitizer mode mismatch')
                require(sha(proof / 'compile.log') == compiler['compile_log_sha256'] and sha(proof / 'dependencies.log') == compiler['dependency_log_sha256'], 'raw compiler digest mismatch')
                required_sources = {'src/descoberta.c', 'src/catalogo.c', 'tests/detalheanime.c', 'tests/addons_manifest_notifications_stub.inc'}
                if suite == 'title_identity524':
                    required_sources.add('tests/title_identity524.c')
                require(required_sources <= set(compiler['dependencies']), 'actual production/harness dependencies missing')
                for name, fact in compiler['dependencies'].items():
                    require(git('rev-parse', plan['source_commit'] + ':' + name) == fact['git_blob'] and sha(root / name) == fact['sha256'], 'actual dependency mismatch: ' + name)
                runs = sorted(proof.glob('runtime-*.json'))
                expected_arguments = [[case] for case in plan['scenarios']] if suite == 'title_identity524' else [[]]
                require(len(runs) == len(expected_arguments), 'incomplete actual binary execution count')
                for path, arguments in zip(runs, expected_arguments):
                    actual = json.loads(path.read_text())
                    require(actual['arguments'] == arguments and actual['returncode'] == 0, 'actual scenario failed: ' + repr(arguments))
                    require(actual['binary_sha256'] == compiler['binary_sha256'], 'executed binary digest mismatch')
                    require(sha(path.with_suffix('.log')) == actual['log_sha256'], 'actual runtime log digest mismatch')
                    text = path.with_suffix('.log').read_text()
                    require(not re.search(r'ERROR: (?:AddressSanitizer|LeakSanitizer)|UndefinedBehaviorSanitizer|runtime error:|Assertion .* failed|Segmentation fault|Aborted', text), 'actual runtime diagnostics')
                if suite == 'title_identity524':
                    require(not re.search(r'^FAIL ', log, re.M), 'functional failure marker')
                    require(re.findall(r'^PASS (.+)$', log, re.M) == plan['scenarios'], 'scenario pass markers mismatch')
                    for case in ('tmdb-tv', 'episode-id'):
                        position = plan['scenarios'].index(case)
                        text = runs[position].with_suffix('.log').read_text()
                        require(text.count('result id=tt9054364 type=series title=That Time I Got Reincarnated as a Slime') == 1, 'known series result mismatch: ' + case)
                        require('request https://v3-cinemeta.strem.io/meta/movie/tt9054364.json' not in text, 'wrong movie fallback: ' + case)
                else:
                    require(log.count('detalheanime: tudo ok\n') == 1, 'inherited metadata completion marker missing')
                require(not git('status', '--porcelain', '--untracked-files=all'), 'source changed during execution')
                group.update(verified=True, compiler_binary_sha256=compiler['binary_sha256'], actual_executions=len(runs))
                print(label + ': PASS (' + str(len(runs)) + ' actual executions)', flush=True)
            except Exception as exc:
                group['error'] = {'type': type(exc).__name__, 'message': str(exc), 'traceback': traceback.format_exc()}
                errors.append({'label': label, **group['error']})
                print(label + ': FAIL ' + str(exc) + '\n' + log[-1500:], flush=True)
            finally:
                group['seconds'] = time.monotonic() - started
                save(directory / 'group-verdict.json', group)
                save(out / 'verified.json', receipt)
    receipt['source_clean_before_after'] = not git('status', '--porcelain', '--untracked-files=all')
    receipt['all_groups_collected'] = len(groups) == 4
    receipt['all_checks_passed'] = receipt['all_groups_collected'] and receipt['source_clean_before_after'] and not errors and all(group['verified'] for group in groups)
    receipt['raw_files_sha256'] = {str(path.relative_to(out)): sha(path) for path in out.rglob('*')
                                   if path.is_file() and path.name != 'verified.json'}
    save(out / 'verified.json', receipt)
    require(receipt['all_checks_passed'], 'positive validation failed; inspect all preserved groups')
    print('PASS: all 17 typed lookup scenarios and inherited metadata suite pass normal plus ASan/UBSan', flush=True)


if __name__ == '__main__':
    main()
