"""Closed source and execution bindings for the scoped Logs18 review."""
from pathlib import Path
import hashlib, json, os, subprocess

def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def git(source, *args):
    return subprocess.check_output(['git', '-C', str(source), *args], text=True).strip()

def load_plan(path, allow_review=False):
    path = Path(path).resolve()
    plan = json.loads(path.read_text())
    assert plan['format'] == 1 and plan['source_frozen']
    for key in ['required_artwork_quality', 'required_private_diagnostics']:
        contract = plan[key]
        assert contract['bound'] is True
        manifest = path.parent / contract['fixture_manifest']
        assert sha(manifest) == contract['fixture_manifest_sha256']
        for name, digest in json.loads(manifest.read_text())['files'].items():
            assert sha(path.parent / 'focused' / name) == digest, name
    assert plan['required_artwork_quality']['modes'] == ['native-normal', 'native-asan', 'browser-normal', 'browser-asan']
    assert sha(path.parent / plan['composition_manifest']['path']) == plan['composition_manifest']['sha256']
    for name, digest in plan.get('recipe_pins', {}).items():
        assert sha(path.parent / name) == digest, name
    approved = all(plan.get(key) is True for key in ['frozen', 'root_reviewed', 'ci_execution_permitted'])
    assert approved or allow_review, 'Root review is required before execution'
    if approved:
        assert sha(path) == os.getenv('NUVIO_REVIEW_PLAN_SHA256'), 'Wrong reviewed plan hash'
    return plan, approved

def verify_source(plan, proof, layer, source):
    entry = plan['sources'][layer]
    assert proof['verified'] and proof['source_clean'] and not proof['review_only'] and proof['original_tests_tree_unchanged']
    assert proof['layer'] == layer and proof['source_commit'] == entry['commit'] and proof['tree'] == entry['tree']
    assert proof['manifest_sha256'] == entry['manifest_sha256'] and proof['bundle_sha256'] == plan['bundle']['sha256'] and proof['base'] == plan['upstream']['commit']
    assert os.environ.get('GITHUB_SHA') and proof['review_commit'] == os.environ['GITHUB_SHA']
    assert git(source, 'rev-parse', 'HEAD') == entry['commit'] and git(source, 'rev-parse', 'HEAD^{tree}') == entry['tree'] and not git(source, 'status', '--porcelain')
    assert git(source, 'rev-parse', 'HEAD:tests') == git(source, 'rev-parse', proof['base'] + ':tests')
