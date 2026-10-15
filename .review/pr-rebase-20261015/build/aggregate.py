#!/usr/bin/env python3
"""A missing or failed required gate cannot be turned into a passing receipt."""
import json
import os
from pathlib import Path
import sys
from verify_source import plan

root = Path(sys.argv[1]).resolve()
value, _ = plan('pr431')
assert all(v['result'] == 'success' for v in json.loads(os.environ['REQUIRED_JOB_RESULTS']).values()), os.environ['REQUIRED_JOB_RESULTS']
required = []
for case in value['cases']:
    required.extend([(case['id'], 'apps'), (case['id'], 'android')])
required += [('pr431', 'behavior'), ('pr490', 'watched')]
required += [('pr493', group) for group in value['profile_groups']]
required += [('pr493', 'original-asan-' + suite) for suite in ('fontepref', 'fontepref_file', 'player_retido')]
required += [('pr493', 'originals-' + group) for group in value['legacy_groups']]
reports = []
for case_id, gate in required:
    path = root / case_id / gate / 'verified.json'
    assert path.is_file(), ('Required proof missing', str(path))
    data = json.loads(path.read_text())
    _, case = plan(case_id)
    assert data['outcome'] == 'passed' and data['source_verified'] is True
    assert data['source_commit'] == case['head'] and data['source_tree'] == case['tree']
    assert data['original_test_sha256'] == case['original_tests']
    assert data['review_commit'] == os.environ['GITHUB_SHA']
    reports.append({'case': case_id, 'gate': gate, 'source_commit': case['head'], 'source_tree': case['tree'],
                    'proof': str(path.relative_to(root))})
receipt = {'outcome': 'passed', 'review_commit': os.environ['GITHUB_SHA'], 'base': value['base'],
    'cases': [{'pr': c['pr'], 'id': c['id'], 'source_commit': c['head'], 'source_tree': c['tree']} for c in value['cases']],
    'required_gates': reports, 'required_gate_count': len(required), 'product_tests_or_workflows_added': False,
    'diagnostic_jobs_counted_as_test_passes': False,
    'ci_speed_changes': ['parallel independent required checks', 'recipe+NDK keyed native/FFmpeg dependency cache',
        'Gradle dependency cache', 'Homebrew download cache', 'one full TV/Mobile build per exact PR source',
        'small frozen patches instead of full inline product blobs']}
(root / 'all-required-gates-verified.json').write_text(json.dumps(receipt, indent=2) + '\n')
print('All required gates passed and exact source/original test bytes verified:', len(required))
