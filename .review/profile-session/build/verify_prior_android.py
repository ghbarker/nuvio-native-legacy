#!/usr/bin/env python3
"""Reuse a successful standard APK build only for the identical product source."""
import argparse
import json
import os
from pathlib import Path
import urllib.request

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--artifact', type=Path, required=True)
    parser.add_argument('--manifest', type=Path, required=True)
    parser.add_argument('--prior-head', required=True)
    parser.add_argument('--job-id', type=int, required=True)
    parser.add_argument('--run-id', type=int, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    plan = json.loads(args.manifest.read_text())
    source = json.loads((args.artifact / 'standalone-source-verified.json').read_text())
    assert source['verified'] is True and source['build_commit'] == args.prior_head
    assert source['base'] == plan['base'] and source['feature_commit'] == plan['feature_commit']
    expected = {entry['path']: (entry['sha'], entry['mode']) for entry in plan['files']}
    observed = {entry['path']: (entry['sha'], entry['mode']) for entry in source['files']}
    assert len(expected) == len(plan['files']) == len(observed) == len(source['files'])
    assert expected == observed and set(source['feature_changed_paths']) == set(expected)
    endpoint = ('https://api.github.com/repos/' + os.environ['GITHUB_REPOSITORY'] +
                '/actions/jobs/' + str(args.job_id))
    request = urllib.request.Request(endpoint, headers={
        'Authorization': 'Bearer ' + os.environ['GH_API_TOKEN'],
        'Accept': 'application/vnd.github+json', 'X-GitHub-Api-Version': '2022-11-28'})
    with urllib.request.urlopen(request, timeout=30) as response:
        job = json.load(response)
    assert job['run_id'] == args.run_id and job['head_sha'] == args.prior_head
    assert job['status'] == 'completed' and job['conclusion'] == 'success'
    required = {'Verify exact clean latest-upstream source and product manifest',
                'Standard production-tool debug APK',
                'Verify standard package and both ARM architectures'}
    steps = {step['name']: step for step in job['steps']}
    assert required <= set(steps)
    assert all(steps[name]['status'] == 'completed' and steps[name]['conclusion'] == 'success'
               for name in required)
    proof = {'verified': True, 'base': plan['base'], 'feature_commit': plan['feature_commit'],
             'source_paths': len(expected), 'prior_review_commit': args.prior_head,
             'prior_run': args.run_id, 'android_job': args.job_id,
             'standard_apk_both_arm_and_page_check': 'PASS',
             'followup_review_commit': os.environ['GITHUB_SHA']}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(proof, indent=2) + '\n')
    print('Prior successful standard APK/both-ARM job verified against every unchanged source blob')

if __name__ == '__main__':
    main()
