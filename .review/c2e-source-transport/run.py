"""Reconstruct reviewed commit objects, then create only seven temporary refs.

No working-tree source checkout, test, build, PR update or force replacement.
The raw commit bytes preserve authors, committers, messages and exact ancestry.
"""
from pathlib import Path
import base64
import hashlib
import json
import os
import re
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
BASE = 'c2e50a9325719ffae85005c0542338f6b44878c3'
REPO = 'ghbarker/nuvio-native-legacy'
BRANCH = 'refs/heads/review/c2e-source-transport-20261016'
HEADS = {
    431: '8979ef16c046a4b976d08dbbede82d81f9888a28',
    538: 'a4cdfd164d28fd374ef328308b38115ec9af096c',
    540: 'a064470826522dcf2b99767635be556b7555d097',
    541: 'ad190469365670c94d4cc3ab40a7aa2dd44c2b66',
    543: 'e2fbaa5667325e15f08b0e53592624ad5996c472',
    546: '6e2912703c8bc00ca9b6459fb1fa7c29f3007cfb',
    547: 'a3c570947a6018bbd1dc58cb8c3fd2a8b7641bdb',
}
REFS = {pr: f'refs/heads/review/c2e-pr{pr}-source-20261016' for pr in HEADS}


def digest(data):
    return hashlib.sha256(data).hexdigest()


def git(*args, data=None, env=None):
    # Never include an authentication header or credential in command arguments.
    result = subprocess.run(['git', *args], input=data, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE, env=env, timeout=180)
    if result.returncode:
        detail = (result.stdout + b'\n' + result.stderr).decode(errors='replace')[-3600:]
        if os.environ.get('GITHUB_TOKEN'):
            detail = detail.replace(os.environ['GITHUB_TOKEN'], '[REDACTED]')
        raise RuntimeError('Git operation failed: ' + args[0] + '\n' +
                           detail)
    return result.stdout


def remote_heads(env):
    data = git('ls-remote', '--heads', 'origin', *REFS.values(), env=env)
    found = {}
    for line in data.decode().splitlines():
        sha, ref = line.split('\t')
        assert re.fullmatch(r'[0-9a-f]{40}', sha) and ref in REFS.values()
        assert ref not in found
        found[ref] = sha
    return found


def main():
    assert os.environ.get('GITHUB_REPOSITORY') == REPO
    assert os.environ.get('GITHUB_EVENT_NAME') == 'push'
    assert os.environ.get('GITHUB_REF') == BRANCH
    actual = os.environ['GITHUB_SHA']
    assert re.fullmatch(r'[0-9a-f]{40}', actual)
    plan_bytes = (HERE / 'plan.json').read_bytes()
    assert digest(plan_bytes) == os.environ['SOURCE_TRANSPORT_PLAN_SHA256']
    plan = json.loads(plan_bytes)
    assert plan['root_reviewed'] is True and plan['execution_authorized'] is True
    assert plan['public_pr_updates_authorized'] is False
    assert plan['base'] == BASE and plan['repository'] == REPO
    assert plan['temporary_refs'] == {str(pr): ref for pr, ref in REFS.items()}
    assert plan['allowed_temporary_before_states'] == [BASE, 'candidate_head']
    assert digest(Path(__file__).read_bytes()) == plan['runner_sha256']
    workflow = Path('.github/workflows/c2e-source-transport.yml').read_bytes()
    normalized = re.sub(rb'(SOURCE_TRANSPORT_PLAN_SHA256: )[0-9a-f]{64}',
                        rb'\g<1>PLAN_SHA_PLACEHOLDER', workflow)
    assert digest(normalized) == plan['workflow_normalized_sha256']
    payload_bytes = (HERE / 'payload.json').read_bytes()
    assert digest(payload_bytes) == plan['payload_sha256']
    payload = json.loads(payload_bytes)
    assert payload['schema'] == 1 and payload['base'] == BASE
    assert git('rev-parse', 'HEAD').decode().strip() == actual
    assert git('rev-parse', 'HEAD^').decode().strip() == BASE
    assert git('rev-parse', '--show-object-format').decode().strip() == 'sha1'
    assert git('rev-parse', BASE + '^{tree}').decode().strip() == payload['base_tree']
    changed = git('diff-tree', '--no-commit-id', '--name-only', '-r', 'HEAD').decode().splitlines()
    allowed = {'.github/workflows/c2e-source-transport.yml',
               '.github/workflows/build-tpk.yml',
               '.review/c2e-source-transport/run.py',
               '.review/c2e-source-transport/cancel_seed_builds.py',
               '.review/c2e-source-transport/plan.json',
               '.review/c2e-source-transport/payload.json'}
    assert set(changed) == allowed
    # Only the transport review tree suppresses the inherited all-push QEMU
    # workflow. Every reconstructed candidate tree remains byte-for-byte exact.
    inherited = '.github/workflows/build-tpk.yml'
    assert plan['review_only_deletions'] == {inherited: '7422520df351f905c448b5a6182a92b330b59766'}
    assert git('rev-parse', BASE + ':' + inherited).decode().strip() == plan['review_only_deletions'][inherited]
    assert not git('ls-tree', '--name-only', 'HEAD', '--', inherited).strip()
    assert not git('status', '--porcelain', '--untracked-files=no').strip()
    cleanup_path = Path('seed-build-cancellation.json')
    cleanup = json.loads(cleanup_path.read_text())
    assert cleanup['passed'] is True and cleanup['actual_review'] == actual
    assert cleanup['allowed_exact_runs'] == plan['seed_build_cancellations']
    assert cleanup['other_runs_cancelled'] is False and cleanup['public_pr_refs_changed'] is False
    assert git('remote', 'get-url', 'origin').decode().strip() in (
        'https://github.com/' + REPO, 'https://github.com/' + REPO + '.git')
    rows = payload['candidates']
    assert len(rows) == len(HEADS) and {r['pr'] for r in rows} == set(HEADS)
    assert sum(len(r['commits']) for r in rows) == plan['commit_count'] == 20
    proof = {'passed': False, 'repository': REPO, 'actual_review': actual,
             'workflow_run_id': os.environ['GITHUB_RUN_ID'],
             'workflow_run_attempt': os.environ['GITHUB_RUN_ATTEMPT'],
             'base': BASE, 'base_tree': payload['base_tree'],
             'plan_sha256': digest(plan_bytes), 'payload_sha256': digest(payload_bytes),
             'seed_cancellation_sha256': digest(cleanup_path.read_bytes()),
             'app_built_or_tested': False, 'public_pr_refs_changed': False,
             'candidates': []}
    # All commit/tree reconstruction must pass before any remote mutation.
    with tempfile.TemporaryDirectory(prefix='c2e-source-', dir=os.environ.get('RUNNER_TEMP')) as td:
        tmp = Path(td)
        index_env = dict(os.environ, GIT_INDEX_FILE=str(tmp / 'index'))
        for row in rows:
            pr = row['pr']
            assert row['head'] == HEADS[pr] and row['temporary_ref'] == REFS[pr]
            previous = BASE
            verified = []
            for entry in row['commits']:
                assert entry['parent'] == previous
                sha = entry['sha']
                assert re.fullmatch(r'[0-9a-f]{40}', sha)
                assert entry['patch_name'] == f'pr{pr}-{sha}.patch'
                raw = base64.b64decode(entry['raw_commit_base64'], validate=True)
                patch = base64.b64decode(entry['patch_base64'], validate=True)
                assert digest(raw) == entry['raw_commit_sha256']
                assert len(patch) == entry['patch_bytes'] and digest(patch) == entry['patch_sha256']
                headers, message = raw.split(b'\n\n', 1)
                parents = [line[7:].decode() for line in headers.splitlines() if line.startswith(b'parent ')]
                trees = [line[5:].decode() for line in headers.splitlines() if line.startswith(b'tree ')]
                assert parents == [previous] and trees == [entry['tree']]
                assert message.decode() == entry['message']
                assert hashlib.sha1(b'commit ' + str(len(raw)).encode() + b'\0' + raw).hexdigest() == sha
                git('read-tree', previous + '^{tree}', env=index_env)
                patch_file = tmp / entry['patch_name']
                patch_file.write_bytes(patch)
                git('apply', '--cached', '--binary', '--whitespace=nowarn', str(patch_file), env=index_env)
                assert git('write-tree', env=index_env).decode().strip() == entry['tree']
                assert git('hash-object', '-t', 'commit', '-w', '--stdin', data=raw).decode().strip() == sha
                observed = git('diff-tree', '--no-commit-id', '--name-only', '-r', sha).decode().splitlines()
                assert observed == entry['changed_paths']
                assert not any(path.startswith('.github/workflows/') for path in observed)
                verified.append({'sha': sha, 'parent': previous, 'tree': entry['tree'],
                                 'raw_commit_sha256': entry['raw_commit_sha256'],
                                 'patch_sha256': entry['patch_sha256']})
                previous = sha
            assert previous == row['head']
            assert git('rev-parse', previous + '^{tree}').decode().strip() == row['tree']
            proof['candidates'].append({'pr': pr, 'head': previous, 'tree': row['tree'],
                                        'temporary_ref': REFS[pr], 'commits': verified})
        # Credentials are read only by this temporary askpass subprocess. They
        # never appear in URLs, Git config, command arguments or proof output.
        assert os.environ.get('GITHUB_TOKEN')
        askpass = tmp / 'askpass.py'
        askpass.write_text('#!/usr/bin/env python3\nimport os,sys\n'
                           'prompt=sys.argv[1].lower()\n'
                           'if "username" in prompt: print("x-access-token")\n'
                           'elif "password" in prompt: print(os.environ["GITHUB_TOKEN"])\n'
                           'else: raise SystemExit(1)\n')
        askpass.chmod(0o700)
        auth_env = dict(os.environ, GIT_ASKPASS=str(askpass), GIT_TERMINAL_PROMPT='0',
                        GIT_CONFIG_NOSYSTEM='1', GIT_CONFIG_GLOBAL=os.devnull)
        before = remote_heads(auth_env)
        assert set(before) == set(REFS.values()), 'Root-seeded temporary refs must all exist'
        updates = []
        for row in proof['candidates']:
            ref, head = row['temporary_ref'], row['head']
            current = before.get(ref)
            assert current in (BASE, head), 'Temporary ref points to unapproved source: ' + ref
            row['publication'] = ('already_exact' if current == head else 'fast_forward_from_root_seed')
            if current != head:
                updates.append((ref, head, current))
        if updates:
            # Root may seed these exact seven temporary refs at BASE so every
            # token update is an ordinary descendant with no workflow changes.
            # Each lease binds its actual observed BASE beforestate.
            git('-c', 'credential.helper=', '-c', 'core.hooksPath=/dev/null',
                '-c', 'push.followTags=false', 'push', '--atomic', '--porcelain',
                *['--force-with-lease=' + ref + ':' + BASE for ref, _, _ in updates],
                'origin', *[head + ':' + ref for ref, head, _ in updates], env=auth_env)
        after = remote_heads(auth_env)
        assert after == {REFS[pr]: HEADS[pr] for pr in HEADS}
        proof['remote_heads_before'] = before
        proof['remote_heads_after'] = after
    assert not git('status', '--porcelain', '--untracked-files=no').strip()
    proof['passed'] = True
    output = Path('source-transport-proof.json')
    output.write_text(json.dumps(proof, indent=2) + '\n')
    print(json.dumps({'passed': True, 'exact_heads': len(rows), 'exact_commits': 20,
                      'proof_sha256': digest(output.read_bytes())}))


if __name__ == '__main__':
    main()
