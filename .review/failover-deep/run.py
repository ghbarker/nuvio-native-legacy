#!/usr/bin/env python3
"""Run only the source-bound failover C regression, retaining small evidence."""
import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def git(source, *args):
    return subprocess.check_output(['git', '-C', str(source), *args], text=True).strip()


def clean(source):
    status = git(source, 'status', '--porcelain=v1', '--untracked-files=all')
    if status:
        raise AssertionError('source checkout became dirty')
    return {'clean': True, 'status': status}


# The public runner invokes CC exactly once. This wrapper records the actual
# compiler invocation and finished ELF before the public runner executes it.
# Its address-space limit applies only to compilation; the test is uncapped.
COMPILER = r'''#!/usr/bin/env python3
import datetime, hashlib, json, os
from pathlib import Path
import resource, subprocess, sys
args = sys.argv[1:]
out = Path(args[args.index('-o') + 1])
inputs = {}
for arg in args:
    if arg.endswith('.c') and Path(arg).is_file():
        inputs[arg] = hashlib.sha256(Path(arg).read_bytes()).hexdigest()
    if arg.startswith('-I'):
        directory = Path(arg[2:])
        if directory.name.startswith('nuvio-failover-prefetch-'):
            for inc in sorted(directory.glob('*.inc')):
                inputs[str(inc)] = hashlib.sha256(inc.read_bytes()).hexdigest()
record = {'compiler': os.environ['NV_REAL_CC'], 'argv': args,
          'sanitize': os.environ.get('SANITIZE'), 'inputs': inputs,
          'started': datetime.datetime.now(datetime.timezone.utc).isoformat()}
resource.setrlimit(resource.RLIMIT_AS, (384 * 1024 * 1024, 384 * 1024 * 1024))
result = subprocess.run([os.environ['NV_REAL_CC'], *args])
record['returncode'] = result.returncode
if result.returncode == 0:
    record['output_sha256'] = hashlib.sha256(out.read_bytes()).hexdigest()
    record['output_bytes'] = out.stat().st_size
    record['elf'] = out.read_bytes()[:4] == b'\x7fELF'
    symbols = subprocess.check_output(['nm', '-u', str(out)], text=True)
    record['asan'] = '__asan_init' in symbols
    record['ubsan'] = '__ubsan_handle_' in symbols
with open(os.environ['NV_CC_RECEIPT'], 'a') as stream:
    stream.write(json.dumps(record, sort_keys=True) + '\n')
sys.exit(result.returncode)
'''


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    source = args.source.resolve()
    output = args.output.resolve()
    if output == source or source in output.parents:
        raise AssertionError('proof output must be outside source')
    output.mkdir(parents=True, exist_ok=False)
    plan_path = Path(__file__).with_name('plan.json')
    plan = json.loads(plan_path.read_text())
    if plan['launch_enabled'] is not True:
        raise AssertionError('review launch is closed')
    source_head = plan['source_commit']
    head = git(source, 'rev-parse', 'HEAD')
    if git(source, 'rev-parse', 'HEAD^') != source_head:
        raise AssertionError('review commit is not a direct child of tested source')
    if git(source, 'rev-parse', source_head + '^{tree}') != plan['source_tree']:
        raise AssertionError('tested source tree changed')
    # The depth-two checkout marks source_head shallow, but its raw commit
    # header still proves the already-reviewed test-only parent identity.
    source_header = git(source, 'cat-file', '-p', source_head).split('\n\n', 1)[0]
    parents = [line[7:] for line in source_header.splitlines() if line.startswith('parent ')]
    if parents != [plan['source_parent']]:
        raise AssertionError('tested source parent changed')
    changed = git(source, 'diff', '--name-only', source_head, 'HEAD').splitlines()
    if sorted(changed) != sorted(plan['review_paths']):
        raise AssertionError('review commit changes source/test or unexpected paths')
    inputs = {p: sha(source / p) for p in plan['inputs']}
    if inputs != plan['inputs']:
        raise AssertionError('reviewed source/test input hash mismatch')
    recipe = {p: sha(source / p) for p in plan['review_paths']}
    receipt = {'verified': False, 'source_commit': source_head,
               'source_tree': plan['source_tree'], 'source_parent': plan['source_parent'],
               'product_commit': plan['product_commit'], 'review_commit': head,
               'plan_sha256': sha(plan_path), 'inputs': inputs, 'recipe': recipe,
               'workflow_run_id': os.environ.get('GITHUB_RUN_ID'),
               'workflow_run_attempt': os.environ.get('GITHUB_RUN_ATTEMPT'),
               'before': clean(source), 'groups': []}
    compiler = shutil.which('cc')
    if not compiler or not shutil.which('nm'):
        raise AssertionError('host C compiler or nm unavailable')
    receipt['compiler_version'] = subprocess.check_output([compiler, '--version'], text=True)
    wrapper = output / 'compiler.py'
    wrapper.write_text(COMPILER)
    wrapper.chmod(0o700)
    receipt['compiler_wrapper_sha256'] = sha(wrapper)
    try:
        for mode, sanitize in [('normal', '0'), ('asan', '1')]:
            log = output / (mode + '.log')
            cc_record = output / (mode + '-compiler.jsonl')
            env = os.environ.copy()
            env.update(CC=str(wrapper), NV_REAL_CC=compiler, NV_CC_RECEIPT=str(cc_record),
                       SANITIZE=sanitize, CFLAGS='-fno-pie -no-pie',
                       ASAN_OPTIONS='detect_leaks=1:abort_on_error=1',
                       UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1',
                       PYTHONDONTWRITEBYTECODE='1')
            command = plan['commands'][len(receipt['groups'])]
            with log.open('w') as stream:
                completed = subprocess.run(command, cwd=source, env=env,
                                           stdout=stream, stderr=subprocess.STDOUT,
                                           timeout=120)
            group = {'mode': mode, 'command': command, 'sanitize': sanitize,
                     'returncode': completed.returncode, 'log': log.name,
                     'log_sha256': sha(log), 'compiler_receipt': cc_record.name}
            receipt['groups'].append(group)
            if completed.returncode:
                raise AssertionError(mode + ' regression failed; inspect its raw log')
            text = log.read_text()
            markers = {marker: text.count(marker) for marker in plan['expected_markers']}
            group['markers'] = markers
            if set(markers.values()) != {1}:
                raise AssertionError(mode + ' did not execute each expected test group exactly once')
            records = [json.loads(line) for line in cc_record.read_text().splitlines()]
            if len(records) != 1:
                raise AssertionError(mode + ' expected exactly one actual C compilation')
            compiled = records[0]
            if compiled['returncode'] or not compiled['elf'] or compiled['output_bytes'] <= 0:
                raise AssertionError(mode + ' did not produce a usable ELF')
            flags = compiled['argv']
            if sanitize == '1':
                if '-fsanitize=address,undefined' not in flags or not compiled['asan'] or not compiled['ubsan']:
                    raise AssertionError('sanitizer invocation or linked runtime missing')
            elif any(flag.startswith('-fsanitize=') for flag in flags) or compiled['asan'] or compiled['ubsan']:
                raise AssertionError('normal mode unexpectedly used sanitizer')
            group['compiler_receipt_sha256'] = sha(cc_record)
            group['after'] = clean(source)
        if {p: sha(source / p) for p in plan['inputs']} != inputs:
            raise AssertionError('test run changed bound source inputs')
        receipt['after'] = clean(source)
        receipt['verified'] = True
    finally:
        receipt['finished'] = datetime.datetime.now(datetime.timezone.utc).isoformat()
        (output / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
        # Keep evidence small: generated program binaries already belong to the
        # public runner's temporary directory, which it removes itself.
        wrapper.unlink(missing_ok=True)
    print('PASS: source-bound failover regression normal + ASan/UBSan (four groups each)')


if __name__ == '__main__':
    main()
