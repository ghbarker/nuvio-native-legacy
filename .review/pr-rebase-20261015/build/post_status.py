#!/usr/bin/env python3
"""Attach success only to the exact three candidates in the verified verdict."""
import json
import os
from pathlib import Path
import urllib.request

receipt = json.loads(Path('all-proofs/all-required-gates-verified.json').read_text())
assert receipt['outcome'] == 'passed' and receipt['review_commit'] == os.environ['GITHUB_SHA']
assert len(receipt['cases']) == 3 and receipt['required_gate_count'] == len(receipt['required_gates'])
repo = os.environ['GITHUB_REPOSITORY']
assert repo == 'ghbarker/nuvio-native-legacy'
url = os.environ['GITHUB_SERVER_URL'] + '/' + repo + '/actions/runs/' + os.environ['GITHUB_RUN_ID']
headers = {'Authorization': 'Bearer ' + os.environ['GH_TOKEN'], 'Accept': 'application/vnd.github+json',
           'X-GitHub-Api-Version': '2022-11-28', 'Content-Type': 'application/json'}
for case in receipt['cases']:
    body = json.dumps({'state': 'success', 'target_url': url, 'context': 'fork/rebased-validation',
        'description': 'Exact PR source: TV/Mobile, ARM32/64 and required regressions passed'}).encode()
    request = urllib.request.Request('https://api.github.com/repos/' + repo + '/statuses/' + case['source_commit'],
        data=body, headers=headers, method='POST')
    with urllib.request.urlopen(request, timeout=30) as response:
        result = json.load(response)
    assert result['state'] == 'success' and result['context'] == 'fork/rebased-validation'
    assert result['target_url'] == url
    print('Verified-source status published:', case['id'], case['source_commit'])
