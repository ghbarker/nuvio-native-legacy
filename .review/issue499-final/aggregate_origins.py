"""Validate five exact-source producers under their real immutable origins."""
from pathlib import Path, PurePosixPath
import argparse
import hashlib
import json
import os
import re
import stat
import subprocess
import sys
import zipfile
from common import approved

HERE = Path(__file__).resolve().parent
NAMES = {'native', 'fontepref', 'fontepref_file', 'player_retido', 'android'}


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def read(path):
    return json.loads(Path(path).read_text())


def gh(repository, path):
    assert repository == 'ghbarker/nuvio-native-legacy'
    return json.loads(subprocess.check_output(
        ['gh', 'api', 'repos/' + repository + '/' + path], text=True, timeout=60))


def check_job(value, expected, origin):
    assert value['id'] == expected['job_id'] and value['name'] == expected['job_name']
    assert value['run_id'] == origin['run_id'] and value['head_sha'] == origin['review_commit']
    assert value['status'] == 'completed' and value['conclusion'] == 'success'
    assert value['run_attempt'] == origin['run_attempt']


def download(repository, entry, origin, output):
    value = gh(repository, 'actions/artifacts/' + str(entry['id']))
    assert value['id'] == entry['id'] and value['name'] == entry['name']
    assert value['size_in_bytes'] == entry['bytes'] and not value['expired']
    assert value['digest'] == 'sha256:' + entry['sha256']
    assert value['workflow_run']['id'] == origin['run_id']
    assert value['workflow_run']['head_sha'] == origin['review_commit']
    output.mkdir(parents=True, exist_ok=True)
    archive = output / (entry['name'] + '.zip')
    with archive.open('xb') as stream:
        subprocess.run(['gh', 'api', 'repos/' + repository + '/actions/artifacts/' +
                        str(entry['id']) + '/zip'], stdout=stream, check=True, timeout=180)
    assert archive.stat().st_size == entry['bytes'] and sha(archive) == entry['sha256']
    target = output / entry['name']
    assert not target.exists()
    with zipfile.ZipFile(archive) as zipped:
        infos = zipped.infolist()
        assert len(infos) == len({item.filename for item in infos})
        assert sum(item.file_size for item in infos) < 1024 ** 3
        for item in infos:
            path = PurePosixPath(item.filename)
            assert not path.is_absolute() and '..' not in path.parts
            assert not stat.S_ISLNK(item.external_attr >> 16)
        target.mkdir()
        zipped.extractall(target)
    return target


def verify_recipe(origin, lock):
    recipe = HERE.parent / origin['directory']
    plan = read(recipe / 'plan.json')
    assert sha(recipe / 'plan.json') == origin['plan_sha256']
    assert plan['frozen'] and plan['root_reviewed'] and plan['ci_execution_permitted']
    assert plan['source_commit'] == lock['source_commit']
    assert plan['source_tree'] == lock['source_tree'] and plan['base'] == lock['base']
    assert plan['source_manifest_sha256'] == lock['source_manifest_sha256']
    assert sha(recipe / 'source-manifest.json') == lock['source_manifest_sha256']
    assert subprocess.check_output(['git', 'rev-parse', origin['review_commit'] + '^{tree}'],
                                   text=True).strip() == origin['review_tree']
    checks = dict(plan['recipe_inputs'], **{'plan.json': origin['plan_sha256']})
    for relative, digest in checks.items():
        assert sha(recipe / relative) == digest, relative
        actual = subprocess.check_output(['git', 'show', origin['review_commit'] +
                                          ':.review/issue499/' + relative])
        assert hashlib.sha256(actual).hexdigest() == digest, relative
    assert sha(recipe / 'aggregate.py') == lock['strict_aggregate_sha256']
    return recipe


def validate_selected(output, artifacts, origin, recipe, lock):
    text = (recipe / 'aggregate.py').read_text()
    needle = "names=['native','fontepref','fontepref_file','player_retido','android'];"
    assert text.count(needle) == 1
    selected = text.replace(needle, 'names=' + repr(origin['producer_scopes']) + ';', 1)
    message = "print('PASS all5 actual producer jobs and source-bound checks')"
    assert selected.count(message) == 1
    selected = selected.replace(message, "print('PASS selected issue499 exact-source origin assertions')", 1)
    output.mkdir()
    driver = output / 'selected-strict-validator.py'
    driver.write_text(selected)
    env = dict(os.environ, PYTHONPATH=str(recipe),
               GITHUB_SHA=origin['review_commit'], GITHUB_RUN_ID=str(origin['run_id']),
               GITHUB_RUN_ATTEMPT=str(origin['run_attempt']),
               CACHE_PLAN_SHA256=origin['plan_sha256'],
               REQUIRED_JOB_RESULTS=json.dumps({scope: {'result': 'success'}
                                                for scope in origin['producer_scopes']}))
    proof = output / 'selected-gates.json'
    subprocess.run([sys.executable, str(driver), '--input', str(artifacts),
                    '--output', str(proof)], env=env, check=True, timeout=240)
    value = read(proof)
    assert value['outcome'] == 'passed' and value['review_commit'] == origin['review_commit']
    assert value['source_commit'] == lock['source_commit']
    assert value['source_tree'] == lock['source_tree']
    assert set(value['verified_groups']) == set(origin['producer_scopes'])
    return value, {'origin': origin['directory'], 'sha256': sha(proof),
                   'validator_sha256': sha(driver),
                   'validator_changes': ['Select API-verified successful producer scopes',
                                         'Report selected scope without relabeling execution'],
                   'artifact_assertions_unchanged': True}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    assert sys.flags.optimize == 0
    plan = approved()
    lock = read(HERE / 'origin-lock.json')
    assert plan['source_commit'] == lock['source_commit'] and plan['source_tree'] == lock['source_tree']
    assert lock['frozen'] and lock['root_reviewed']
    assert sha(HERE / 'origin-lock.json') == os.environ['CACHE_ORIGIN_LOCK_SHA256']
    repository = lock['repository']
    assert repository == os.environ['GITHUB_REPOSITORY'] == 'ghbarker/nuvio-native-legacy'
    assert len(lock['origins']) == 2
    scopes = [scope for origin in lock['origins'] for scope in origin['producer_scopes']]
    assert len(scopes) == 5 and set(scopes) == NAMES
    final_review = os.environ['GITHUB_SHA']
    final_run = int(os.environ['GITHUB_RUN_ID'])
    final_attempt = int(os.environ['GITHUB_RUN_ATTEMPT'])
    current = gh(repository, 'actions/runs/' + str(final_run))
    assert current['head_sha'] == final_review and current['head_branch'] == lock['final_branch']
    assert current['run_attempt'] == final_attempt and current['event'] == 'push'
    failed = lock['excluded_failed_native']
    excluded = [failed, *lock['additional_excluded_failed_native']]
    for rejection in excluded:
        rejected = gh(repository, 'actions/jobs/' + str(rejection['job_id']))
        assert rejected['run_id'] == rejection['run_id'] and rejected['head_sha'] == rejection['review_commit']
        assert rejected['status'] == 'completed' and rejected['conclusion'] == 'failure'
    assert not args.output.exists()
    output = args.output.resolve()
    output.mkdir(parents=True)
    origins, groups, validations, inputs = [], {}, [], {}
    for origin in lock['origins']:
        recipe = verify_recipe(origin, lock)
        run = gh(repository, 'actions/runs/' + str(origin['run_id']))
        assert run['id'] == origin['run_id'] and run['head_sha'] == origin['review_commit']
        assert run['head_branch'] == origin['branch'] and run['path'] == origin['workflow_path']
        assert run['event'] == 'push' and run['run_attempt'] == origin['run_attempt']
        assert run['status'] == 'completed' and run['conclusion'] == origin['required_conclusion']
        assert len(origin['producers']) == len(origin['producer_scopes'])
        assert {entry['scope'] for entry in origin['producers']} == set(origin['producer_scopes'])
        artifact_root = output / origin['directory']
        for entry in origin['producers']:
            job = gh(repository, 'actions/jobs/' + str(entry['job_id']))
            check_job(job, entry, origin)
            assert entry['job_id'] not in {rejection['job_id'] for rejection in excluded}
            expected_name = 'issue499-' + entry['scope'] + '-' + origin['review_commit']
            assert entry['artifact']['name'] == expected_name
            download(repository, entry['artifact'], origin, artifact_root)
            origins.append({'scope': entry['scope'], 'run_id': origin['run_id'],
                            'run_attempt': origin['run_attempt'], 'review_commit': origin['review_commit'],
                            'plan_sha256': origin['plan_sha256'], 'job_id': job['id'],
                            'job_conclusion': 'success', 'artifact': entry['artifact'],
                            'reexecuted_for_fixture_repair': entry['scope'] == 'native'})
        value, receipt = validate_selected(output / (origin['directory'] + '-validation'),
                                           artifact_root, origin, recipe, lock)
        groups.update(value['verified_groups'])
        validations.append(receipt)
        inputs[origin['directory']] = value['artifact_inputs_sha256']
    assert set(groups) == NAMES and len(origins) == 5
    result = {'outcome': 'passed', 'review_commit': final_review, 'run_id': str(final_run),
              'run_attempt': final_attempt, 'source_commit': lock['source_commit'],
              'source_tree': lock['source_tree'],
              'source_manifest_sha256': lock['source_manifest_sha256'],
              'origin_lock_sha256': sha(HERE / 'origin-lock.json'),
              'plan_sha256': sha(HERE / 'plan.json'),
              'producer_origins': origins, 'verified_groups': groups,
              'excluded_failed_native': excluded, 'retained_raw_proofs_rewritten': False,
              'strict_aggregate_sha256': lock['strict_aggregate_sha256'],
              'selected_validation': validations, 'artifact_inputs_sha256': inputs,
              'release_or_pr_created': False}
    (output / 'all-gates-verified.json').write_text(json.dumps(result, indent=2) + '\n')
    print('PASS four original issue499 successful producers and fresh native with immutable origin proofs')


if __name__ == '__main__':
    main()
