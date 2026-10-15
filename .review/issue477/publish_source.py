#!/usr/bin/env python3
"""Publish only the exact green product source to its dedicated fork branch."""
import argparse, json, os, re, subprocess, tempfile
from common import source_binding
from pathlib import Path

SOURCE = '41c3e67a9472ff4d8f8b852d57a605c26c5cab3d'
TREE = '3a784281d39e007d05c1db606503da97c6e78578'
BASE = 'e3e21a590120ca283bcf7a96df1813ff5041eb66'
REPO = 'ghbarker/nuvio-native-legacy'
REMOTE = 'https://github.com/' + REPO + '.git'
REF = 'refs/heads/feat/synopsis-text-size-477-20261015'

def publish(source, proof_path, output, run=subprocess.run, read=subprocess.check_output):
    source = Path(source).resolve()
    proof = json.loads(Path(proof_path).read_text())
    source_binding(source, proof_path)
    review = os.environ['GITHUB_SHA']
    assert re.fullmatch(r'[0-9a-f]{40}', review)
    assert os.environ['GH_REPO'] == REPO and os.environ.get('GH_TOKEN')
    assert proof['verified'] and proof['source_commit'] == SOURCE and proof['tree'] == TREE
    assert proof['base'] == BASE and proof['review_commit'] == review
    def git(*args): return read(['git', *args], cwd=source, text=True).strip()
    assert git('rev-parse', 'HEAD') == SOURCE
    assert git('rev-parse', 'HEAD^{tree}') == TREE
    assert git('rev-list', '--parents', '-n', '1', 'HEAD').split() == [SOURCE, BASE]
    assert not git('status', '--porcelain', '--untracked-files=no')
    with tempfile.TemporaryDirectory(prefix='nuvio-synopsis-source-auth-', dir=os.environ['RUNNER_TEMP']) as temporary:
        askpass = Path(temporary) / 'askpass.sh'
        askpass.write_text('#!/bin/sh\ncase "$1" in\n'
                           '  *Username*) printf "%s\\n" x-access-token;;\n'
                           '  *Password*) printf "%s\\n" "$GH_TOKEN";;\n'
                           '  *) exit 1;;\nesac\n')
        askpass.chmod(0o700)
        env = dict(os.environ, GIT_ASKPASS=str(askpass), GIT_TERMINAL_PROMPT='0')
        # No credential-bearing URL, trace, shell body or force option.
        for key in tuple(env):
            if key.startswith(('GIT_TRACE', 'GIT_CURL_VERBOSE')): env.pop(key)
        def remote():
            result = run(['git', 'ls-remote', '--exit-code', REMOTE, REF], cwd=source,
                         env=env, text=True, capture_output=True, timeout=60)
            if result.returncode == 2:
                assert not result.stdout.strip()
                return None
            assert result.returncode == 0, 'Could not inspect the dedicated source branch'
            rows = [line.split() for line in result.stdout.splitlines() if line.strip()]
            assert len(rows) == 1 and len(rows[0]) == 2 and rows[0][1] == REF
            assert re.fullmatch(r'[0-9a-f]{40}', rows[0][0])
            return rows[0][0]
        previous = remote()
        assert previous in (None, SOURCE), 'Dedicated source branch already points to a different commit; nothing changed'
        if previous is None:
            result = run(['git', '-c', 'push.followTags=false', 'push', '--porcelain',
                          REMOTE, 'HEAD:' + REF], cwd=source, env=env, text=True,
                         capture_output=True, timeout=120)
            assert result.returncode == 0, 'Normal source-branch push failed; no force or overwrite attempted'
        assert remote() == SOURCE, 'Published source branch did not confirm the exact reviewed commit'
    receipt = {'outcome': 'passed', 'review_commit': review, 'source_commit': SOURCE,
               'source_tree': TREE, 'source_parent': BASE, 'repository': REPO, 'ref': REF,
               'created_new_ref': previous is None, 'already_exact': previous == SOURCE,
               'required_successful_jobs': ['native', 'fontepref', 'fontepref_file', 'player_retido', 'android', 'all_gates'],
               'normal_non_force_push_only': True, 'pull_request_or_release_created': False}
    output = Path(output); output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(receipt, indent=2) + '\n')
    print('Verified green product source branch:', REF, SOURCE)
    return receipt

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--proof', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    publish(args.source, args.proof, args.output)
