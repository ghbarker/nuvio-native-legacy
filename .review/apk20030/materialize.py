#!/usr/bin/env python3
"""Materialize immutable layers on CI, or verify their Git objects without a checkout."""
from pathlib import Path
import argparse,base64,hashlib,json,os,subprocess,tempfile
from require_plan import load_plan
p=argparse.ArgumentParser()
p.add_argument('--layer',required=True,choices=['product','test'])
p.add_argument('--output',type=Path)
p.add_argument('--proof',required=True,type=Path)
p.add_argument('--base-repository',type=Path)
p.add_argument('--allow-review-plan',action='store_true')
p.add_argument('--verify-only',action='store_true')
a=p.parse_args();here=Path(__file__).resolve().parent
plan,approved=load_plan(here/'plan.json',allow_review=a.allow_review_plan)
entry=plan['sources'][a.layer];manifest=json.loads((here/entry['manifest']).read_text())
sha=lambda b:hashlib.sha256(b).hexdigest()
assert manifest['commit']==entry['commit'] and manifest['tree']==entry['tree']
bundle=base64.b64decode((here/plan['bundle']['path']).read_bytes(),validate=False)
assert len(bundle)==plan['bundle']['bytes'] and sha(bundle)==plan['bundle']['sha256']
def git(repo,*args):return subprocess.check_output(['git','-C',str(repo),*args],stderr=subprocess.PIPE)
if a.verify_only:
 assert a.base_repository and a.allow_review_plan
 source=a.base_repository.resolve()
else:
 assert approved,'A pending source may only use object-only inspection'
 assert a.output and not a.output.exists()
 source=a.output.resolve()
 if a.base_repository:
  subprocess.run(['git','clone','--quiet','--shared','--no-checkout',str(a.base_repository.resolve()),str(source)],check=True)
 else:
  source.mkdir(parents=True);git(source,'init','--quiet');git(source,'remote','add','origin',plan['upstream']['url'])
  git(source,'fetch','--quiet','--depth=1','origin',plan['upstream']['commit'])
with tempfile.NamedTemporaryFile(prefix='logs20-source-',suffix='.bundle') as f:
 f.write(bundle);f.flush();git(source,'bundle','verify',f.name)
 if not a.verify_only:git(source,'bundle','unbundle',f.name)
if not a.verify_only:git(source,'checkout','--quiet','--detach',entry['commit'])
assert git(source,'rev-parse',entry['commit']+'^{tree}').decode().strip()==entry['tree']
parents=git(source,'show','-s','--format=%P',entry['commit']).decode().split()
assert parents==manifest['parents']
product=plan['sources']['product']['commit']
product_tests=git(source,'rev-parse',product+':tests').decode().strip()
assert git(source,'rev-parse',entry['commit']+':tests').decode().strip()==product_tests==manifest['tests_tree']
assert manifest['tests_tree_matches_product']
for f in manifest['files']:
 if f.get('deleted'):
  assert subprocess.run(['git','-C',str(source),'cat-file','-e',entry['commit']+':'+f['path']],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL).returncode
 else:
  data=git(source,'show',entry['commit']+':'+f['path']);assert sha(data)==f['sha256']
  assert git(source,'rev-parse',entry['commit']+':'+f['path']).decode().strip()==f['blob']
  if not a.verify_only:assert sha((source/f['path']).read_bytes())==f['sha256']
if not a.verify_only:
 assert git(source,'rev-parse','HEAD').decode().strip()==entry['commit']
 assert not git(source,'status','--porcelain')
report={'verified':True,'source_commit':entry['commit'],'tree':entry['tree'],'source_parents':parents,
 'base':plan['upstream']['commit'],'layer':a.layer,'review_commit':os.getenv('GITHUB_SHA','local-review'),
 'review_only':not approved,'source_frozen':plan['source_frozen'],
 'manifest_sha256':entry['manifest_sha256'],'bundle_sha256':plan['bundle']['sha256'],
 'changed_files_checked':len(manifest['files']),'product_tests_tree':product_tests,
 'product_tests_tree_preserved':True,'source_clean':True,
 'materialization_mode':'git-objects-only' if a.verify_only else 'checkout'}
a.proof.parent.mkdir(parents=True,exist_ok=True);a.proof.write_text(json.dumps(report,indent=2)+'\n')
print('Verified exact',a.layer,entry['commit'],'object-only' if a.verify_only else 'checkout')
