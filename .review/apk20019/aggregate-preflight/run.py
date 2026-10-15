#!/usr/bin/env python3
"""Cheap aggregate metadata checks only; no compilation or product tests."""
from pathlib import Path
import copy, hashlib, importlib.util, json, tempfile
HERE = Path(__file__).resolve().parent
FILE = HERE.parent / 'aggregate.py'
spec = importlib.util.spec_from_file_location('aggregate_candidate', FILE)
module = importlib.util.module_from_spec(spec); spec.loader.exec_module(module)
plan = module.read_json(HERE.parent / 'plan.json'); plan['_sha256'] = module.sha(HERE.parent / 'plan.json')
review = 'a' * 40
checks = []
def check(name, action, reject=False):
    try:
        action()
    except (ValueError, KeyError, AssertionError, OSError):
        assert reject, name
    else:
        assert not reject, name
    checks.append(name)
jobs = {key: {'result': 'success', 'outputs': {}} for key in module.JOBS}
check('six successful required needs accepted', lambda: module.job_results(jobs))
for state in ['failure', 'cancelled', 'skipped', None]:
    bad = copy.deepcopy(jobs); bad['native']['result'] = state
    check('reject job result ' + str(state), lambda: module.job_results(bad), True)
check('reject absent need', lambda: module.job_results({key: value for key, value in jobs.items() if key != 'jvm'}), True)
check('reject extra need', lambda: module.job_results(dict(jobs, extra='success')), True)
check('reject duplicate JSON keys', lambda: module.job_results('{"native":"success","native":"success"}'), True)
for layer in ['product', 'test']:
    binding = {'source_commit': plan['sources'][layer]['commit'], 'tree': plan['sources'][layer]['tree'], 'review_commit': review, 'manifest_sha256': plan['sources'][layer]['manifest_sha256'], 'plan_sha256': plan['_sha256'], 'bundle_sha256': plan['bundle']['sha256']}
    check(layer + ' exact source binding accepted', lambda: module.source_binding(binding, layer, plan, review))
    for field in binding:
        bad = dict(binding); bad[field] = '0' * len(binding[field])
        check(layer + ' rejects wrong ' + field, lambda: module.source_binding(bad, layer, plan, review), True)
with tempfile.TemporaryDirectory(prefix='apk20019-aggregate-metadata-') as temporary:
    root = Path(temporary)
    for suffix in module.ARTIFACTS.values():
        (root / ('apk20019-' + suffix + '-' + review)).mkdir()
    for layer in ['product', 'test']:
        proof = {'verified': True, 'layer': layer, 'source_commit': plan['sources'][layer]['commit'], 'tree': plan['sources'][layer]['tree'], 'review_commit': review, 'source_clean': True, 'original_tests_tree_unchanged': True, 'review_only': False, 'base': plan['upstream']['commit'], 'manifest_sha256': plan['sources'][layer]['manifest_sha256'], 'bundle_sha256': plan['bundle']['sha256']}
        (root / ('apk20019-android-' + review) / (layer + '-source.json')).write_text(json.dumps(proof))
    gate = module.Aggregate(root, plan, review)
    check('exact materialization proofs accepted', lambda: module.Aggregate(root, plan, review))
    check('reject missing actual native receipt', lambda: gate.one('native', 'native-verified.json'), True)
    check('reject traversing relative receipt path', lambda: gate.child(root, '../outside'), True)
    check('reject absolute receipt path', lambda: gate.child(root, '/tmp/outside'), True)
    data = root / 'data'; data.write_text('actual file bytes')
    check('actual file hash accepted', lambda: gate.file(data, module.sha(data)))
    check('reject wrong actual file hash', lambda: gate.file(data, '0' * 64), True)
    link = root / 'link'; link.symlink_to(data)
    check('reject symlink input', lambda: gate.file(link), True)
    for code in [False, None, 1, -1, '0']:
        check('reject non-success return code ' + repr(code), lambda: module.need(module.zero(code), 'execution failed'), True)
    proof_path = root / ('apk20019-android-' + review) / 'test-source.json'
    proof = json.loads(proof_path.read_text()); proof['review_only'] = True; proof_path.write_text(json.dumps(proof))
    check('reject review-only source proof', lambda: module.Aggregate(root, plan, review), True)
receipt = {'outcome': 'passed', 'scope': 'Aggregate syntax/metadata guards only. No source tests, native/JVM/SDK builds, CI or provider requests.', 'aggregate_sha256': module.sha(FILE), 'plan_sha256': plan['_sha256'], 'checks': checks, 'count': len(checks)}
(HERE / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
print('PASS', len(checks), 'aggregate metadata checks')
