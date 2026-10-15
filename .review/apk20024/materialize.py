#!/usr/bin/env python3
"""Verify and unpack immutable source commits; never merges or edits product files."""
from pathlib import Path
import argparse,base64,hashlib,json,os,subprocess,tempfile
from require_plan import load_plan
p=argparse.ArgumentParser();p.add_argument('--layer',required=True,choices=['product','test']);p.add_argument('--output',required=True,type=Path);p.add_argument('--proof',required=True,type=Path);p.add_argument('--base-repository',type=Path);p.add_argument('--allow-review-plan',action='store_true');a=p.parse_args();here=Path(__file__).resolve().parent
plan,approved=load_plan(here/'plan.json',allow_review=a.allow_review_plan)
entry=plan['sources'][a.layer];m=here/entry['manifest'];sha=lambda x:hashlib.sha256(x).hexdigest();assert sha(m.read_bytes())==entry['manifest_sha256'];manifest=json.loads(m.read_text());assert manifest['commit']==entry['commit'] and manifest['tree']==entry['tree'] and manifest['tests_tree_unchanged']
bundle=base64.b64decode((here/plan['bundle']['path']).read_text(),validate=False);assert len(bundle)==plan['bundle']['bytes'] and sha(bundle)==plan['bundle']['sha256']
source=a.output.resolve();assert not source.exists(),f'Refusing to replace existing source {source}'
def run(cmd,cwd=None):return subprocess.check_output(cmd,cwd=cwd,text=True,stderr=subprocess.STDOUT).strip()
if a.base_repository:
 run(['git','clone','--quiet','--shared','--no-checkout',str(a.base_repository.resolve()),str(source)])
else:
 source.mkdir(parents=True);run(['git','init','--quiet'],cwd=source);run(['git','remote','add','origin',plan['upstream']['url']],cwd=source);run(['git','fetch','--quiet','--depth=1','origin',plan['upstream']['commit']],cwd=source)
try:run(['git','cat-file','-e',plan['upstream']['commit']+'^{commit}'],cwd=source)
except subprocess.CalledProcessError:run(['git','fetch','--quiet','--depth=1',plan['upstream']['url'],plan['upstream']['commit']],cwd=source)
run(['git','checkout','--quiet','--detach',plan['upstream']['commit']],cwd=source)
with tempfile.NamedTemporaryFile(prefix='logs8-source-',suffix='.bundle') as f:
 f.write(bundle);f.flush();run(['git','bundle','verify',f.name],cwd=source);run(['git','bundle','unbundle',f.name],cwd=source)
run(['git','checkout','--quiet','--detach',entry['commit']],cwd=source)
assert run(['git','rev-parse','HEAD'],cwd=source)==entry['commit'];assert run(['git','rev-parse','HEAD^{tree}'],cwd=source)==entry['tree'];assert not run(['git','status','--porcelain'],cwd=source)
for f in manifest['files']:assert sha((source/f['path']).read_bytes())==f['sha256'],f['path']
assert run(['git','rev-parse','HEAD:tests'],cwd=source)==manifest['tests_tree'];assert run(['git','rev-parse',plan['upstream']['commit']+':tests'],cwd=source)==manifest['tests_tree']
a.proof.parent.mkdir(parents=True,exist_ok=True);report={'verified':True,'source_commit':entry['commit'],'tree':entry['tree'],'base':plan['upstream']['commit'],'layer':a.layer,'review_commit':os.getenv('GITHUB_SHA','local-review'),'review_only':not approved,'manifest_sha256':entry['manifest_sha256'],'bundle_sha256':plan['bundle']['sha256'],'changed_files_checked':len(manifest['files']),'original_tests_tree_unchanged':True,'source_clean':True}
a.proof.write_text(json.dumps(report,indent=2)+'\n');print('Verified exact',a.layer,'source',entry['commit'],entry['tree'])
