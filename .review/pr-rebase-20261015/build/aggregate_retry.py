#!/usr/bin/env python3
"""Require every reused and repaired exact-source gate; never count diagnostics."""
import hashlib
import json
import os
from pathlib import Path
from verify_source import plan

root = Path('all-proofs').resolve()
value, _ = plan('pr431')
assert all(v['result'] == 'success' for v in json.loads(os.environ['REQUIRED_JOB_RESULTS']).values())
reuse = json.loads((root / 'reuse-receipt.json').read_text())
assert reuse['outcome'] == 'passed' and reuse['source_candidates_changed'] is False
reused = {(r['case'], r['gate']): r for r in reuse['reused']}
replaced = {(r['case'], r['gate']) for r in reuse['replaced']}
assert len(reused) == 21 and len(replaced) == 7 and not set(reused) & replaced
reports = []
for case_id, gate in sorted(set(reused) | replaced):
    path = root / case_id / gate / 'verified.json'
    data = json.loads(path.read_text())
    _, case = plan(case_id)
    assert data['outcome'] == 'passed' and data['source_verified'] is True
    assert data['source_commit'] == case['head'] and data['source_tree'] == case['tree']
    assert data['original_test_sha256'] == case['original_tests']
    if (case_id, gate) in reused:
        prior = reused[(case_id, gate)]
        assert hashlib.sha256(path.read_bytes()).hexdigest() == prior['proof_sha256']
        assert data['review_commit'] == prior['origin_review_commit']
    else:
        assert data['review_commit'] == os.environ['GITHUB_SHA']
    reports.append({'case': case_id, 'gate': gate, 'source_commit': case['head'], 'source_tree': case['tree'],
        'origin_review_commit': data['review_commit'], 'reused': (case_id,gate) in reused,
        'proof': str(path.relative_to(root)), 'proof_sha256': hashlib.sha256(path.read_bytes()).hexdigest()})
assert len(reports) == 28
receipt = {'outcome': 'passed', 'review_commit': os.environ['GITHUB_SHA'], 'base': value['base'],
    'cases': [{'pr': c['pr'], 'id': c['id'], 'source_commit': c['head'], 'source_tree': c['tree']} for c in value['cases']],
    'required_gates': reports, 'required_gate_count': len(reports), 'reused_required_gates': 21,
    'repaired_required_gates': 7, 'product_tests_or_workflows_added': False,
    'diagnostic_jobs_counted_as_test_passes': False,
    'ci_speed_changes': ['Repair three independent reduced-fixture groups in parallel',
        'Reuse 21 successful exact-source gates with original run/job/hash provenance',
        'Bounded concurrent wait for remaining original gates', 'Upload proofs and logs without intermediate binaries']}
(root / 'all-required-gates-verified.json').write_text(json.dumps(receipt, indent=2) + '\n')
print('All28 required gates passed:',21,'unchanged-source passes reused and7 repaired gates rerun')
