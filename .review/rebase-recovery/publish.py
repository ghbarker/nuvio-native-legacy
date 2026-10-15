#!/usr/bin/env python3
"""Publish unchanged tested PR heads after the reviewed unrelated upstream advance."""
from pathlib import Path, PurePosixPath
import argparse, hashlib, importlib.util, json, os, subprocess, sys, tempfile
import urllib.request, urllib.error, zipfile

HERE = Path(__file__).resolve().parent

def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def git(source, *args, env=None):
    return subprocess.check_output(['git', '-C', str(source), *args], env=env, text=True).strip()

def api(path, token):
    request = urllib.request.Request('https://api.github.com/repos/' + path,
        headers={'Authorization': 'Bearer ' + token, 'Accept': 'application/vnd.github+json',
                 'X-GitHub-Api-Version': '2022-11-28'})
    with urllib.request.urlopen(request, timeout=60) as response:
        return json.load(response)

def collection(repo, path, key, token):
    rows = []
    for page in range(1, 10):
        data = api(repo + '/' + path + ('&' if '?' in path else '?') +
                   'per_page=100&page=' + str(page), token)
        current = data[key]; rows += current
        if len(current) < 100:
            return rows
    raise AssertionError('Unexpected API pagination overflow')

def verify_origin(lock, entry, token):
    repo, run_id = lock['repository'], entry['origin_run']
    run = api(repo + '/actions/runs/' + str(run_id), token)
    assert run['id'] == run_id and run['head_sha'] == entry['origin_commit']
    assert run['head_branch'] == entry['origin_branch'] and run['run_attempt'] == entry['origin_attempt']
    assert run['status'] == 'completed' and run['conclusion'] == 'failure'
    assert run['repository']['full_name'] == repo
    jobs = collection(repo, 'actions/runs/' + str(run_id) + '/jobs?filter=latest', 'jobs', token)
    expected = entry['successful_jobs'] + [entry['failed_publisher']]
    assert {j['id'] for j in jobs} == {j['id'] for j in expected}
    actual = {j['id']: j for j in jobs}
    for pinned in entry['successful_jobs']:
        job = actual[pinned['id']]
        assert job['name'] == pinned['name'] and job['run_id'] == run_id
        assert job['status'] == 'completed' and job['conclusion'] == 'success'
    failed = actual[entry['failed_publisher']['id']]
    assert failed['name'] == entry['failed_publisher']['name'] and failed['conclusion'] == 'failure'
    artifacts = collection(repo, 'actions/runs/' + str(run_id) + '/artifacts', 'artifacts', token)
    for pinned in entry['artifacts']:
        matches = [a for a in artifacts if a['id'] == pinned['id']]
        assert len(matches) == 1
        artifact = matches[0]
        assert all(artifact[k] == pinned[k] for k in ['id', 'name', 'size_in_bytes', 'digest'])
        assert not artifact['expired'] and artifact['workflow_run']['id'] == run_id
        assert artifact['workflow_run']['head_sha'] == entry['origin_commit']
    return {'run_id': run_id, 'review_commit': entry['origin_commit'],
            'successful_jobs': entry['successful_jobs'], 'failed_publisher_preserved': entry['failed_publisher']}

def download_json_artifacts(lock, entry, destination, token):
    destination.mkdir(parents=True, exist_ok=False)
    for pinned in entry['artifacts']:
        request = urllib.request.Request('https://api.github.com/repos/' + lock['repository'] +
            '/actions/artifacts/' + str(pinned['id']) + '/zip',
            headers={'Authorization': 'Bearer ' + token, 'Accept': 'application/vnd.github+json'})
        class NoRedirect(urllib.request.HTTPRedirectHandler):
            def redirect_request(self, request, file, code, message, headers, new_url):
                return None
        try:
            urllib.request.build_opener(NoRedirect).open(request, timeout=60)
            raise AssertionError('Expected signed artifact redirect')
        except urllib.error.HTTPError as error:
            assert error.code == 302
            signed_url = error.headers['Location']; assert signed_url.startswith('https://')
        # The temporary blob request has no GitHub authorization header.
        # Hash the full original archive, then retain JSON evidence only. No APK is copied into this recovery artifact.
        with tempfile.TemporaryFile() as archive:
            digest, count = hashlib.sha256(), 0
            with urllib.request.urlopen(signed_url, timeout=120) as response:
                while True:
                    data = response.read(1024 * 1024)
                    if not data: break
                    archive.write(data); digest.update(data); count += len(data)
            assert count == pinned['size_in_bytes'] and 'sha256:' + digest.hexdigest() == pinned['digest']
            archive.seek(0)
            with zipfile.ZipFile(archive) as zipped:
                names = zipped.namelist(); assert len(names) == len(set(names))
                for info in zipped.infolist():
                    relative = PurePosixPath(info.filename)
                    assert not relative.is_absolute() and '..' not in relative.parts
                    if info.is_dir() or relative.suffix != '.json': continue
                    assert info.file_size < 20 * 1024 * 1024
                    target = destination / pinned['name'] / str(relative)
                    target.parent.mkdir(parents=True, exist_ok=True)
                    assert not target.exists(); target.write_bytes(zipped.read(info))

def materialize(entry, origin, source, proof):
    recipe = origin / entry['recipe']
    assert git(origin, 'rev-parse', 'HEAD') == entry['origin_commit']
    assert sha(recipe / 'plan.json') == entry['plan_sha256']
    env = dict(os.environ, GITHUB_SHA=entry['origin_commit'])
    env[entry['plan_env']] = entry['plan_sha256']
    if entry['recipe'] == '.review/pr431-exact':
        command = [sys.executable, str(recipe / 'materialize.py'), '--output', str(source),
                   '--proof', str(proof), '--base-repository', str(origin)]
    else:
        command = [sys.executable, str(recipe / 'materialize.py'), '--target', str(source), '--proof', str(proof)]
    subprocess.run(command, cwd=origin, env=env, check=True)
    value = json.loads(proof.read_text())
    assert value['verified'] and value['source_commit'] == entry['source'] and value['tree'] == entry['tree']
    assert value['review_commit'] == entry['origin_commit'] and value['manifest_sha256'] == entry['manifest_sha256']
    return recipe

def verify_evidence(entry, recipe, source, proof, evidence):
    if entry['recipe'] == '.review/pr431-exact':
        # Run the exact original receipt validator, without entering its publication main().
        sys.path.insert(0, str(recipe))
        spec = importlib.util.spec_from_file_location('original_pr431_publisher', recipe / 'publish.py')
        module = importlib.util.module_from_spec(spec); spec.loader.exec_module(module)
        previous = os.environ.get('GITHUB_SHA'); os.environ['GITHUB_SHA'] = entry['origin_commit']
        try:
            module.verify(source, proof)
            records = module.verify_evidence(evidence, entry['origin_commit'])
        finally:
            if previous is None: os.environ.pop('GITHUB_SHA', None)
            else: os.environ['GITHUB_SHA'] = previous
        return {'validator': 'unchanged original PR431 verify_evidence', 'records': records}
    files = list(evidence.glob('*/all-gates-verified.json')); assert len(files) == 1
    value = json.loads(files[0].read_text())
    assert value['outcome'] == 'passed' and value['review_commit'] == entry['origin_commit']
    assert int(value['run_id']) == entry['origin_run'] and value['run_attempt'] == entry['origin_attempt']
    assert value['source_commit'] == entry['source'] and value['source_tree'] == entry['tree']
    assert value['source_manifest_sha256'] == entry['manifest_sha256'] and value['plan_sha256'] == entry['plan_sha256']
    groups = {'native', 'fontepref', 'fontepref_file', 'player_retido', 'android'}
    assert set(value['required_jobs']) == groups and set(value['verified_groups']) == groups
    assert all(x['result'] == 'success' for x in value['required_jobs'].values())
    assert value['verified_groups']['native']['full_apps'] == 2 and value['verified_groups']['android']['abis'] == 2
    for suite in groups - {'native', 'android'}:
        assert value['verified_groups'][suite]['modes'] == ['normal', 'asan']
    assert value['artifact_inputs_sha256'] and value['release_or_pr_created'] is False
    return {'validator': 'successful immutable original aggregate', 'file': str(files[0].relative_to(evidence)),
            'sha256': sha(files[0]), 'verified_groups': value['verified_groups']}

def verify_merge(lock, entry, source, upstream, fetch=True):
    if fetch:
        subprocess.run(['git', '-C', str(source), 'fetch', '--no-tags',
                        'https://github.com/' + lock['upstream_repository'] + '.git', upstream], check=True)
    assert git(source, 'rev-parse', 'HEAD') == entry['source'] and git(source, 'rev-parse', 'HEAD^{tree}') == entry['tree']
    assert not git(source, 'status', '--porcelain')
    assert git(source, 'rev-list', '--reverse', lock['base'] + '..HEAD').splitlines() == entry['chain']
    assert git(source, 'merge-base', upstream, entry['source']) == lock['base']
    upstream_paths = git(source, 'diff', '--name-only', lock['base'], upstream).splitlines()
    feature_paths = git(source, 'diff', '--name-only', lock['base'], entry['source']).splitlines()
    assert len(feature_paths) == entry['feature_path_count']
    overlap = sorted(set(upstream_paths).intersection(feature_paths))
    approved = entry['reviewed_upstream_overlap_blobs']
    for path in overlap:
        assert path in approved and git(source, 'rev-parse', upstream + ':' + path) == approved[path], \
            'New or changed upstream overlap requires semantic review: ' + path
    # A successful merge-tree must return exactly one tree and no conflict records.
    merged = git(source, 'merge-tree', '--write-tree', upstream, entry['source'])
    assert len(merged) == 40 and set(merged) <= set('0123456789abcdef')
    if upstream == lock['reviewed_upstream']:
        assert merged == entry['reviewed_merge_tree'], 'Reviewed clean merge tree changed'
    return {'upstream': upstream, 'merge_base': lock['base'], 'clean_merge_tree': merged,
            'upstream_paths': upstream_paths, 'feature_path_count': len(feature_paths),
            'overlap': overlap, 'reviewed_overlap_blobs': {path: approved[path] for path in overlap}}

def check_live_refs(lock, entry, pr, token):
    latest = api(lock['upstream_repository'] + '/git/ref/heads/master', token)
    request = api(lock['upstream_repository'] + '/pulls/' + str(pr), token)
    assert request['state'] == 'open' and not request['merged']
    assert request['head']['repo']['full_name'] == lock['repository'] and request['head']['ref'] == entry['branch']
    assert request['head']['sha'] in [entry['old'], entry['source']], 'Existing PR branch lease changed'
    return latest['object']['sha'], request['head']['sha']

def publish(lock, entry, source, token, checked_upstream):
    remote, ref = 'https://github.com/' + lock['repository'] + '.git', 'refs/heads/' + entry['branch']
    with tempfile.TemporaryDirectory(prefix='pr-rebase-recovery-auth-', dir=os.environ['RUNNER_TEMP']) as temporary:
        askpass = Path(temporary) / 'askpass.py'
        askpass.write_text('#!/usr/bin/env python3\nimport os,sys\nprint("x-access-token" if "username" in sys.argv[1].lower() else os.environ["GH_TOKEN"])\n')
        askpass.chmod(0o700)
        env = dict(os.environ, GH_TOKEN=token, GIT_ASKPASS=str(askpass), GIT_TERMINAL_PROMPT='0')
        for key in tuple(env):
            if key.startswith(('GIT_TRACE', 'GIT_CURL_VERBOSE')): env.pop(key)
        previous = git(source, 'ls-remote', '--exit-code', remote, ref, env=env).split()
        assert previous in [[entry['old'], ref], [entry['source'], ref]], 'Exact old branch lease changed'
        upstream = git(source, 'ls-remote', '--exit-code',
                       'https://github.com/' + lock['upstream_repository'] + '.git', 'refs/heads/master', env=env).split()
        assert upstream == [checked_upstream, 'refs/heads/master'], 'Upstream changed during the final leased push guard'
        if previous[0] == entry['old']:
            git(source, '-c', 'push.followTags=false', 'push', '--porcelain',
                '--force-with-lease=' + ref + ':' + entry['old'], remote, entry['source'] + ':' + ref, env=env)
        assert git(source, 'ls-remote', '--exit-code', remote, ref, env=env).split() == [entry['source'], ref]
    return {'previous_head': previous[0], 'new_head': entry['source'], 'lease': entry['old'],
            'branch': entry['branch'], 'exact_force_with_lease_only': True}

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--pr', choices=['431', '526', '527'], required=True)
    parser.add_argument('--origin', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--check-only', action='store_true')
    args = parser.parse_args(); lock_path = HERE / 'lock.json'
    assert sha(lock_path) == os.environ['RECOVERY_LOCK_SHA256']
    lock = json.loads(lock_path.read_text()); entry = lock['prs'][args.pr]
    assert os.environ['GITHUB_REPOSITORY'] == lock['repository']
    assert os.environ['GITHUB_REF'] == 'refs/heads/' + lock['review_branch']
    token = os.environ['GH_TOKEN']; args.output.mkdir(parents=True, exist_ok=False)
    origin = args.origin.resolve(); source = args.output.resolve() / 'source'; proof = args.output.resolve() / 'source-verified.json'
    origins = verify_origin(lock, entry, token)
    upstream, previous = check_live_refs(lock, entry, args.pr, token)
    recipe = materialize(entry, origin, source, proof)
    evidence = args.output / 'evidence'; download_json_artifacts(lock, entry, evidence, token)
    gates = verify_evidence(entry, recipe, source, proof, evidence)
    merge = verify_merge(lock, entry, source, upstream)
    # Follow unrelated upstream commits by checking their actual merge, without rewriting source or repeating platform checks.
    for attempt in range(3):
        latest, head = check_live_refs(lock, entry, args.pr, token)
        assert head == previous, 'Existing PR branch changed during evidence verification'
        if latest == merge['upstream']: break
        merge = verify_merge(lock, entry, source, latest)
    else:
        raise AssertionError('Upstream kept changing during bounded clean-merge checks')
    publication = None if args.check_only else publish(lock, entry, source, token, merge['upstream'])
    receipt = {'outcome': 'checked' if args.check_only else 'passed', 'pr': int(args.pr),
               'recovery_review_commit': os.environ['GITHUB_SHA'], 'recovery_run_id': os.environ['GITHUB_RUN_ID'],
               'lock_sha256': sha(lock_path), 'source_commit': entry['source'], 'source_tree': entry['tree'],
               'source_base': lock['base'], 'original_checks': origins, 'original_evidence': gates,
               'reviewed_upstream_merge': merge, 'publication': publication,
               'source_rewritten': False, 'platform_checks_repeated': False,
               'product_tests_workflows_private_overlay_added': False, 'pr_or_release_created': False}
    (args.output / 'publication-recovery-verified.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print('Verified unchanged source publication:', args.pr, entry['source'])

if __name__ == '__main__':
    main()
