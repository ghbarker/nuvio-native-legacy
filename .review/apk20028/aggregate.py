"""Require exact current-run actual-C and compatible APK proofs for Logs18."""
from pathlib import Path
import argparse, hashlib, json, os, subprocess
from require_plan import load_plan, sha

parser = argparse.ArgumentParser()
parser.add_argument('--artifacts', type=Path, required=True)
parser.add_argument('--output', type=Path, required=True)
args = parser.parse_args()
here = Path(__file__).resolve().parent
plan, approved = load_plan(here / 'plan.json')
assert approved
needs = {key: value['result'] for key, value in json.loads(os.environ['SCOPED_JOB_RESULTS']).items()}
assert set(needs) == set(plan['required_jobs']) and all(value == 'success' for value in needs.values()), needs
root = args.artifacts.resolve()
records = []

def source_hash(commit, path):
    return hashlib.sha256(subprocess.check_output(['git', 'show', commit + ':' + path])).hexdigest()

def verify_proof(path, layer):
    proof = json.loads(path.read_text())
    entry = plan['sources'][layer]
    assert proof['verified'] and proof['source_clean'] and not proof['review_only'] and proof['original_tests_tree_unchanged']
    assert proof['layer'] == layer and proof['source_commit'] == entry['commit'] and proof['tree'] == entry['tree']
    assert proof['review_commit'] == os.environ['GITHUB_SHA'] and proof['base'] == plan['upstream']['commit']
    assert proof['manifest_sha256'] == entry['manifest_sha256'] and proof['bundle_sha256'] == plan['bundle']['sha256']

for job, layers in [('focused-linux', ['product', 'test']), ('focused-macos', ['product'])]:
    files = list((root / job).rglob('focused-verified.json'))
    assert len(files) == 1, job
    path = files[0]
    proof = json.loads(path.read_text())
    assert proof['passed'] and not proof['prior_passes_reused']
    assert proof['review_commit'] == os.environ['GITHUB_SHA'] and proof['run_id'] == int(os.environ['GITHUB_RUN_ID']) and proof['run_attempt'] == int(os.environ['GITHUB_RUN_ATTEMPT'])
    assert proof['plan_sha256'] == sha(here / 'plan.json')
    assert proof['quality_fixture_manifest_sha256'] == plan['required_artwork_quality']['fixture_manifest_sha256']
    assert proof['diagnostic_fixture_manifest_sha256'] == plan['required_private_diagnostics']['fixture_manifest_sha256']
    expected = {(layer, 'quality', mode) for layer in layers for mode in ['normal', 'asan']}
    if job == 'focused-linux':
        expected |= {('test', 'diagnostics', mode) for mode in ['normal', 'asan']}
    assert proof['diagnostics_executed'] == (job == 'focused-linux')
    assert {(entry['layer'], entry['kind'], entry['mode']) for entry in proof['records']} == expected
    for entry in proof['records']:
        source = plan['sources'][entry['layer']]
        assert entry['source_commit'] == source['commit'] and entry['tree'] == source['tree']
        source_proof = path.parent.parent / (entry['layer'] + '-source.json')
        assert sha(source_proof) == entry['source_proof_sha256']
        verify_proof(source_proof, entry['layer'])
        receipt_path = path.parent / entry['receipt']
        assert sha(receipt_path) == entry['receipt_sha256']
        receipt = json.loads(receipt_path.read_text())
        assert receipt['success'] and receipt['source_commit'] == source['commit'] and receipt['sanitize'] == (entry['mode'] == 'asan')
        for name, digest in receipt['source_hashes'].items():
            assert source_hash(source['commit'], name) == digest, name
        fixture = json.loads((here / plan['required_artwork_quality' if entry['kind'] == 'quality' else 'required_private_diagnostics']['fixture_manifest']).read_text())
        assert receipt['runner_sha256'] == fixture['files'][entry['kind'] + '/run.py']
        if entry['kind'] == 'quality':
            assert receipt['fixture_sha256'] == fixture['files']['quality/artehero-fixture.c']
            assert {(run['mode'], run['case_groups']) for run in receipt['runs']} == {('native', 27), ('browser', 3)}
        else:
            assert receipt['macro_off_production_unchanged'] and receipt['native_and_android_detail_syntax_passed'] and receipt['full_detail_syntax_skipped'] is False
            assert set(receipt['fixture_sha256']) == {'detail-probe.c', 'cache-boundary.c', 'baseline.json'}
            for name, digest in receipt['fixture_sha256'].items():
                assert fixture['files']['diagnostics/' + name] == digest
            assert len(receipt['runs']) == 1 and receipt['runs'][0]['mode'] == 'native'
        for run in receipt['runs']:
            name = run['mode']
            assert sha(receipt_path.parent / name) == run['binary_sha256']
            if entry['kind'] == 'quality':
                assert run['compile_exit'] == 0 and run['exit'] == 0
                assert sha(receipt_path.parent / (name + '-compile.log')) == run['compile_log_sha256']
                assert sha(receipt_path.parent / (name + '.log')) == run['log_sha256']
            else:
                assert 'private-art-trace: 30 checks passed' in run['stdout']
                assert {'native-syntax.log', 'android-syntax.log', '0-compile.log', '1-compile.log', 'link.log', 'native.log', 'native-compile.log'} == set(run['logs_sha256'])
                for name, digest in run['logs_sha256'].items():
                    assert sha(receipt_path.parent / name) == digest
                assert sha(receipt_path.parent / 'actual-detail-trace.inc') == run['generated_detail_trace_sha256']
                assert sha(receipt_path.parent / 'actual-cache-snapshot.c') == run['generated_cache_source_sha256']
    records.append({'job': job, 'proof_sha256': sha(path), 'invocations': len(proof['records'])})

# The proven Android APK verifier below is retained after version normalization.
found=list((root/'android').rglob('android-test/build-receipt.json'));assert len(found)==1;bp=found[0];b=json.loads(bp.read_text());e=plan['sources']['test'];assert b['verified']and b['layer']=='test'and b['source_commit']==e['commit']and b['tree']==e['tree']and b['review_commit']==os.environ['GITHUB_SHA'];assert b['plan_sha256']==sha(here/'plan.json')and b['fresh_build_of_both_arms']and not b['prior_apk_proofs_reused']and b['source_restored_clean'];apk=bp.parent/b['apk_name'];assert sha(apk)==b['apk_sha256'];vp=bp.parent/'android-verified.json';assert sha(vp)==b['verification_sha256'];v=json.loads(vp.read_text());assert v['outcome']=='passed'and v['source_commit']==e['commit']and v['tree']==e['tree']and v['review_commit']==os.environ['GITHUB_SHA'];assert v['package']==plan['identity']['package']and v['version_code']==20028 and v['version_name']==plan['identity']['version_name']and v['certificate_sha256']==plan['identity']['certificate_sha256'];assert v['native_16kb_pages']=='passed'and v['private_log_opt_in']and not v['prior_apk_proofs_reused'];assert {x['abi']for x in v['actual_arm_compiles']}=={'arm64-v8a','armeabi-v7a'}
sp=bp.parent.parent/'test-source.json';assert sha(sp)==b['source_proof_sha256']==v['source_proof_sha256'];proof=json.loads(sp.read_text());assert proof['verified']and proof['source_clean']and proof['original_tests_tree_unchanged']and not proof['review_only'];assert proof['source_commit']==e['commit']and proof['tree']==e['tree']and proof['review_commit']==os.environ['GITHUB_SHA'];assert proof['manifest_sha256']==e['manifest_sha256']and proof['bundle_sha256']==plan['bundle']['sha256']
tr=bp.parent/'ci-transforms.json';assert sha(tr)==b['ci_only_transforms_sha256']==v['ci_transforms_sha256'];assert json.loads(tr.read_text())['changes']==[]
records.append({'job':'android','proof_sha256':sha(bp),'apk_name':b['apk_name'],'apk_sha256':b['apk_sha256'],'apk_bytes':apk.stat().st_size,'verification_sha256':sha(vp)})

result={'all_passed':True,'review_commit':os.environ['GITHUB_SHA'],'run_id':int(os.environ['GITHUB_RUN_ID']),'run_attempt':int(os.environ['GITHUB_RUN_ATTEMPT']),'plan_sha256':sha(here/'plan.json'),'sources':plan['sources'],'identity':plan['identity'],'jobs':records,'scope':plan['scope'],'previous_passes_count_as_fresh':False};args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(result,indent=2)+'\n');print('All scoped source and compatible APK gates passed')
