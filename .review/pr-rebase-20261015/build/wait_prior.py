#!/usr/bin/env python3
"""Let original successful gates finish while repaired reduced groups run."""
import json
import os
from pathlib import Path
import time
import urllib.request
repo = os.environ['GITHUB_REPOSITORY']
assert repo == 'ghbarker/nuvio-native-legacy'
headers = {'Authorization': 'Bearer ' + os.environ['GH_TOKEN'], 'Accept': 'application/vnd.github+json', 'X-GitHub-Api-Version': '2022-11-28'}
base = 'https://api.github.com/repos/' + repo + '/actions/runs/38667419517'
end = time.monotonic() + 1800
while True:
    with urllib.request.urlopen(urllib.request.Request(base, headers=headers), timeout=30) as response:
        value = json.load(response)
    assert value['head_sha'] == 'bb8d91167afa69b78a79722e12f40633a81df1c8'
    assert value['run_attempt'] == 1
    if value['status'] == 'completed':
        break
    assert time.monotonic() < end, 'Original validation did not finish within bounded reuse wait'
    print('Waiting for original required gates:', value['status'], flush=True)
    time.sleep(15)
with urllib.request.urlopen(urllib.request.Request(base + '/jobs?per_page=100', headers=headers), timeout=30) as response:
    jobs = json.load(response)
assert jobs['total_count'] == len(jobs['jobs'])
out = Path('build/review')
out.mkdir(parents=True, exist_ok=True)
(out / 'prior-run.json').write_text(json.dumps(value, indent=2) + '\n')
(out / 'prior-jobs.json').write_text(json.dumps(jobs, indent=2) + '\n')
print('Original run terminal; successful exact-source evidence can now be verified')
