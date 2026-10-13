#!/usr/bin/env python3
"""Verify the retained test app, then apply only the frozen profile-session overlay."""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import subprocess

EMPTY = hashlib.sha1(b'blob 0\0').hexdigest()
BUILD_SCRIPTS = {'tools/android.sh', 'tools/arm.sh'}
def digest(path):
    data = path.read_bytes() if path.is_file() else b''
    return hashlib.sha1(b'blob ' + str(len(data)).encode() + b'\0' + data).hexdigest()
def verify(entries, key='sha', allowed_paths=None):
    checked = []
    for entry in entries:
        relative = entry['path']
        assert not Path(relative).is_absolute() and '..' not in Path(relative).parts
        if allowed_paths is None:
            assert relative.startswith('src/') or relative in BUILD_SCRIPTS
        else:
            # Only prior, successfully verified retained files receive this
            # exception. New feature and preview inputs remain product-only.
            assert relative in allowed_paths
        path = Path(relative)
        assert digest(path) == entry[key], (relative, key, digest(path), entry[key])
        if entry[key] != EMPTY:
            assert path.is_file() and bool(path.stat().st_mode & 0o111) == (entry['mode'] == '100755')
        checked.append({'path': relative, 'sha': digest(path), 'mode': entry['mode']})
    return checked

def git_blob(ref, path):
    result = subprocess.run(['git', 'rev-parse', ref + ':' + path], capture_output=True, text=True)
    return result.stdout.strip() if result.returncode == 0 else EMPTY

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--review-root', default='.review/profile-session')
    args = parser.parse_args()
    root = Path(args.review_root)
    plan = json.loads((root / 'combined-files.json').read_text())
    subprocess.run(['git', 'merge-base', '--is-ancestor', plan['retained_commit'], 'HEAD'], check=True)
    changed = subprocess.check_output(['git', 'diff', '--name-only', '-z', plan['retained_commit'], 'HEAD']).decode().split('\0')
    assert all(not p or p.startswith(('.review/', '.github/workflows/')) for p in changed)
    # The parent contains review overlays, so reconstruct its tested product bytes first.
    subprocess.run(['python3', '.review/hide-watched/build/verify_overlay.py'], check=True)
    retained_proof = json.loads(Path('build/review/combined-source-verified.json').read_text())
    subprocess.run(['git', 'merge-base', '--is-ancestor', plan['upstream_base'], plan['feature_commit']], check=True)
    feature_paths = sorted(p for p in subprocess.check_output(['git', 'diff', '--name-only', '-z', plan['upstream_base'], plan['feature_commit']]).decode().split('\0') if p)
    assert feature_paths == sorted(e['path'] for e in plan['files'])
    for entry in plan['files']:
        assert git_blob(plan['upstream_base'], entry['path']) == entry['upstream_sha']
        assert git_blob(plan['feature_commit'], entry['path']) == entry['feature_sha']
    inputs = verify(plan['files'], 'retained_sha')
    preview = plan.get('preview_only_files', [])
    preview_inputs = verify(preview, 'retained_sha')
    old_settings = Path('src/ajustes.c').read_bytes()
    old_defaults = Path('src/ajustes_ux_padrao.inc').read_bytes()
    subprocess.run(['git', 'apply', '--check', str(root / 'combined.patch')], check=True)
    subprocess.run(['git', 'apply', str(root / 'combined.patch')], check=True)
    final = verify(plan['files'])
    preview_final = verify(preview)
    spec = importlib.util.spec_from_file_location('profile_merge', root / 'build/prepare_combined.py')
    module = importlib.util.module_from_spec(spec); spec.loader.exec_module(module)
    settings = module.assert_settings_preserved(old_settings, Path('src/ajustes.c').read_bytes(), old_defaults, Path('src/ajustes_ux_padrao.inc').read_bytes())
    assert settings == plan['settings']
    modified = {e['path'] for e in plan['files'] + preview}
    retained_entries = {e['path']: e for e in retained_proof['retained_files']}
    retained_entries.update({e['path']: e for e in retained_proof['final_files']})
    untouched = verify([e for p, e in retained_entries.items() if p not in modified],
                       allowed_paths=set(retained_entries))
    proof = {'build_commit': os.environ['GITHUB_SHA'], 'retained_commit': plan['retained_commit'],
             'feature_commit': plan['feature_commit'], 'upstream_base': plan['upstream_base'],
             'verified': True, 'settings': settings, 'retained_files': list(retained_entries.values()),
             'merge_inputs': inputs, 'final_files': final, 'preview_only_inputs': preview_inputs,
             'preview_only_files': preview_final, 'untouched_retained_files': untouched}
    Path('build/review/retained-source-verified.json').write_text(json.dumps(retained_proof, indent=2) + '\n')
    Path('build/review/combined-source-verified.json').write_text(json.dumps(proof, indent=2) + '\n')
    print('Profile-session source and retained feature hashes verified:', len(final), 'source files')
    print('Retained Settings indices preserved:', settings['retained_setting_count'])
if __name__ == '__main__': main()
