"""Run fresh source-bound artwork policy and private diagnostics regressions."""
from pathlib import Path
import argparse, json, os, platform, subprocess, sys, time
from require_plan import load_plan, verify_source, sha

parser = argparse.ArgumentParser()
parser.add_argument('--layers', nargs='+', choices=['product', 'test'], required=True)
parser.add_argument('--source-root', type=Path, required=True)
parser.add_argument('--proof-root', type=Path, required=True)
parser.add_argument('--output', type=Path, required=True)
parser.add_argument('--diagnostics', action='store_true')
args = parser.parse_args()
here = Path(__file__).resolve().parent
plan, approved = load_plan(here / 'plan.json')
assert approved
out = args.output.resolve()
out.mkdir(parents=True, exist_ok=True)
records = []
for layer in args.layers:
    source = (args.source_root / ('source-' + layer)).resolve()
    source_proof = (args.proof_root / (layer + '-source.json')).resolve()
    proof = json.loads(source_proof.read_text())
    verify_source(plan, proof, layer, source)
    kinds = ['quality'] + (['diagnostics'] if args.diagnostics and layer == 'test' else [])
    for kind in kinds:
        for mode in ['normal', 'asan']:
            destination = out / (layer + '-' + kind + '-' + mode)
            command = [sys.executable, str(here / 'focused' / kind / 'run.py'), '--source', str(source), '--out', str(destination), '--sanitize', '1' if mode == 'asan' else '0']
            start = time.monotonic()
            subprocess.run(command, check=True)
            receipt = json.loads((destination / 'receipt.json').read_text())
            assert receipt['success'] and receipt['source_commit'] == proof['source_commit'] and receipt['sanitize'] == (mode == 'asan')
            if kind == 'quality':
                assert {(entry['mode'], entry['case_groups']) for entry in receipt['runs']} == {('native', 27), ('browser', 3)}
            else:
                assert receipt['macro_off_production_unchanged'] and receipt['native_and_android_detail_syntax_passed'] and receipt['full_detail_syntax_skipped'] is False
            records.append({'layer': layer, 'kind': kind, 'mode': mode, 'source_commit': proof['source_commit'], 'tree': proof['tree'], 'source_proof_sha256': sha(source_proof), 'receipt': str(destination.relative_to(out) / 'receipt.json'), 'receipt_sha256': sha(destination / 'receipt.json'), 'seconds': round(time.monotonic() - start, 3)})
assert not args.diagnostics or 'test' in args.layers
result = {'passed': True, 'review_commit': os.environ['GITHUB_SHA'], 'run_id': int(os.environ['GITHUB_RUN_ID']), 'run_attempt': int(os.environ['GITHUB_RUN_ATTEMPT']), 'plan_sha256': sha(here / 'plan.json'), 'platform': platform.system(), 'quality_fixture_manifest_sha256': plan['required_artwork_quality']['fixture_manifest_sha256'], 'diagnostic_fixture_manifest_sha256': plan['required_private_diagnostics']['fixture_manifest_sha256'], 'records': records, 'prior_passes_reused': False, 'diagnostics_executed': args.diagnostics}
(out / 'focused-verified.json').write_text(json.dumps(result, indent=2) + '\n')
print(json.dumps({'passed': True, 'invocations': len(records), 'platform': platform.system(), 'diagnostics': args.diagnostics}))
