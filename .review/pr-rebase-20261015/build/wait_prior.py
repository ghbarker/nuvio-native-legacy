#!/usr/bin/env python3
"""Check the required prior jobs themselves; failed unrelated jobs are not passes."""
import json,os,urllib.request
from pathlib import Path
repo=os.environ['GITHUB_REPOSITORY'];assert repo=='ghbarker/nuvio-native-legacy'
headers={'Authorization':'Bearer '+os.environ['GH_TOKEN'],'Accept':'application/vnd.github+json','X-GitHub-Api-Version':'2022-11-28'}
values={}
for key,run,sha in [('first_retry',38668819623,'2501484817c26215e5128110a3bd90529986cac0'),('second_retry',38669355249,'845ea6137755e40a486b6bc4d77dc173a4ae4567')]:
 base='https://api.github.com/repos/'+repo+'/actions/runs/'+str(run)
 with urllib.request.urlopen(urllib.request.Request(base,headers=headers),timeout=30) as f:meta=json.load(f)
 assert meta['head_sha']==sha and meta['run_attempt']==1
 with urllib.request.urlopen(urllib.request.Request(base+'/jobs?per_page=100',headers=headers),timeout=30) as f:jobs=json.load(f)
 assert jobs['total_count']==len(jobs['jobs']);values[key]=jobs
out=Path('build/review');out.mkdir(parents=True,exist_ok=True)
(out/'prior-jobs.json').write_text(json.dumps(values,indent=2)+'\n')
print('Prior exact-source job metadata downloaded')
