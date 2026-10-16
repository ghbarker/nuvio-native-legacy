"""Validate exact selected public sources, test policy and cache limits before SDK work."""
from pathlib import Path
import ast, hashlib, json, os, subprocess
from common import HERE, approved, sha

p = approved()
m = json.loads((HERE / 'source-candidates.json').read_text())
w = json.loads((Path(os.environ['GITHUB_WORKSPACE']) / '.github/workflows/maintainer-review.yml').read_text())
selected = p['selected_candidates']
assert sorted(m) == sorted(selected) and selected and len(selected) == len(set(selected))
assert set(selected) <= {'431', '538', '540', '541', '543'}
assert w['on']['push']['branches'] == [p['review_branch']]
assert set(w['jobs']) == {'native', 'android'} and p['required_jobs'] == ['native', 'android']
assert w['env']['REBASE_PLAN_SHA256'] == sha(HERE / 'plan.json')
n = json.loads(json.dumps(w))
n['env']['REBASE_PLAN_SHA256'] = '<PLAN_SHA256>'
for name, job in w['jobs'].items():
    assert job['if'] is True and job['strategy']['fail-fast'] is False
    assert job['strategy']['matrix'] == {'pr': selected}
    assert job['env']['REBASE_PR'] == '${{ matrix.pr }}'
    n['jobs'][name]['if'] = '<ROOT_GATE>'
    uploads = [s for s in job['steps'] if s.get('uses', '').startswith('actions/upload-artifact@')]
    assert len(uploads) == 2
    assert any('-proof-' in s['with']['name'] and '!results/**/*.apk' in s['with']['path'] for s in uploads)
assert hashlib.sha256(json.dumps(n, sort_keys=True, separators=(',', ':')).encode()).hexdigest() == p['workflow_normalized_sha256']
counts = []
for job in w['jobs'].values():
    for step in job['steps']:
        if step.get('uses', '').startswith('actions/cache@'):
            keys = [step['with']['key']] + [x.strip() for x in step['with'].get('restore-keys', '').splitlines() if x.strip()]
            assert len(keys) <= 10 and len(keys) == len(set(keys))
            counts.append(len(keys))
assert counts == [10, 10]
steps = w['jobs']['android']['steps']
before = next(s for s in steps if s.get('name') == 'Capture strictly clean Android workspace before build')
after = next(s for s in steps if s.get('name') == 'Preserve and verify post-build Android workspace')
assert before['id'] == 'workspace_before'
assert after['if'] == "${{ always() && steps.workspace_before.conclusion == 'success' }}"
for key, source in m.items():
    assert str(source['pr']) == key and source['base'] == p['current_master']
    assert source['fresh_sdk_required'] is True
    assert source['head'] == source['public_test_commit'] and source['parents'] == [source['product_head']]
    delta = subprocess.check_output(['git', 'diff', '--name-only', source['product_head'], source['head']], text=True).splitlines()
    assert delta == source['public_test_commit_paths'] and delta and all(x.startswith('tests/') for x in delta)
    assert source['public_suites']
    for suite in source['public_suites']:
        assert suite['script'].startswith('tests/') and suite['script'].endswith('.sh')
        assert set(suite.get('modes', ['normal', 'asan'])) <= {'normal', 'asan'}
        assert suite['inputs']
        for variant in suite.get('variants', [{}]):
            assert {**suite, **variant}.get('required_markers')
for relative in p['recipe_inputs']:
    if relative.endswith('.py'):
        ast.parse((HERE / relative).read_text(), filename=relative)
print(json.dumps({'source_bound': True, 'selected_candidates': selected, 'sdk_producers': 2 * len(selected),
                  'cache_keys': counts, 'tests_are_separate_public_commits': True, 'prior_sdk_passes_reused': False}))
