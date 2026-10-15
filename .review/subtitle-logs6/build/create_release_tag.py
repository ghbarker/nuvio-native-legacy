#!/usr/bin/env python3
"""Atomically create the unique tag only after exact-source final proofs pass."""
import json
import os
from pathlib import Path
import subprocess

source = os.environ['PLANNED_SOURCE']
proof = json.loads(Path('delivery/proof/update-identity.json').read_text())
assert proof['verified'] and proof['phase'] == 'final' and proof['source_commit'] == source
payload = {'ref': 'refs/tags/tv-mobile-test-local-logs-v6', 'sha': source}
created = subprocess.run(['gh', 'api', '--method', 'POST', 'repos/' + os.environ['GH_REPO'] + '/git/refs', '--input', '-'],
                         input=json.dumps(payload), text=True, capture_output=True)
if created.returncode:
    print(created.stderr)
    raise SystemExit('Creating the unique source tag failed. Existing tags and releases are never reused or overwritten.')
result = json.loads(created.stdout)
assert result['ref'] == payload['ref'] and result['object']['sha'] == source and result['object']['type'] == 'commit'
Path('delivery/proof/release-tag.json').write_text(json.dumps({'created_new_ref': True, 'ref': result['ref'], 'source_commit': source}, indent=2) + '\n')
print('Created new exact-source release tag:', result['ref'], source)
