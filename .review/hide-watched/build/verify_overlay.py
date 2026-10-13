#!/usr/bin/env python3
"""Verify both frozen overlays before any compile, and preserve settings IDs."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess


def digest(path):
    data = path.read_bytes()
    return hashlib.sha1(b'blob ' + str(len(data)).encode() + b'\0' + data).hexdigest()


def verify(files, key='sha'):
    checked = []
    seen = set()
    for entry in files:
        relative = entry['path']
        assert relative.startswith(('src/', 'tests/')) and '..' not in Path(relative).parts
        assert relative not in seen
        seen.add(relative)
        path = Path(relative)
        assert digest(path) == entry[key], (relative, key, digest(path), entry[key])
        assert entry['mode'] in ('100644', '100755')
        assert bool(path.stat().st_mode & 0o111) == (entry['mode'] == '100755')
        checked.append({'path': relative, 'sha': digest(path), 'mode': entry['mode']})
    assert checked
    return checked


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--review-root', default='.review/hide-watched')
    args = parser.parse_args()
    root = Path(args.review_root)
    retained_commit = 'c5f0118c8889ba3930fd7230388603aa41153354'
    subprocess.run(['git', 'merge-base', '--is-ancestor', retained_commit, 'HEAD'], check=True)
    changes = subprocess.check_output(['git', 'diff', '--name-only', '-z', retained_commit, 'HEAD']).decode().split('\0')
    assert all(not p or p.startswith(('.github/workflows/', '.review/')) for p in changes), changes
    baseline = json.loads(Path('.review/source-metadata-files.json').read_text())
    assert baseline['base'] == '3b5bfc78a0deaa4fe0f2b33b7c33bf3cd2ec81b8'
    subprocess.run(['git', 'apply', '--check', '--unidiff-zero', '.review/source-metadata.patch'], check=True)
    subprocess.run(['git', 'apply', '--unidiff-zero', '.review/source-metadata.patch'], check=True)
    checked_baseline = verify(baseline['files'])
    plan = json.loads((root / 'combined-files.json').read_text())
    assert plan['retained_commit'] == retained_commit
    assert plan['upstream_base'] == '17bb330373273a3de284d7f2898a3a3e16f1fc3f'
    subprocess.run(['git', 'merge-base', '--is-ancestor', plan['upstream_base'], plan['feature_commit']], check=True)
    feature_paths = subprocess.check_output([
        'git', 'diff', '--name-only', '-z', plan['upstream_base'], plan['feature_commit']
    ]).decode().split('\0')
    assert sorted(p for p in feature_paths if p) == sorted(e['path'] for e in plan['files'])
    for entry in plan['files']:
        for field, ref in (('upstream_sha', plan['upstream_base']), ('feature_sha', plan['feature_commit'])):
            expected = subprocess.check_output(['git', 'rev-parse', ref + ':' + entry['path']], text=True).strip()
            assert expected == entry[field], (entry['path'], field, expected, entry[field])
    checked_inputs = verify(plan['files'], 'retained_sha')
    old_settings = Path('src/ajustes.c').read_bytes()
    old_defaults = Path('src/ajustes_ux_padrao.inc').read_bytes()
    subprocess.run(['git', 'apply', '--check', str(root / 'combined.patch')], check=True)
    subprocess.run(['git', 'apply', str(root / 'combined.patch')], check=True)
    checked_final = verify(plan['files'])
    # The exact source parser used when freezing runs again on the build host.
    import importlib.util
    spec = importlib.util.spec_from_file_location('prepare_combined', root / 'build/prepare_combined.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    settings = module.assert_settings_preserved(
        old_settings, Path('src/ajustes.c').read_bytes(),
        old_defaults, Path('src/ajustes_ux_padrao.inc').read_bytes())
    assert settings == plan['settings']
    final_paths = {e['path'] for e in plan['files']}
    untouched_retained = verify([e for e in baseline['files'] if e['path'] not in final_paths])
    output = Path('build/review')
    output.mkdir(parents=True, exist_ok=True)
    proof = {'build_commit': os.environ['GITHUB_SHA'], 'retained_commit': retained_commit,
             'feature_commit': plan['feature_commit'], 'upstream_base': plan['upstream_base'],
             'verified': True, 'settings': settings, 'retained_files': checked_baseline,
             'merge_inputs': checked_inputs, 'final_files': checked_final,
             'untouched_retained_files': untouched_retained}
    (output / 'combined-source-verified.json').write_text(json.dumps(proof, indent=2) + '\n')
    print('Exact retained failover/filter/metadata overlay verified:', len(checked_baseline))
    print('Issue #366 merged source blobs verified:', len(checked_final))
    print('All retained Settings IDs/options/keys/defaults unchanged:', settings['retained_setting_count'])


if __name__ == '__main__':
    main()
