#!/usr/bin/env python3
"""Materialize reviewed patches and publish new candidate refs only."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

from verify_source import ROOT, blob, load_plan

def git(*args, cwd=None, env=None, capture=False):
    return subprocess.run(['git', *args], cwd=cwd, env=env, check=True,
                          capture_output=capture, text=True)

plan = load_plan()
out = Path('build/review')
out.mkdir(parents=True, exist_ok=True)
task_temp = Path(os.environ['RUNNER_TEMP']) / 'source-preload-materialization'
task_temp.mkdir(parents=True, exist_ok=True)
askpass = task_temp / 'git-askpass.sh'
askpass.write_text('#!/bin/sh\ncase "$1" in\n*Username*) printf "%s\\n" x-access-token;;\n*Password*) printf "%s\\n" "$GH_TOKEN";;\n*) exit 1;;\nesac\n')
askpass.chmod(0o700)
network_env = dict(os.environ, GIT_ASKPASS=str(askpass), GIT_TERMINAL_PROMPT='0')
assert network_env.get('GH_TOKEN')
prepared = []
for case in plan['cases']:
    patch = ROOT / case['patch']
    assert hashlib.sha256(patch.read_bytes()).hexdigest() == case['patch_sha256'], case['id']
    manifest = json.loads((ROOT / case['manifest']).read_text())
    assert manifest.get('base', manifest.get('upstream_commit')) == case['base']
    entries = manifest['files']
    assert len({e['path'] for e in entries}) == len(entries) > 0
    for entry in entries:
        relative = Path(entry['path'])
        assert not relative.is_absolute() and '..' not in relative.parts
        assert not entry['path'].startswith(('.review/', '.github/'))
        assert entry['mode'] in ('100644', '100755')
    if subprocess.run(['git', 'cat-file', '-e', case['base']], capture_output=True).returncode:
        git('fetch', 'origin', case['base'], env=network_env)
    target = task_temp / case['id']
    assert not target.exists()
    git('worktree', 'add', '--detach', str(target), case['base'])
    git('apply', '--check', str(patch.resolve()), cwd=target)
    git('apply', '--index', str(patch.resolve()), cwd=target)
    changed = git('diff', '--cached', '--name-only', '-z', cwd=target, capture=True).stdout.split('\0')
    assert sorted(p for p in changed if p) == sorted(e['path'] for e in entries)
    for entry in entries:
        file = target / entry['path']
        actual = blob(file.read_bytes())
        staged = git('rev-parse', ':' + entry['path'], cwd=target, capture=True).stdout.strip()
        assert actual == staged == entry['sha'], (case['id'], entry['path'], actual, staged, entry['sha'])
        assert bool(file.stat().st_mode & 0o111) == (entry['mode'] == '100755')
    actor = os.environ['GITHUB_ACTOR']
    git('-c', 'user.name=' + actor, '-c', 'user.email=' + actor + '@users.noreply.github.com',
        'commit', '-m', 'Materialize reviewed ' + case['label'] + ' on pinned upstream', cwd=target)
    source = git('rev-parse', 'HEAD', cwd=target, capture=True).stdout.strip()
    assert git('rev-list', '--parents', '-n', '1', 'HEAD', cwd=target, capture=True).stdout.split() == [source, case['base']]
    branch = 'candidate/source-preload-final-' + os.environ['GITHUB_RUN_ID'] + '-' + os.environ['GITHUB_RUN_ATTEMPT'] + '-' + case['id']
    lookup = subprocess.run(['git', 'ls-remote', '--exit-code', 'origin', 'refs/heads/' + branch], env=network_env, capture_output=True, text=True)
    assert lookup.returncode == 2, 'Candidate branch already exists or remote lookup failed: ' + branch
    git('push', 'origin', 'HEAD:refs/heads/' + branch, cwd=target, env=network_env)
    updated = dict(case, source_commit=source, candidate_branch=branch)
    prepared.append(updated)
    proof = {'base': case['base'], 'source_commit': source, 'candidate_branch': branch,
             'review_commit': os.environ['GITHUB_SHA'], 'verified_product_files': len(entries),
             'patch_sha256': case['patch_sha256'], 'one_real_parent_verified': True,
             'existing_refs_updated': False, 'files': entries}
    (out / ('prepared-' + case['id'] + '.json')).write_text(json.dumps(proof, indent=2) + '\n')
delivery = next((c for c in prepared if c['delivery']), None)
media3 = next((c for c in prepared if c['id'] == plan.get('media3_case')), delivery)
result = {'matrix': {'include': prepared},
          'native_matrix': {'include': [c for c in prepared if not c.get('skip_native', False)]},
          'phase': plan['validation_phase'], 'publish': plan['publish_release'],
          'delivery': delivery, 'media3': media3}
(out / 'prepared-cases.json').write_text(json.dumps(result, indent=2) + '\n')
with open(os.environ['GITHUB_OUTPUT'], 'a') as stream:
    for key, value in result.items():
        text = value if key == 'phase' else str(value).lower() if key == 'publish' else json.dumps(value, separators=(',', ':'))
        stream.write(key + '=' + text + '\n')
print('Published', len(prepared), 'new candidate refs with verified real pinned ancestry; existing refs unchanged')
