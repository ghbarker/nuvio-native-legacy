#!/usr/bin/env python3
"""Publish an unchanged candidate after verifying and replaying its original green proof."""
from pathlib import Path, PurePosixPath
import argparse, hashlib, json, os, re, stat, subprocess, sys, tempfile
import urllib.request, urllib.error, zipfile

HERE = Path(__file__).resolve().parent
REPOSITORY = 'ghbarker/nuvio-native-legacy'
CANDIDATE_REF = 'refs/heads/candidate/pr527-startup-identity-16194b4-20261015'
EXISTING_PR_REF = 'refs/heads/feat/collection-view-default-362-20261015'

def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def git(root, *args, env=None):
    return subprocess.check_output(['git', '-C', str(root), *args], text=True, env=env).strip()

def api(path, token):
    request = urllib.request.Request('https://api.github.com/repos/' + REPOSITORY + '/' + path,
        headers={'Authorization': 'Bearer ' + token, 'Accept': 'application/vnd.github+json',
                 'X-GitHub-Api-Version': '2022-11-28'})
    with urllib.request.urlopen(request, timeout=60) as response:
        return json.load(response)

def collection(path, key, token):
    rows = []
    for page in range(1, 10):
        data = api(path + ('&' if '?' in path else '?') + 'per_page=100&page=' + str(page), token)
        current = data[key]; rows.extend(current)
        if len(current) < 100: return rows
    raise AssertionError('Unexpected pagination overflow')

def approved():
    path = HERE / 'plan.json'; plan = json.loads(path.read_text())
    assert sha(path) == os.environ['IDENTITY_PUBLICATION_PLAN_SHA256']
    assert all(plan[k] is True for k in ['frozen', 'root_reviewed', 'ci_execution_permitted', 'origin_evidence_final'])
    assert plan['repository'] == REPOSITORY and plan['candidate_ref'] == CANDIDATE_REF
    assert CANDIDATE_REF != EXISTING_PR_REF
    assert sha(HERE / 'publish.py') == plan['publisher_sha256']
    assert os.environ['GITHUB_REPOSITORY'] == REPOSITORY
    assert os.environ['GITHUB_REF'] == 'refs/heads/' + plan['review_branch']
    assert re.fullmatch('[0-9a-f]{40}', os.environ['GITHUB_SHA'])
    assert set(plan['jobs']) == {'native', 'fontepref', 'fontepref_file', 'player_retido', 'android', 'all_gates'}
    assert set(plan['artifacts']) == set(plan['jobs'])
    for entry in list(plan['jobs'].values()) + list(plan['artifacts'].values()):
        assert isinstance(entry['id'], int) and entry['id'] > 0, 'Actual origin pins are not final'
    for entry in plan['artifacts'].values():
        assert re.fullmatch('sha256:[0-9a-f]{64}', entry['digest'])
    return plan

def verify_origin(plan, token):
    run = api('actions/runs/' + str(plan['origin_run_id']), token)
    assert run['id'] == plan['origin_run_id'] and run['head_sha'] == plan['origin_review_commit']
    assert run['head_branch'] == plan['origin_branch'] and run['run_attempt'] == plan['origin_attempt']
    assert run['status'] == 'completed' and run['conclusion'] == 'success'
    assert run['repository']['full_name'] == REPOSITORY
    jobs = collection('actions/runs/' + str(run['id']) + '/jobs?filter=latest', 'jobs', token)
    assert {j['id'] for j in jobs} == {j['id'] for j in plan['jobs'].values()}
    observed = {j['id']: j for j in jobs}
    for pinned in plan['jobs'].values():
        job = observed[pinned['id']]
        assert job['name'] == pinned['name'] and job['run_id'] == run['id']
        assert job['status'] == 'completed' and job['conclusion'] == 'success'
    artifacts = collection('actions/runs/' + str(run['id']) + '/artifacts', 'artifacts', token)
    for pinned in plan['artifacts'].values():
        matches = [a for a in artifacts if a['id'] == pinned['id']]; assert len(matches) == 1
        actual = matches[0]
        assert all(actual[k] == pinned[k] for k in ['id', 'name', 'size_in_bytes', 'digest'])
        assert not actual['expired'] and actual['workflow_run']['id'] == run['id']
        assert actual['workflow_run']['head_sha'] == plan['origin_review_commit']
    return {'run_id': run['id'], 'attempt': run['run_attempt'], 'review_commit': run['head_sha'],
            'status': run['status'], 'conclusion': run['conclusion'], 'jobs': plan['jobs']}

def download(pinned, target, token):
    request = urllib.request.Request('https://api.github.com/repos/' + REPOSITORY +
        '/actions/artifacts/' + str(pinned['id']) + '/zip',
        headers={'Authorization': 'Bearer ' + token, 'Accept': 'application/vnd.github+json'})
    class NoRedirect(urllib.request.HTTPRedirectHandler):
        def redirect_request(self, request, file, code, message, headers, new_url): return None
    try:
        urllib.request.build_opener(NoRedirect).open(request, timeout=60)
        raise AssertionError('Expected signed artifact redirect')
    except urllib.error.HTTPError as error:
        assert error.code == 302
        signed = error.headers['Location']; assert signed.startswith('https://')
    target.mkdir(parents=True, exist_ok=False)
    with tempfile.TemporaryFile() as archive:
        digest, count = hashlib.sha256(), 0
        # The signed blob request deliberately has no GitHub authorization header.
        with urllib.request.urlopen(signed, timeout=120) as response:
            while True:
                data = response.read(1024 * 1024)
                if not data: break
                digest.update(data); archive.write(data); count += len(data)
        assert count == pinned['size_in_bytes'] and 'sha256:' + digest.hexdigest() == pinned['digest']
        archive.seek(0)
        with zipfile.ZipFile(archive) as zipped:
            names = zipped.namelist(); assert len(names) == len(set(names))
            for entry in zipped.infolist():
                relative = PurePosixPath(entry.filename)
                assert not relative.is_absolute() and '..' not in relative.parts
                assert not stat.S_ISLNK(entry.external_attr >> 16)
                output = target / str(relative)
                if entry.is_dir(): output.mkdir(parents=True, exist_ok=True); continue
                output.parent.mkdir(parents=True, exist_ok=True)
                assert not output.exists()
                with zipped.open(entry) as source, output.open('wb') as destination:
                    while True:
                        data = source.read(1024 * 1024)
                        if not data: break
                        destination.write(data)
    return {k: pinned[k] for k in ['id', 'name', 'size_in_bytes', 'digest']}

def verify_recipe(plan, root):
    assert git(root, 'rev-parse', plan['origin_review_commit'] + '^{tree}') == plan['origin_review_tree']
    allowed = {'.review/pr527-identity-publisher/plan.json', '.review/pr527-identity-publisher/publish.py',
               '.github/workflows/pr527-identity-source-publisher.yml'}
    assert set(git(root, 'diff', '--name-only', plan['origin_review_commit'], 'HEAD').splitlines()) == allowed
    recipe = root / plan['origin_recipe']
    assert sha(recipe / 'plan.json') == plan['origin_plan_sha256']
    original = json.loads((recipe / 'plan.json').read_text())
    assert original['frozen'] and original['root_reviewed'] and original['ci_execution_permitted']
    assert original['source_commit'] == plan['source_commit'] and original['source_tree'] == plan['source_tree']
    assert original['source_ref_after_success'] is False and original['source_ref_update_mode'] == 'none-validation-only'
    for name, digest in original['recipe_inputs'].items(): assert sha(recipe / name) == digest, name
    assert sha(recipe / 'source-manifest.json') == plan['source_manifest_sha256']
    return recipe

def origin_env(plan):
    return dict(os.environ, GITHUB_SHA=plan['origin_review_commit'], GITHUB_RUN_ID=str(plan['origin_run_id']),
                GITHUB_RUN_ATTEMPT=str(plan['origin_attempt']), COLLECTION_PLAN_SHA256=plan['origin_plan_sha256'])

def materialize(plan, root, recipe, source, proof):
    env = origin_env(plan)
    subprocess.run([sys.executable, str(recipe / 'materialize.py'), '--target', str(source),
                    '--proof', str(proof)], cwd=root, env=env, check=True)
    value = json.loads(proof.read_text())
    assert value['verified'] and value['product_only'] and value['exact_two_commit_chain']
    assert (value['source_commit'], value['tree'], value['source_parent'], value['source_parent_tree'], value['base']) == \
           (plan['source_commit'], plan['source_tree'], plan['source_parent'], plan['source_parent_tree'], plan['source_base'])
    assert value['review_commit'] == plan['origin_review_commit'] and value['manifest_sha256'] == plan['source_manifest_sha256']
    assert git(source, 'rev-parse', 'HEAD') == plan['source_commit'] and not git(source, 'status', '--porcelain')
    return value

def replay(plan, recipe, incoming, aggregate, output):
    original = json.loads(aggregate.read_text())
    assert original['outcome'] == 'passed' and original['review_commit'] == plan['origin_review_commit']
    assert int(original['run_id']) == plan['origin_run_id'] and original['run_attempt'] == plan['origin_attempt']
    assert original['source_commit'] == plan['source_commit'] and original['source_tree'] == plan['source_tree']
    assert original['source_manifest_sha256'] == plan['source_manifest_sha256'] and original['plan_sha256'] == plan['origin_plan_sha256']
    required = {'native', 'fontepref', 'fontepref_file', 'player_retido', 'android'}
    assert set(original['required_jobs']) == required and all(v['result'] == 'success' for v in original['required_jobs'].values())
    env = origin_env(plan); env['REQUIRED_JOB_RESULTS'] = json.dumps(original['required_jobs'])
    subprocess.run([sys.executable, str(recipe / 'aggregate.py'), '--input', str(incoming),
                    '--output', str(output)], env=env, check=True)
    assert json.loads(output.read_text()) == original, 'Original aggregate replay differs'
    return {'original_sha256': sha(aggregate), 'replayed_sha256': sha(output), 'parsed_json_equal': True,
            'required_jobs': original['required_jobs'], 'verified_groups': original['verified_groups']}

def publish(plan, source, token):
    remote = 'https://github.com/' + REPOSITORY + '.git'
    with tempfile.TemporaryDirectory(prefix='pr527-identity-auth-', dir=os.environ['RUNNER_TEMP']) as temporary:
        askpass = Path(temporary) / 'askpass.py'
        askpass.write_text('#!/usr/bin/env python3\nimport os,sys\nprint("x-access-token" if "username" in sys.argv[1].lower() else os.environ["GH_TOKEN"])\n')
        askpass.chmod(0o700)
        env = dict(os.environ, GH_TOKEN=token, GIT_ASKPASS=str(askpass), GIT_TERMINAL_PROMPT='0')
        for key in tuple(env):
            if key.startswith(('GIT_TRACE', 'GIT_CURL_VERBOSE')): env.pop(key)
        def ref(name):
            result = subprocess.run(['git', '-C', str(source), 'ls-remote', '--exit-code', remote, name],
                                    env=env, text=True, capture_output=True, timeout=60)
            if result.returncode == 2: assert not result.stdout.strip(); return None
            assert result.returncode == 0
            rows = result.stdout.split(); assert len(rows) == 2 and rows[1] == name
            assert re.fullmatch('[0-9a-f]{40}', rows[0]); return rows[0]
        pr_before = ref(EXISTING_PR_REF)
        previous = ref(CANDIDATE_REF)
        assert previous in [None, plan['source_commit']], 'Candidate branch already holds a different commit'
        if previous is None:
            subprocess.run(['git', '-C', str(source), '-c', 'push.followTags=false', 'push', '--porcelain',
                            remote, plan['source_commit'] + ':' + CANDIDATE_REF], env=env, check=True, timeout=120)
        assert ref(CANDIDATE_REF) == plan['source_commit']
        pr_after = ref(EXISTING_PR_REF)
    return {'ref': CANDIDATE_REF, 'previous_head': previous, 'published_head': plan['source_commit'],
            'normal_non_force_push_only': True, 'existing_pr_ref': EXISTING_PR_REF,
            'existing_pr_head_before': pr_before, 'existing_pr_head_after': pr_after, 'existing_pr_branch_writes': False}

def main():
    parser = argparse.ArgumentParser(); parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args(); plan = approved(); root = Path(os.environ['GITHUB_WORKSPACE']).resolve()
    recipe = verify_recipe(plan, root); token = os.environ['GH_TOKEN']
    args.output.mkdir(parents=True, exist_ok=False); output = args.output.resolve()
    origin = verify_origin(plan, token)
    with tempfile.TemporaryDirectory(prefix='pr527-identity-publish-', dir=os.environ['RUNNER_TEMP']) as temporary:
        work = Path(temporary); incoming = work / 'incoming'; incoming.mkdir(); records = {}
        for key, pinned in plan['artifacts'].items():
            target = work / 'aggregate-origin' if key == 'all_gates' else incoming / pinned['name']
            records[key] = download(pinned, target, token)
        aggregate = work / 'aggregate-origin/all-gates-verified.json'
        (output / 'origin-all-gates-verified.json').write_bytes(aggregate.read_bytes())
        verification = replay(plan, recipe, incoming, aggregate, output / 'replayed-all-gates-verified.json')
        source = work / 'source'; proof = output / 'source-verified.json'
        materialize(plan, root, recipe, source, proof)
        publication = publish(plan, source, token)
    receipt = {'outcome': 'passed', 'publisher_review_commit': os.environ['GITHUB_SHA'],
               'publisher_run_id': os.environ['GITHUB_RUN_ID'], 'publisher_plan_sha256': sha(HERE / 'plan.json'),
               'source_commit': plan['source_commit'], 'source_tree': plan['source_tree'],
               'source_parent': plan['source_parent'], 'source_base': plan['source_base'],
               'original_ci': origin, 'original_artifacts': records, 'aggregate_replay': verification,
               'publication': publication, 'original_proofs_retagged': False, 'sdk_or_tests_rerun': False,
               'pr_or_release_created': False, 'existing_pr_branch_updated': False}
    (output / 'source-publication-verified.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print('Published only the exact validated candidate:', CANDIDATE_REF, plan['source_commit'])

if __name__ == '__main__': main()
