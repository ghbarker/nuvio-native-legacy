#!/usr/bin/env python3
"""Reuse only required proofs from successful original jobs on unchanged source."""
import hashlib
import json
from pathlib import Path
import shutil
import sys
from verify_source import plan
old = Path(sys.argv[1]).resolve()
out = Path(sys.argv[2]).resolve()
value, _ = plan('pr431')
origin = 'bb8d91167afa69b78a79722e12f40633a81df1c8'
replaced = {('pr493', g) for g in ('trackers', 'trackers-existing', 'auth-owned', 'auth-existing', 'http', 'originals-providers', 'originals-profile-state')}
required = []
for case in value['cases']:
    required.extend([(case['id'], 'apps'), (case['id'], 'android')])
required += [('pr431', 'behavior'), ('pr490', 'watched')]
required += [('pr493', g) for g in value['profile_groups']]
required += [('pr493', 'original-asan-' + g) for g in ('fontepref', 'fontepref_file', 'player_retido')]
required += [('pr493', 'originals-' + g) for g in value['legacy_groups']]
assert len(required) == 28 and replaced <= set(required)
jobs = json.loads(Path('build/review/prior-jobs.json').read_text())['jobs']
by_name = {j['name']: j for j in jobs}
def job_for(case, gate):
    if gate == 'apps': return case + ' full TV and Mobile builds'
    if gate == 'android': return case + ' standard Android ARM32/ARM64'
    if gate.startswith('original-asan-'): return 'PR493 original full-source ' + gate.removeprefix('original-asan-') + ' ASan'
    if gate.startswith('originals-'): return 'PR493 original regressions ' + gate.removeprefix('originals-')
    if case == 'pr431': return 'pr431 behavior normal and ASan'
    if case == 'pr490': return 'pr490 watched normal and ASan'
    if gate in ('core', 'data', 'ui', 'provider'): return 'pr493 profile-core-ui normal and ASan'
    if gate in ('native-failover', 'playback', 'live-tv'): return 'pr493 profile-playback-tv normal and ASan'
    raise AssertionError((case, gate))
reused = []
for case_id, gate in required:
    if (case_id, gate) in replaced:
        continue
    name = job_for(case_id, gate)
    job = by_name[name]
    assert job['status'] == 'completed' and job['conclusion'] == 'success', (name, job['conclusion'])
    path = old / case_id / gate / 'verified.json'
    assert path.is_file(), ('Required passed proof missing', str(path))
    data = json.loads(path.read_text())
    _, case = plan(case_id)
    assert data['outcome'] == 'passed' and data['source_verified'] is True
    assert data['source_commit'] == case['head'] and data['source_tree'] == case['tree']
    assert data['original_test_sha256'] == case['original_tests'] and data['review_commit'] == origin
    target = out / case_id / gate / 'verified.json'
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(path, target)
    reused.append({'case': case_id, 'gate': gate, 'origin_run': 38667419517, 'origin_review_commit': origin,
        'job_id': job['id'], 'job_name': name, 'proof_sha256': hashlib.sha256(path.read_bytes()).hexdigest()})
assert len(reused) == 21
out.mkdir(parents=True, exist_ok=True)
(out / 'reuse-receipt.json').write_text(json.dumps({'outcome': 'passed', 'source_candidates_changed': False,
    'reused': reused, 'replaced': [{'case': c, 'gate': g} for c,g in sorted(replaced)],
    'basis': 'Original complete jobs passed; exact product tree and all original test bytes match; seven repaired reduced-fixture gates rerun'}, indent=2) + '\n')
print('Verified reusable original required gates:', len(reused))
