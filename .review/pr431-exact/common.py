"""Read-only bindings for the exact standalone PR431 source."""
from pathlib import Path
import hashlib, json, os, subprocess

HERE = Path(__file__).resolve().parent

def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def git(source, *args):
    return subprocess.check_output(['git', '-C', str(source), *args], text=True).strip()

def plan():
    p = json.loads((HERE/'plan.json').read_text())
    assert p['format'] == 1 and p['frozen'] is True
    assert p['source']['commit'] == '85aa72edd5f033b8ef21d97744df229983db79d4'
    assert p['source']['tree'] == 'bc9a49e98919ce2c585490ad92d2b8325adfb7b6'
    return p

def verify(source, proof_path, signer_transform=False):
    p = plan(); source = Path(source).resolve(); proof = json.loads(Path(proof_path).read_text())
    assert proof['verified'] and proof['source_clean'] and proof['original_tests_tree_unchanged']
    assert proof['layer'] == 'product' and proof['source_commit'] == p['source']['commit'] and proof['tree'] == p['source']['tree']
    if os.environ.get('GITHUB_SHA'):
        assert proof['review_commit'] == os.environ['GITHUB_SHA']
    assert proof['manifest_sha256'] == p['source']['manifest_sha256']
    assert sha(HERE/p['source']['manifest']) == p['source']['manifest_sha256']
    m = json.loads((HERE/p['source']['manifest']).read_text())
    assert git(source, 'rev-parse', 'HEAD') == p['source']['commit']
    assert git(source, 'rev-parse', 'HEAD^{tree}') == p['source']['tree']
    assert git(source, 'rev-parse', 'HEAD:tests') == m['tests_tree']
    assert git(source, 'rev-parse', p['upstream']['commit']+':tests') == m['tests_tree']
    changes = git(source, 'diff', '--name-only', 'HEAD').splitlines()
    expected = ['android/app/build.gradle.kts'] if signer_transform else []
    assert changes == expected, changes
    assert not git(source, 'ls-files', '--others', '--exclude-standard'), 'Untracked source files'
    for f in m['files']:
        if not (signer_transform and f['path'] in expected):
            assert sha(source/f['path']) == f['sha256'], f['path']
    return p, proof, m
