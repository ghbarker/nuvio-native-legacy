"""Require one fresh Android recovery and the identical retained native source."""
from pathlib import Path
import ast, hashlib, json, os
from common import HERE, approved, sha
p = approved()
root = Path(os.environ['GITHUB_WORKSPACE'])
w = json.loads((root / '.github/workflows/pr431-rebase3a3-review.yml').read_text())
sources = json.loads((HERE / 'source-candidates.json').read_text())
assert list(sources) == ['431']
m = sources['431']
assert m['base'] == p['base'] and m['head'] == p['candidate_head'] and m['tree'] == p['candidate_tree']
assert w['on']['push']['branches'] == [p['review_branch']] and set(w['jobs']) == {'android'}
assert p['required_jobs'] == ['android']
assert w['env']['REBASE_PR'] == '431' and w['env']['REBASE_PLAN_SHA256'] == sha(HERE / 'plan.json')
n = json.loads(json.dumps(w)); n['env']['REBASE_PLAN_SHA256'] = '<PLAN_SHA256>'
j = w['jobs']['android']
assert j['if'] is True and 'strategy' not in j and j['env']['REBASE_PR'] == '431'
n['jobs']['android']['if'] = '<ROOT_GATE>'
assert len([s for s in j['steps'] if s.get('uses', '').startswith('actions/upload-artifact@')]) == 2
assert any('-proof-' in s.get('with', {}).get('name', '') and '!results/**/*.apk' in s['with']['path'] for s in j['steps'])
before = next(s for s in j['steps'] if s.get('name') == 'Capture strictly clean Android workspace before build')
after = next(s for s in j['steps'] if s.get('name') == 'Preserve and verify post-build Android workspace')
assert before['id'] == 'workspace_before'
assert after['if'] == "${{ always() && steps.workspace_before.conclusion == 'success' }}"
assert hashlib.sha256(json.dumps(n, sort_keys=True, separators=(',', ':')).encode()).hexdigest() == p['workflow_normalized_sha256']
counts = []
for step in j['steps']:
    if step.get('uses', '').startswith('actions/cache@'):
        keys = [step['with']['key']] + [x for x in step['with'].get('restore-keys', '').splitlines() if x.strip()]
        assert len(keys) <= 10 and len(keys) == len(set(keys)); counts.append(len(keys))
assert counts == [10, 10]
for relative in p['recipe_inputs']:
    if (HERE / relative).suffix == '.py': ast.parse((HERE / relative).read_text(), filename=relative)
assert p['fresh_runtime'] == {'compiled_fixtures': 22, 'executed_cases': 70, 'settings_cases': 48, 'storage_error_geometry_cases': 12, 'tv_notice_observations': 12, 'normal_and_sanitizer_modes': ['normal', 'address+undefined']}
assert p['fresh_android_kotlin'] == ['interface-mode', 'registro-download']
retained = json.loads((HERE / 'retained-native.json').read_text())
assert retained['require_actual_original_job_success_and_raw_proof_before_ready']
assert retained['source_identity_required'] and not retained['previous_pass_relabelled_as_new_execution']
assert retained['fresh_runtime_and_kotlin_passes_required'] and not retained['product_source_changed']
assert retained['original_source_commit'] == m['head'] == p['retained_native_source_commit']
assert retained['original_source_tree'] == m['tree'] == p['retained_native_source_tree']
assert retained['original_source_base'] == m['base']
assert retained['original_source_manifest_sha256'] == p['source_manifest_sha256']
assert retained['original_run_id'] == p['retained_native_run_id']
assert retained['original_job_id'] == p['retained_native_job_id']
assert retained['original_review_commit'] == p['retained_native_review_commit']
compat = json.loads((HERE / 'fixture-compatibility.json').read_text())
delta = compat['reviewed_fixture_delta']
assert delta['path'] == 'runtime/fixtures/settings_runtime.c'
assert delta['old_sha256'] == 'f0061ebd4e1b9eb3bad64d325f00beb8710dfa78768dfd51611b9ee5f1d2ef14'
assert delta['new_sha256'] == sha(HERE / 'fixtures/pr431' / delta['path'])
assert delta['runtime_cases_preserved'] == 70 and delta['all_other_fixture_bytes_unchanged']
assert compat['all_other_runtime_and_android_fixture_bytes_unchanged']
for relative, digest in compat['fixture_files_sha256'].items(): assert sha(HERE / 'fixtures/pr431' / relative) == digest, relative
identity = {'verified': True, 'review_commit': os.environ['GITHUB_SHA'], 'workflow_run_id': int(os.environ['GITHUB_RUN_ID']), 'workflow_run_attempt': int(os.environ['GITHUB_RUN_ATTEMPT']), 'plan_sha256': sha(HERE / 'plan.json'), 'source_manifest_sha256': p['source_manifest_sha256'], 'retained_origin_sha256': sha(HERE / 'retained-native.json'), 'source_commit': m['head'], 'source_tree': m['tree'], 'original_native_run_id': retained['original_run_id'], 'original_native_job_id': retained['original_job_id'], 'original_native_review_commit': retained['original_review_commit'], 'identical_source_identity': True, 'native_reexecuted': False, 'original_native_success_claimed_here': False, 'runtime_passes_reused': False, 'scope': 'Same immutable product source as original actual macOS producer; root must independently verify that original job and raw proof before readiness.'}
out = root / 'results'; out.mkdir(exist_ok=True)
(out / 'retained-native-identity.json').write_text(json.dumps(identity, indent=2) + '\n')
print(json.dumps({'closed_recipe': True, 'source': m['head'], 'sdk_producers': 1, 'cache_keys': counts, 'runtime_host': 'Linux Android lane', 'runtime_cases_preserved': 70, 'native_source_identical': True, 'native_reexecuted': False, 'runtime_passes_reused': False}))
