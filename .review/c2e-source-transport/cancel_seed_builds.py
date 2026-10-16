"""Cancel only seven root-approved, metadata-verified temporary seed builds."""
from pathlib import Path
import hashlib
import json
import os
import subprocess
import time
import urllib.error
import urllib.request

HERE = Path(__file__).resolve().parent
REPO = 'ghbarker/nuvio-native-legacy'
BASE = 'c2e50a9325719ffae85005c0542338f6b44878c3'
RUNS = {
    38940687822: 'review/c2e-pr547-source-20261016',
    38940684909: 'review/c2e-pr546-source-20261016',
    38940682836: 'review/c2e-pr541-source-20261016',
    38940684226: 'review/c2e-pr543-source-20261016',
    38940681149: 'review/c2e-pr540-source-20261016',
    38940679309: 'review/c2e-pr538-source-20261016',
    38940677408: 'review/c2e-pr431-source-20261016',
}


def api(run_id, cancel=False):
    assert run_id in RUNS
    url = 'https://api.github.com/repos/' + REPO + '/actions/runs/' + str(run_id)
    if cancel:
        url += '/cancel'
    req = urllib.request.Request(url, data=b'' if cancel else None,
        method='POST' if cancel else 'GET', headers={
            'Authorization': 'Bearer ' + os.environ['GITHUB_TOKEN'],
            'Accept': 'application/vnd.github+json',
            'X-GitHub-Api-Version': '2022-11-28',
            'User-Agent': 'reviewed-seven-seed-build-cleanup'})
    try:
        with urllib.request.urlopen(req, timeout=30) as response:
            body = response.read(2 * 1024 * 1024 + 1)
            assert len(body) <= 2 * 1024 * 1024
            return response.status, json.loads(body) if body else None
    except urllib.error.HTTPError as error:
        if cancel and error.code in (409, 422):
            return error.code, None  # May have completed after the verified GET.
        raise RuntimeError('Seed build API failed: HTTP ' + str(error.code)) from None


def verified(run_id):
    code, d = api(run_id)
    assert code == 200 and d['id'] == run_id
    assert d['repository']['full_name'] == REPO
    assert d['head_sha'] == BASE and d['head_branch'] == RUNS[run_id]
    assert d['path'] == '.github/workflows/build-tpk.yml' and d['event'] == 'push'
    return {k: d[k] for k in ('id', 'head_sha', 'head_branch', 'path', 'event', 'status', 'conclusion')}


def main():
    assert os.environ['GITHUB_REPOSITORY'] == REPO
    assert os.environ['GITHUB_REF'] == 'refs/heads/review/c2e-source-transport-20261016'
    assert os.environ['GITHUB_EVENT_NAME'] == 'push'
    raw = (HERE / 'plan.json').read_bytes()
    assert hashlib.sha256(raw).hexdigest() == os.environ['SOURCE_TRANSPORT_PLAN_SHA256']
    plan = json.loads(raw)
    assert plan['root_reviewed'] is True and plan['execution_authorized'] is True
    assert plan['public_pr_updates_authorized'] is False
    assert plan['seed_build_cancellations'] == {str(k): v for k, v in RUNS.items()}
    assert hashlib.sha256(Path(__file__).read_bytes()).hexdigest() == plan['cleanup_sha256']
    assert subprocess.check_output(['git', 'rev-parse', 'HEAD^'], text=True).strip() == BASE
    # Validate every allowed ID and its corresponding branch before any POST.
    before = {str(run_id): verified(run_id) for run_id in RUNS}
    requests = []
    for run_id in RUNS:
        if before[str(run_id)]['status'] == 'completed':
            requests.append({'id': run_id, 'action': 'already_completed'})
            continue
        code, _ = api(run_id, cancel=True)
        assert code in (202, 409, 422)
        requests.append({'id': run_id, 'action': 'cancel_requested', 'http_status': code})
    deadline = time.monotonic() + 5
    while True:
        after = {str(run_id): verified(run_id) for run_id in RUNS}
        if all(row['status'] == 'completed' for row in after.values()):
            break
        if time.monotonic() >= deadline:
            break
        time.sleep(2)
    receipt = {'passed': True, 'actual_review': os.environ['GITHUB_SHA'],
        'workflow_run_id': os.environ['GITHUB_RUN_ID'], 'base': BASE,
        'allowed_exact_runs': {str(k): v for k, v in RUNS.items()},
        'cancellation_requests_accepted': True,
        'all_completed': all(row['status'] == 'completed' for row in after.values()),
        'pending_exact_run_ids': [int(k) for k, row in after.items() if row['status'] != 'completed'],
        'before': before, 'requests': requests, 'after': after,
        'public_pr_refs_changed': False, 'other_runs_cancelled': False}
    Path('seed-build-cancellation.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps({'verified_exact_seed_builds': 7, 'cancellation_requests_accepted': True, 'all_completed': receipt['all_completed'], 'pending_exact_run_ids': receipt['pending_exact_run_ids']}))


if __name__ == '__main__':
    main()
