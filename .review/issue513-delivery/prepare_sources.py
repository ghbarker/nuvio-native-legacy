#!/usr/bin/env python3
"""Publish exact isolated review candidates; original feature commits stay intact."""
import hashlib,json,os,subprocess
from pathlib import Path

HERE=Path(__file__).resolve().parent
plan=json.loads((HERE/'plans.json').read_text());assert plan['frozen_sources']
out=Path('build/review');out.mkdir(parents=True,exist_ok=True)
tmp=Path(os.environ['RUNNER_TEMP'])/'issue513-sources';tmp.mkdir()
ask=tmp/'askpass.sh';ask.write_text('#!/bin/sh\ncase "$1" in *Username*) printf "%s\\n" x-access-token;; *Password*) printf "%s\\n" "$GH_TOKEN";; *) exit 1;; esac\n');ask.chmod(0o700)
env=dict(os.environ,GIT_ASKPASS=str(ask),GIT_TERMINAL_PROMPT='0')
def git(*args,cwd=None,capture=False):return subprocess.run(['git',*args],cwd=cwd,env=env,text=True,capture_output=capture,check=True)
for origin in plan['source_origins']['immutable_manifests']:
 assert hashlib.sha256((HERE/origin['manifest']).read_bytes()).hexdigest()==origin['sha256']
prepared=[]
for case in plan['cases']:
 base=case['base'];patch=HERE/case['patch'];manifest=json.loads((HERE/case['manifest']).read_text())
 assert hashlib.sha256(patch.read_bytes()).hexdigest()==case['patch_sha256'] and manifest['base']==base
 if subprocess.run(['git','cat-file','-e',base],capture_output=True).returncode:
  fetched=subprocess.run(['git','fetch','origin',base],env=env)
  if fetched.returncode:git('fetch','https://github.com/iqui27/nuvio-native-legacy.git',base)
 target=tmp/case['id'];git('worktree','add','--detach',str(target),base)
 git('apply','--check',str(patch),cwd=target);git('apply','--index',str(patch),cwd=target)
 changed=git('diff','--cached','--name-only','-z',cwd=target,capture=True).stdout.strip('\0').split('\0')
 assert sorted(changed)==sorted(e['path'] for e in manifest['files'])
 for e in manifest['files']:
  assert e['path'].startswith('src/') and e['path'].endswith(('.c','.h'))
  assert git('rev-parse',':'+e['path'],cwd=target,capture=True).stdout.strip()==e['sha']
 actor=os.environ['GITHUB_ACTOR'];git('-c','user.name='+actor,'-c','user.email='+actor+'@users.noreply.github.com','commit','-m',case['label'],cwd=target)
 source=git('rev-parse','HEAD',cwd=target,capture=True).stdout.strip()
 assert git('rev-list','--parents','-n','1','HEAD',cwd=target,capture=True).stdout.split()==[source,base]
 branch='candidate/issue513-'+os.environ['GITHUB_RUN_ID']+'-'+os.environ['GITHUB_RUN_ATTEMPT']+'-'+case['id']
 lookup=subprocess.run(['git','ls-remote','--exit-code','origin','refs/heads/'+branch],env=env,capture_output=True)
 assert lookup.returncode==2
 git('push','origin','HEAD:refs/heads/'+branch,cwd=target)
 prepared.append(dict(case,source_commit=source,candidate_branch=branch))
result={'matrix':{'include':prepared},'android_matrix':{'include':[c for c in prepared if c['delivery']]},'delivery':next(c for c in prepared if c['delivery'])}
(out/'prepared-cases.json').write_text(json.dumps(result,indent=2)+'\n')
with open(os.environ['GITHUB_OUTPUT'],'a') as f:
 for k,v in result.items():f.write(k+'='+json.dumps(v,separators=(',',':'))+'\n')
print('Materialized exact isolated issue513 sources; original product commits and unrelated refs untouched')
