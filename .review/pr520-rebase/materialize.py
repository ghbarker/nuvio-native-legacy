from pathlib import Path
import argparse,hashlib,json,os,subprocess
from common import HERE,approved,sha,git
p=argparse.ArgumentParser();p.add_argument('--target',type=Path,required=True);p.add_argument('--proof',type=Path,required=True);p.add_argument('--reverify',action='store_true');p.add_argument('--preflight',action='store_true');a=p.parse_args()
if not a.preflight:approved()
assert not(a.preflight and os.environ.get('GITHUB_ACTIONS')=='true')
m=json.loads((HERE/'source-manifest.json').read_text());target=a.target.resolve();patch=HERE/'product-delta.patch';commit=(HERE/'product-commit.txt').read_bytes()
assert sha(patch)==m['patch_sha256'];assert hashlib.sha1(b'commit '+str(len(commit)).encode()+b'\0'+commit).hexdigest()==m['head']
if not a.reverify:
 assert not target.exists()
 subprocess.run(['git','cat-file','-e',m['base']],check=True)
 subprocess.run(['git','worktree','add','--detach',str(target),m['base']],check=True)
 subprocess.run(['git','apply','--check','--index',str(patch)],cwd=target,check=True)
 subprocess.run(['git','apply','--index',str(patch)],cwd=target,check=True)
 assert git(target,'write-tree')==m['tree']
 assert subprocess.check_output(['git','hash-object','-w','-t','commit','--stdin'],cwd=target,input=commit).decode().strip()==m['head']
 subprocess.run(['git','reset','--hard',m['head']],cwd=target,check=True)
assert git(target,'rev-parse','HEAD')==m['head'] and git(target,'rev-parse','HEAD^{tree}')==m['tree']
assert git(target,'rev-list','--parents','-n','1','HEAD').split()==[m['head'],m['base']]
assert not git(target,'status','--porcelain')
assert git(target,'diff','--name-only',m['base'],m['head']).splitlines()==[e['path']for e in m['paths']]
assert len(m['paths'])==36
assert all(e['path'].startswith('src/') or e['path'] in ['android/app/src/main/java/space/nuvio/nativelegacy/NuvioActivity.kt','android/app/src/main/java/space/nuvio/nativelegacy/RegistroDownload.kt'] for e in m['paths'])
for e in m['paths']:assert sha(target/e['path'])==e['sha256'] and git(target,'rev-parse','HEAD:'+e['path'])==e['blob']
for name in ['tests','.github','tools','deploy','android/app/src/main/AndroidManifest.xml','android/app/build.gradle.kts','android/app/src/main/cpp/CMakeLists.txt']:assert git(target,'rev-parse','HEAD:'+name)==git(target,'rev-parse',m['base']+':'+name)
v={'verified':True,'source_commit':m['head'],'tree':m['tree'],'base':m['base'],'review_commit':os.environ.get('GITHUB_SHA','local-preflight'),'manifest_sha256':sha(HERE/'source-manifest.json'),'product_paths':len(m['paths']),'product_only':True,'tests_workflows_tools_deploy_unchanged':True}
a.proof.parent.mkdir(parents=True,exist_ok=True);a.proof.write_text(json.dumps(v,indent=2)+'\n');print('Verified exact source',m['head'],flush=True)
