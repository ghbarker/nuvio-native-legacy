#!/usr/bin/env python3
"""Checkout the exact source-only Git commit; no bundle or source transforms."""
from pathlib import Path
import argparse,hashlib,json,os,subprocess
from require_plan import load_plan
p=argparse.ArgumentParser()
p.add_argument('--layer',required=True,choices=['test'])
p.add_argument('--output',type=Path);p.add_argument('--proof',required=True,type=Path)
p.add_argument('--base-repository',required=True,type=Path)
p.add_argument('--allow-review-plan',action='store_true');p.add_argument('--verify-only',action='store_true')
a=p.parse_args();here=Path(__file__).resolve().parent
plan,approved=load_plan(here/'plan.json',allow_review=a.allow_review_plan)
entry=plan['sources']['test'];manifest=json.loads((here/entry['manifest']).read_text())
assert manifest['commit']==entry['commit'] and manifest['tree']==entry['tree']
def git(repo,*args):return subprocess.check_output(['git','-C',str(repo),*args],stderr=subprocess.PIPE)
source=a.base_repository.resolve();inspected=entry['commit']
if a.verify_only:
 assert a.allow_review_plan
 available=subprocess.run(['git','-C',str(source),'cat-file','-e',inspected+'^{commit}'],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL).returncode==0
 if not available:
  assert not approved;inspected=plan['local_trace']['commit']
else:
 assert approved and a.output and not a.output.exists(),'Closed plan cannot materialize a build source'
 source=a.output.resolve()
 subprocess.run(['git','clone','--quiet','--shared','--no-checkout',str(a.base_repository.resolve()),str(source)],check=True)
 git(source,'checkout','--quiet','--detach',inspected)
assert git(source,'rev-parse',inspected+'^{tree}').decode().strip()==entry['tree']
parents=git(source,'show','-s','--format=%P',inspected).decode().split()
assert parents==(manifest['parents'] if inspected==entry['commit'] else plan['local_trace']['parents'])
tests=git(source,'rev-parse',inspected+':tests').decode().strip()
assert tests==manifest['tests_tree'] and manifest['tests_tree_preserved'] is False
assert manifest['existing_test_entries_preserved'] is False and manifest['existing_test_paths_preserved'] is True
change=manifest['reviewed_test_changes'];old=git(source,'ls-tree','-r',change['parent'],'tests').splitlines()
assert len(old)==manifest['existing_test_entries'] and hashlib.sha256(b'\n'.join(old)+b'\n').hexdigest()==manifest['existing_test_listing_sha256']
current={r.split(b'\t',1)[1]:r for r in git(source,'ls-tree','-r',inspected,'tests').splitlines()}
modified={x['path']:x for x in change['modified_existing_test_files']}
assert set(modified)=={'tests/ass_fontes.c','tests/ass_fontes.sh','tests/text_largura.c','tests/text_largura.sh'}
for row in old:
 path=row.split(b'\t',1)[1].decode()
 if path not in modified:assert current.get(path.encode())==row,'Unreviewed existing test changed or disappeared: '+path
 else:
  e=modified[path];assert row.decode()==e['before_listing'] and current[path.encode()].decode()==e['after_listing']
  before=git(source,'show',change['parent']+':'+path);after=git(source,'show',inspected+':'+path)
  assert hashlib.sha256(before).hexdigest()==e['before_sha256'] and hashlib.sha256(after).hexdigest()==e['after_sha256']
  if path.endswith('.c'):
   lines=iter(after.splitlines());assert e['old_c_lines_preserved_in_order'] and all(any(x==y for y in lines) for x in before.splitlines())
assert len(old)-len(modified)==manifest['unchanged_existing_test_entries']==1572
assert sorted(x.decode() for x in set(current)-{r.split(b'\t',1)[1] for r in old})==change['new_test_files']
product=change['product_commit'] if inspected==entry['commit'] else change['product_local_commit']
assert git(source,'show','-s','--format=%P',product).decode().split()==[change['parent']]
assert git(source,'diff','--name-only',product,inspected).decode().splitlines()==change['changed_paths']
assert len(change['changed_paths'])==10 and change['new_test_files']==['tests/ass_fonte_local.c','tests/ass_fonte_local.sh','tests/font_fixture.py','tests/legfontes.c','tests/legfontes.sh']
for path in change['new_test_files']:
 assert subprocess.run(['git','-C',str(source),'cat-file','-e',product+':'+path],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL).returncode
 assert git(source,'cat-file','-t',inspected+':'+path).strip()==b'blob'

sha=lambda b:hashlib.sha256(b).hexdigest()
for f in manifest['files']:
 if f.get('deleted'):
  assert subprocess.run(['git','-C',str(source),'cat-file','-e',inspected+':'+f['path']],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL).returncode
 else:
  data=git(source,'show',inspected+':'+f['path']);assert sha(data)==f['sha256']
  assert git(source,'rev-parse',inspected+':'+f['path']).decode().strip()==f['blob']
  if not a.verify_only:assert sha((source/f['path']).read_bytes())==f['sha256']
if not a.verify_only:
 assert git(source,'rev-parse','HEAD').decode().strip()==entry['commit']
 assert not git(source,'status','--porcelain')
r={'verified':True,'source_commit':inspected,'expected_remote_source_commit':entry['commit'],
 'tree':entry['tree'],'source_parents':parents,'layer':'test','review_commit':os.getenv('GITHUB_SHA','local-review'),
 'review_only':not approved,'source_frozen':plan['source_frozen'],'manifest_sha256':entry['manifest_sha256'],
 'source_binding':'exact-git-tree','changed_files_checked':len(manifest['files']),
 'tests_tree':tests,'tests_tree_preserved':False,'existing_test_entries_preserved':False,'existing_test_paths_preserved':True,'reviewed_tests_delta_verified':True,'source_clean':True,
 'materialization_mode':'git-objects-only' if a.verify_only else 'checkout'}
a.proof.parent.mkdir(parents=True,exist_ok=True);a.proof.write_text(json.dumps(r,indent=2)+'\n')
print('Verified exact private source',inspected,r['materialization_mode'])
