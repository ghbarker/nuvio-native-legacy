"""Reject source/test/cache drift before the two actual issue503 SDK producers."""
from pathlib import Path
import ast, hashlib, json, os, subprocess
from common import HERE, approved, sha

p = approved()
m = json.loads((HERE / 'source-candidates.json').read_text())
w = json.loads((Path(os.environ['GITHUB_WORKSPACE']) / '.github/workflows/issue503-review.yml').read_text())
assert list(m) == ['503'] and p['selected_candidates'] == ['503']
assert p['internal_issue_candidate'] == 503 and p['public_pr_number'] is None
assert w['on']['push']['branches'] == [p['review_branch']]
assert set(w['jobs']) == {'native', 'android'} and p['required_jobs'] == ['native', 'android']
assert w['env']['REBASE_PLAN_SHA256'] == sha(HERE / 'plan.json')
n = json.loads(json.dumps(w))
n['env']['REBASE_PLAN_SHA256'] = '<PLAN_SHA256>'
for lane, job in w['jobs'].items():
    assert job['if'] is True and 'strategy' not in job and job['env']['REBASE_PR'] == '503'
    assert job['name'].startswith('Issue503 ')
    n['jobs'][lane]['if'] = '<ROOT_GATE>'
    uploads = [step for step in job['steps'] if step.get('uses', '').startswith('actions/upload-artifact@')]
    assert len(uploads) == 2
    assert any('-proof-' in step['with']['name'] and '!results/**/*.apk' in step['with']['path'] for step in uploads)
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
before = next(step for step in steps if step.get('name') == 'Capture strictly clean Android workspace before build')
after = next(step for step in steps if step.get('name') == 'Preserve and verify post-build Android workspace')
assert before['id'] == 'workspace_before'
assert after['if'] == "${{ always() && steps.workspace_before.conclusion == 'success' }}"
source = m['503']
assert source['base'] == p['current_master'] and source['fresh_sdk_required'] is True
assert source['head'] == source['public_test_commit'] and source['parents'] == [source['product_head']]
delta = subprocess.check_output(['git', 'diff', '--name-only', source['product_head'], source['head']], text=True).splitlines()
assert delta == source['public_test_commit_paths'] and delta and all(path.startswith('tests/') for path in delta)
assert len(source['public_suites']) == 2
suite = next(suite for suite in source['public_suites'] if suite['script'] == 'tests/artehero.sh')
assert suite['script'] == 'tests/artehero.sh' and suite['modes'] == ['normal', 'asan']
assert suite['expected_compile_count'] == suite['expected_link_count'] == 2
assert suite['compile_flag_counts'] == {'-D__EMSCRIPTEN__': 1}
assert suite['ok_marker_count'] == 30
assert suite['required_markers'] == ['artehero: tudo ok', 'artehero (tizen): tudo ok']
for suite in source['public_suites']:
    assert suite['script'].startswith('tests/') and suite['script'].endswith('.sh')
    assert suite['modes'] == ['normal', 'asan'] and suite['inputs']
    for variant in suite.get('variants', [{}]):
        rules = {**suite, **variant}
        assert rules.get('required_markers') or rules.get('marker_counts')
for relative in p['recipe_inputs']:
    if relative.endswith('.py'):
        ast.parse((HERE / relative).read_text(), filename=relative)
print(json.dumps({'source_bound': True, 'internal_issue_candidate': 503, 'public_pr_number': None,
                  'sdk_producers': 2, 'cache_keys': counts, 'native_browser_binaries_each_mode': 2,
                  'public_tests_are_separate_commit': True, 'prior_sdk_passes_reused': False}))
