#!/usr/bin/env python3
"""Run one independent reviewed group, then prove source/test bytes unchanged."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
from verify_source import plan, verify, REVIEW

p = argparse.ArgumentParser()
p.add_argument('--case', required=True)
p.add_argument('--group', required=True)
p.add_argument('--output', type=Path, required=True)
a = p.parse_args()
root = Path.cwd().resolve()
value, case = plan(a.case)
proof = verify(root, value, case)
out = a.output.resolve()
out.mkdir(parents=True, exist_ok=True)
fixture = REVIEW / 'fixtures' / a.case
if a.case == 'pr431':
    for path in (fixture / 'tests').iterdir():
        target = root / 'tests' / path.name
        assert not target.exists(), target
        shutil.copyfile(path, target)
    command = ['/bin/bash', str(REVIEW / 'build/pr431_behavior.sh')]
    assert a.group == 'behavior'
elif a.case == 'pr490':
    command = ['/bin/bash', str(fixture / 'run_ci.sh')]
    assert a.group == 'watched'
else:
    assert a.case == 'pr493'
    runners = {'core': 'core/run.sh', 'data': 'data/run.sh', 'ui': 'ui/run.sh',
               'provider': 'provider/run.sh', 'native-failover': 'native-failover/run.sh',
               'live-tv': 'live-tv/run.sh', 'trackers': 'trackers/run.sh',
               'playback': 'playback/run_playback.sh', 'trackers-existing': 'trackers/existing.sh',
               'auth-owned': 'trackers/auth-owned.sh', 'auth-existing': 'trackers/auth-existing.sh'}
    if a.group == 'http':
        command = ['/bin/bash', '-e', '-c', 'for name in jellyfin plex emby; do /bin/bash "tests/$name.sh"; done']
    else:
        command = ['/bin/bash', str(fixture / 'tests' / runners[a.group]), str(root)]
http_companions = None
http_ccbin = None
http_real_cc = None
if a.case == 'pr493' and a.group == 'http':
    http_companions = out / 'http-companions'
    helper = REVIEW / 'fixtures/pr493/legacy'
    subprocess.run([sys.executable, str(helper / 'make_legacy_companions.py'), str(root), str(http_companions)], check=True)
    http_ccbin = out / 'cc-bin'
    http_ccbin.mkdir(exist_ok=True)
    http_real_cc = shutil.which('cc')
    assert http_real_cc
    shutil.copyfile(helper / 'legacy-cc.py', http_ccbin / 'cc')
    (http_ccbin / 'cc').chmod(0o755)
runs = []
for mode in ('normal', 'asan'):
    folder = out / mode
    folder.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ, SANITIZE='1' if mode == 'asan' else '0', RUN_ASAN='1' if mode == 'asan' else '0',
        NV_SANITIZERS='1' if mode == 'asan' else '0', ASAN_OPTIONS='detect_leaks=0:halt_on_error=1', UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1',
        NUVIO_WATCHED_REVIEW=str(fixture), NUVIO_WATCHED_RESULTS=str(folder),
        NUVIO_PROFILE_REVIEW=str(fixture), NUVIO_PROFILE_RESULTS=str(folder),
        NUVIO_PROFILE_NATIVE_FAILOVER='1', TMPDIR='/tmp')
    if http_companions is not None:
        env.update(PROFILE_LEGACY_COMPANIONS=str(http_companions), PROFILE_LEGACY_REAL_CC=http_real_cc,
                   PATH=str(http_ccbin) + os.pathsep + os.environ['PATH'])
    logfile = folder / 'run.log'
    with logfile.open('w') as log:
        result = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, env=env, timeout=1200)
    if result.returncode:
        print('\n'.join(logfile.read_text(errors='replace').splitlines()[-100:]), flush=True)
        raise SystemExit(a.case + '/' + a.group + '/' + mode + ': failed')
    verify(root, value, case)
    runs.append({'mode': mode, 'status': 'passed', 'command': command, 'exit_code': result.returncode,
                 'sanitizers': ['address', 'undefined'] if mode == 'asan' else []})
    print(a.case + '/' + a.group + '/' + mode + ': PASS', flush=True)
proof.update({'outcome': 'passed', 'group': a.group, 'gates': runs,
              'original_test_assertions_modified': False})
if http_companions is not None:
    proof['fixture_scope'] = 'original HTTP assertions with initialized secret buffer'
    proof['external_fixture_setup'] = json.loads((http_companions / 'manifest.json').read_text())
    assert all(item['assertions_unchanged'] for item in proof['external_fixture_setup']['files'])
(out / 'verified.json').write_text(json.dumps(proof, indent=2) + '\n')
