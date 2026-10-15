#!/usr/bin/env python3
"""Original full-source sanitizer script, unmodified; an actual pass is mandatory."""
import argparse
import json
import os
from pathlib import Path
import subprocess
from verify_source import plan, verify

p = argparse.ArgumentParser()
p.add_argument('--case', required=True)
p.add_argument('--suite', choices=['fontepref', 'fontepref_file', 'player_retido'], required=True)
p.add_argument('--output', type=Path, required=True)
a = p.parse_args()
root = Path.cwd().resolve()
value, case = plan(a.case)
proof = verify(root, value, case)
out = a.output.resolve()
out.mkdir(parents=True, exist_ok=True)
env = dict(os.environ, SANITIZE='1', ASAN_OPTIONS='detect_leaks=0:halt_on_error=1',
           UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
logfile = out / (a.suite + '.log')
with logfile.open('w') as log:
    result = subprocess.run(['/bin/bash', 'tests/' + a.suite + '.sh'], stdout=log,
        stderr=subprocess.STDOUT, env=env, timeout=1200)
if result.returncode:
    print('\n'.join(logfile.read_text(errors='replace').splitlines()[-100:]), flush=True)
    raise SystemExit(a.suite + ': original sanitizer script failed')
modules = sorted(str(p) for p in Path('src').glob('*.c')) + sorted(str(p) for p in Path('src/dts').glob('*.c'))
records = [json.loads(p.read_text()) for p in Path(os.environ['REVIEW_CC_OUTPUT']).glob('*-invocation.json')]
matches = [r for r in records if 'tests/' + a.suite + '.c' in r['arguments']]
assert len(matches) == 1, (a.suite, len(matches))
record = matches[0]
omitted = {'src/main.c', 'src/fontepref.c'} if a.suite != 'player_retido' else {'src/main.c', 'src/player.c', 'src/video.c'}
assert set(record['arguments']) & set(modules) == set(modules) - omitted
assert record['returncode'] == 0 and '-fsanitize=address,undefined' in record['arguments']
assert record['added_link_rpath'].startswith('-Wl,-rpath,')
assert record['effective_arguments'] == record['arguments'] + [record['added_link_rpath']]
verify(root, value, case)
proof.update({'outcome': 'passed', 'suite': a.suite, 'script_unchanged': True,
    'sanitizers': ['address', 'undefined'], 'full_source_files_compiled': len(set(modules) - omitted),
    'original_flags_unchanged': True, 'only_added_flag': record['added_link_rpath']})
(out / 'verified.json').write_text(json.dumps(proof, indent=2) + '\n')
print('Original full-source ASan/UBSan passed:', a.case, a.suite, flush=True)
