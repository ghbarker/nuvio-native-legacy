#!/usr/bin/env python3
"""Publish only the exact green subtitle source to its new dedicated branch."""
from pathlib import Path
import argparse,json,os,re,subprocess,tempfile
from common import source_binding
SOURCE='2faf69154a371fa9b5147c1ccd58017fb387077f';TREE='15dabcd6a9e395f13993ca2013df7ceacfe09a20';BASE='5d2f8c8c99113e416e1fb06f8b60da1d13635e85';REPO='ghbarker/nuvio-native-legacy';REMOTE='https://github.com/'+REPO+'.git';UPSTREAM='https://github.com/iqui27/nuvio-native-legacy.git';REF='refs/heads/feat/subtitle-settings-parity-475-20261015'
def publish(source,proof_path,output,run=subprocess.run,read=subprocess.check_output):
 source=Path(source).resolve();plan,proof,manifest=source_binding(source,proof_path);review=os.environ['GITHUB_SHA'];assert re.fullmatch('[0-9a-f]{40}',review);assert os.environ['GH_REPO']==REPO and os.environ.get('GH_TOKEN');assert(proof['source_commit'],proof['tree'],proof['base'])==(SOURCE,TREE,BASE)
 def git(*args):return read(['git',*args],cwd=source,text=True).strip()
 assert git('rev-list','--parents','-n','1','HEAD').split()==[SOURCE,BASE]
 with tempfile.TemporaryDirectory(prefix='nuvio-subtitle-source-auth-',dir=os.environ['RUNNER_TEMP'])as tmp:
  ask=Path(tmp)/'askpass.sh';ask.write_text('#!/bin/sh\ncase "$1" in\n *Username*) printf "%s\\n" x-access-token;;\n *Password*) printf "%s\\n" "$GH_TOKEN";;\n *) exit 1;;\nesac\n');ask.chmod(0o700);env=dict(os.environ,GIT_ASKPASS=str(ask),GIT_TERMINAL_PROMPT='0')
  for key in tuple(env):
   if key.startswith(('GIT_TRACE','GIT_CURL_VERBOSE')):env.pop(key)
  # Observe current upstream; do not discard a green source for unrelated advances.
  z=run(['git','fetch','--no-tags',UPSTREAM,'refs/heads/master'],cwd=source,env=env,text=True,capture_output=True,timeout=90);assert z.returncode==0,'Could not inspect current upstream';live=git('rev-parse','FETCH_HEAD');assert git('merge-base',BASE,live)==BASE,'Upstream history no longer contains the reviewed base'
  overlapping=set(git('diff','--name-only',BASE,live).splitlines())&{x['path']for x in manifest['paths']};assert not overlapping,('Upstream changed reviewed product paths; root review required',sorted(overlapping))
  merged=run(['git','merge-tree','--write-tree',live,SOURCE],cwd=source,env=env,text=True,capture_output=True,timeout=60);assert merged.returncode==0,'Current upstream cannot cleanly merge tested source';merge_tree=merged.stdout.splitlines()[0];assert re.fullmatch('[0-9a-f]{40}',merge_tree)
  def remote():
   q=run(['git','ls-remote','--exit-code',REMOTE,REF],cwd=source,env=env,text=True,capture_output=True,timeout=60)
   if q.returncode==2:assert not q.stdout.strip();return None
   assert q.returncode==0;rows=[x.split()for x in q.stdout.splitlines()if x.strip()];assert len(rows)==1 and rows[0][1]==REF and re.fullmatch('[0-9a-f]{40}',rows[0][0]);return rows[0][0]
  previous=remote();assert previous in [None,SOURCE],'Dedicated source ref already holds another commit; no overwrite'
  if previous is None:
   z=run(['git','-c','push.followTags=false','push','--porcelain',REMOTE,'HEAD:'+REF],cwd=source,env=env,text=True,capture_output=True,timeout=120);assert z.returncode==0,'Normal non-force source push failed'
  assert remote()==SOURCE
 result={'outcome':'passed','review_commit':review,'source_commit':SOURCE,'source_tree':TREE,'source_parent':BASE,'repository':REPO,'ref':REF,'previous_head':previous,'current_upstream_commit':live,'clean_merge_tree':merge_tree,'unreviewed_overlapping_product_paths':[],'required_successful_jobs':plan['required_jobs']+['all_gates'],'normal_non_force_push_only':True,'pull_request_or_release_created':False};output=Path(output);output.parent.mkdir(parents=True,exist_ok=True);output.write_text(json.dumps(result,indent=2)+'\n');print('Verified exact green subtitle source branch',REF,SOURCE)
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);p.add_argument('--proof',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();publish(a.source,a.proof,a.output)
