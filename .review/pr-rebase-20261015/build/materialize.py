#!/usr/bin/env python3
"""Publish immutable validation candidates, never the existing PR branches."""
import json
import os
from pathlib import Path
import subprocess
import tempfile
from verify_source import REVIEW, verify

value = json.loads((REVIEW / 'cases.json').read_text())
root = Path.cwd().resolve()
assert os.environ['GITHUB_REPOSITORY'] == 'ghbarker/nuvio-native-legacy'
assert value['base'] == '3c28c0d73664fe82fabced499bd7171810e0c91b'
assert len(value['cases']) == 3
subprocess.run(['git', 'fetch', '--no-tags', 'https://github.com/iqui27/nuvio-native-legacy.git', value['base']], check=True)
proofs = []
# ASKPASS reads the ephemeral token only when Git asks; it is never interpolated
# into command arguments, logged, copied into config, or included in artifacts.
with tempfile.TemporaryDirectory(prefix='rebase-auth-') as temp:
    askpass = Path(temp) / 'askpass'
    askpass.write_text('#!/bin/sh\ncase "$1" in *Username*) printf "%s\\n" x-access-token ;; *) printf "%s\\n" "$GH_TOKEN" ;; esac\n')
    askpass.chmod(0o700)
    env = dict(os.environ, GIT_ASKPASS=str(askpass), GIT_TERMINAL_PROMPT='0',
               GIT_AUTHOR_NAME='ghbarker', GIT_AUTHOR_EMAIL='ghbarker@users.noreply.github.com',
               GIT_COMMITTER_NAME='ghbarker', GIT_COMMITTER_EMAIL='ghbarker@users.noreply.github.com')
    for case in value['cases']:
        target = root / ('materialize-' + case['id'])
        subprocess.run(['git', 'worktree', 'add', '--detach', str(target), value['base']], check=True)
        subprocess.run(['git', 'apply', '--index', str(REVIEW / 'patches' / (case['id'] + '.patch'))], cwd=target, check=True)
        tree = subprocess.check_output(['git', 'write-tree'], cwd=target, text=True).strip()
        assert tree == case['tree'], (case['id'], tree, case['tree'])
        message = case['commit_subject'] + '\n\nRebased on current upstream for validation.\n'
        commit = subprocess.check_output(['git', 'commit-tree', tree, '-p', value['base']],
            cwd=target, env=env, input=message, text=True).strip()
        case['head'] = commit
        subprocess.run(['git', 'reset', '--hard', commit], cwd=target, check=True)
        proof = verify(target, value, case)
        branch = 'candidate/pr-conflicts-rebased-' + case['id'] + '-' + os.environ['GITHUB_RUN_ID'] + '-' + os.environ['GITHUB_RUN_ATTEMPT']
        ref = 'refs/heads/' + branch
        assert ref.startswith('refs/heads/candidate/pr-conflicts-rebased-')
        subprocess.run(['git', '-c', 'credential.helper=', 'push', 'origin', commit + ':' + ref],
                       cwd=target, env=env, check=True)
        proof['candidate_ref'] = ref
        proofs.append(proof)
        print('Exact rebased candidate published:', case['id'], commit, tree, flush=True)
out = root / 'build/candidate-plan'
out.mkdir(parents=True, exist_ok=True)
(out / 'cases.json').write_text(json.dumps(value, indent=2) + '\n')
(out / 'materialization.json').write_text(json.dumps({'outcome': 'passed', 'candidates': proofs,
    'existing_pr_refs_updated': False}, indent=2) + '\n')
matrix = {'include': [{'id': c['id'], 'pr': c['pr'], 'source_commit': c['head']} for c in value['cases']]}
with Path(os.environ['GITHUB_OUTPUT']).open('a') as output:
    output.write('matrix=' + json.dumps(matrix, separators=(',', ':')) + '\n')
