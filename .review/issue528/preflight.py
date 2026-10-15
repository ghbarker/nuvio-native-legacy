"""Reject source, workflow, recipe or cache drift before using an SDK."""
from pathlib import Path
import ast, hashlib, json, os, subprocess
from common import HERE, approved, sha

plan = approved()
root = Path(os.environ['GITHUB_WORKSPACE'])
workflow = json.loads((root / '.github/workflows/issue528-review.yml').read_text())
sources = json.loads((HERE / 'source-candidates.json').read_text())
assert workflow['on']['push']['branches'] == [plan['review_branch']]
assert set(workflow['jobs']) == {'native', 'android'}
assert workflow['env']['ISSUE528_PLAN_SHA256'] == sha(HERE / 'plan.json')
assert workflow['env']['ISSUE528_ID'] == '528'
assert list(sources) == ['528'] and sources['528']['base'] == plan['base']
assert sources['528']['head'] == plan['candidate_head']
assert sources['528']['tree'] == plan['candidate_tree']
normalized = json.loads(json.dumps(workflow))
normalized['env']['ISSUE528_PLAN_SHA256'] = '<PLAN_SHA256>'
for kind in ['native', 'android']:
    job = workflow['jobs'][kind]
    assert job['if'] is True, 'Workflow must be explicitly opened after review'
    assert 'strategy' not in job, 'This is one isolated source, not a matrix'
    normalized['jobs'][kind]['if'] = '<ROOT_GATE>'
assert hashlib.sha256(json.dumps(normalized, sort_keys=True, separators=(',', ':')).encode()).hexdigest() == plan['workflow_normalized_sha256']
counts = []
for job in workflow['jobs'].values():
    for step in job['steps']:
        if step.get('uses', '').startswith('actions/cache@'):
            keys = [step['with']['key']] + [x for x in step['with'].get('restore-keys', '').splitlines() if x.strip()]
            assert len(keys) <= 10 and len(keys) == len(set(keys))
            counts.append(len(keys))
assert counts == [6, 6]
for relative in plan['recipe_inputs']:
    path = HERE / relative
    if path.suffix == '.py':
        ast.parse(path.read_text(), filename=str(path))
print(json.dumps({'closed_recipe': True, 'candidate': plan['candidate_head'],
    'source_manifest_sha256': plan['source_manifest_sha256'], 'sdk_producers': 2,
    'cache_keys': counts, 'unrelated_feature_suites': False}))
