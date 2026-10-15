#!/usr/bin/env python3
import json,os,subprocess
from pathlib import Path
plan=json.loads(Path(__file__).with_name('plans.json').read_text());proof=json.loads(Path('delivery/validation.json').read_text())
assert proof['outcome']=='passed' and proof['source_commit']==os.environ['PLANNED_SOURCE'] and proof['review_commit']==os.environ['GITHUB_SHA']
payload={'ref':'refs/tags/'+plan['release']['tag'],'sha':proof['source_commit']}
r=subprocess.run(['gh','api','--method','POST','repos/'+os.environ['GH_REPO']+'/git/refs','--input','-'],input=json.dumps(payload),text=True,capture_output=True)
assert r.returncode==0,'Creating unique source tag failed; no existing tags/assets overwritten'
result=json.loads(r.stdout);assert result['ref']==payload['ref'] and result['object']['sha']==proof['source_commit']
Path('delivery/release-tag.json').write_text(json.dumps({'created_new_ref':True,**payload},indent=2)+'\n')
