#!/usr/bin/env python3
"""Verify each exact issue513 delta against its recorded parent and clean tests."""
import argparse,hashlib,json,os,subprocess
from pathlib import Path

HERE=Path(__file__).resolve().parent;p=argparse.ArgumentParser()
p.add_argument('--case',required=True);p.add_argument('--source-commit',required=True);p.add_argument('--target',type=Path,required=True);p.add_argument('--proof',type=Path,required=True);p.add_argument('--reverify',action='store_true');a=p.parse_args()
plan=json.loads((HERE/'plans.json').read_text());assert plan['frozen_sources']
case=next(c for c in plan['cases'] if c['id']==a.case);manifest=json.loads((HERE/case['manifest']).read_text())
source=a.source_commit;target=a.target.resolve()
if not a.reverify:
 assert not target.exists()
 if subprocess.run(['git','cat-file','-e',source],capture_output=True).returncode:subprocess.run(['git','fetch','origin',source],check=True)
 subprocess.run(['git','worktree','add','--detach',str(target),source],check=True)
def git(*args):return subprocess.check_output(['git','-C',str(target),*args],text=True).strip()
assert git('rev-parse','HEAD')==source
assert git('rev-list','--parents','-n','1',source).split()==[source,case['base']]
assert git('diff','--name-only',case['base'],source).splitlines()==sorted(e['path'] for e in manifest['files'])
subprocess.run(['git','-C',str(target),'diff','--exit-code',source,'--'],check=True)
for e in manifest['files']:
 data=(target/e['path']).read_bytes();assert hashlib.sha1(b'blob '+str(len(data)).encode()+b'\0'+data).hexdigest()==e['sha']
 assert git('rev-parse',source+':'+e['path'])==e['sha']
tests=git('rev-parse',source+':tests');assert tests==git('rev-parse',case['base']+':tests')
proof={'verified':True,'source_commit':source,'base':case['base'],'build_commit':os.environ['GITHUB_SHA'],'phase':'final','case':case['id'],'label':case['label'],
 'scope':'complete combined product candidate' if case['delivery'] else 'clean issue513 product candidate','files':manifest['files'],'feature_changed_paths':[e['path'] for e in manifest['files']],
 'original_tests_tree_unchanged':tests,'source_origins':plan['source_origins'],'product_delta_only':True,'test_only_overlay_unchanged_from_parent':case['delivery']}
a.proof.parent.mkdir(parents=True,exist_ok=True);a.proof.write_text(json.dumps(proof,indent=2)+'\n')
print('Verified exact issue513 source:',case['id'],source,len(manifest['files']),'product files; original tests unchanged')
