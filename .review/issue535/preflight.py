"""Reject source, workflow, recipe or cache drift before using an SDK."""
from pathlib import Path
import ast, hashlib, json, os, subprocess
from common import HERE, approved, sha

plan = approved()
root = Path(os.environ['GITHUB_WORKSPACE'])
workflow = json.loads((root / '.github/workflows/issue535-review.yml').read_text())
sources = json.loads((HERE / 'source-candidates.json').read_text())
assert workflow['on']['push']['branches'] == [plan['review_branch']]
assert set(workflow['jobs']) == {'native', 'android'}
android_steps = workflow['jobs']['android']['steps']
workspace_before = [step for step in android_steps if step.get('id') == 'workspace_before']
workspace_after = [step for step in android_steps if step.get('name') == 'Record and verify post-build workspace']
assert len(workspace_before) == len(workspace_after) == 1
assert workspace_before[0]['run'].endswith('--phase before')
assert workspace_after[0]['run'].endswith('--phase after')
assert workspace_after[0]['if'] == "${{ always() && steps.workspace_before.conclusion == 'success' }}"

assert workflow['env']['ISSUE535_PLAN_SHA256'] == sha(HERE / 'plan.json')
assert workflow['env']['ISSUE535_ID'] == '535'
assert list(sources) == ['535'] and sources['535']['base'] == plan['base']
assert sources['535']['head'] == plan['candidate_head']
assert sources['535']['tree'] == plan['candidate_tree']
normalized = json.loads(json.dumps(workflow))
normalized['env']['ISSUE535_PLAN_SHA256'] = '<PLAN_SHA256>'
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
