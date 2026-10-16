"""One exact-c2 behavioral negative control; compiler/link failures never count."""
from pathlib import Path
import argparse, hashlib, json, os, platform, re, shutil, subprocess, sys
from common import HERE, sha, source_binding

p=argparse.ArgumentParser()
for key in ['source','proof','output']:p.add_argument('--'+key,type=Path,required=True)
a=p.parse_args();s=a.source.resolve();o=a.output.resolve();o.mkdir(parents=True,exist_ok=False)
plan,binding,manifest=source_binding(s,a.proof)
assert binding['candidate_pr']==524 and platform.system()=='Darwin'
base=plan['base'];b=o/'baseline-source';setup=[]
def run(label,command,cwd=None):
    log=o/(label+'.log')
    with log.open('w')as f:r=subprocess.run(command,cwd=cwd,stdout=f,stderr=subprocess.STDOUT,timeout=120)
    assert r.returncode==0,(label,r.returncode)
    setup.append({'command':command,'returncode':r.returncode,'log':log.name,'log_sha256':sha(log)})
run('clone',['git','clone','--shared','--no-checkout',str(s),str(b)])
run('sparse-init',['git','sparse-checkout','init','--cone'],b)
run('sparse-paths',['git','sparse-checkout','set','src','tests','tools'],b)
run('checkout',['git','checkout','--detach',base],b)
assert subprocess.check_output(['git','rev-parse','HEAD'],cwd=b,text=True).strip()==base
tree=subprocess.check_output(['git','rev-parse','HEAD^{tree}'],cwd=b,text=True).strip()
assert tree==manifest['base_tree']
overlay={}
for entry in manifest['reviewed_test_paths']:
    path=entry['path'];assert path.startswith('tests/')
    assert sha(s/path)==entry['sha256']
    target=b/path;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(s/path,target)
    target.chmod(int(entry['mode'],8)&0o777);overlay[path]=sha(target)
def snapshot():
    subprocess.run(['git','diff','--exit-code',base,'--','src','rust','tools','deploy','android'],cwd=b,check=True,stdout=subprocess.DEVNULL)
    changed=subprocess.check_output(['git','status','--porcelain','--untracked-files=all'],cwd=b,text=True).splitlines()
    assert all(row[3:]in overlay for row in changed),changed
    assert all(sha(b/path)==digest for path,digest in overlay.items())
    return {str(path.relative_to(b)):sha(path) for path in sorted((b/'src').rglob('*'))if path.is_file()}
before=snapshot();d=o/'compiler';d.mkdir();ccbin=o/'cc-bin';ccbin.mkdir()
for name in ['cc','clang','gcc']:
    path=ccbin/name;path.write_bytes((HERE/'cc_rpath.py').read_bytes());path.chmod(0o755)
sdl3=Path(subprocess.check_output(['brew','--prefix','sdl3'],text=True).strip())/'lib'
tmp=o/'tmp';tmp.mkdir()
env=dict(os.environ,SANITIZE='0',CC=str(ccbin/'cc'),TMPDIR=str(tmp),REVIEW_REAL_CC='/usr/bin/cc',
    REVIEW_SDL3_LIBRARY_DIR=str(sdl3.resolve()),REVIEW_CC_OUTPUT=str(d),PATH=str(ccbin)+os.pathsep+os.environ['PATH'])
command=['/bin/bash','tests/title_identity524.sh','episode-id'];log=o/'episode-id.log'
with log.open('w')as f:r=subprocess.run(command,cwd=b,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=240)
output=log.read_text(errors='replace');captures=[]
for path in sorted(d.glob('*-invocation.json')):
    v=json.loads(path.read_text());assert v['compiler']=='/usr/bin/cc'and v['cwd']==str(b)and v['returncode']==0
    assert not any(x=='-DNDEBUG'or x.startswith('-fsanitize=')for x in v['arguments'])
    captures.append({'path':str(path.relative_to(o)),'sha256':sha(path),'record':v})
assert len(captures)==1 and captures[0]['record'].get('linked_binary'),'Missing successful actual compiler/link proof'
v=captures[0]['record'];assert v['bytes']>0 and v['load_commands_returncode']==v['linked_libraries_returncode']==0
assert r.returncode==1,('exact behavioral exit',r.returncode)
signature='result id=tt9054364 type=movie title=Chasing Mavericks'
request='request https://v3-cinemeta.strem.io/meta/movie/tt9054364.json'
assert sum(re.fullmatch(re.escape(signature)+r" index=[0-9]+ requests=[0-9]+", line) is not None for line in output.splitlines())==1 and output.splitlines().count(request)==1
assert not re.search(r'unknown scenario|Undefined symbols|linker command failed|error:|AddressSanitizer|runtime error:',output,re.I)
assert snapshot()==before
source_binding(s,a.proof)
raw={str(path.relative_to(o)):sha(path)for path in sorted(o.rglob('*'))if path.is_file()and not path.is_relative_to(b)and not path.is_relative_to(tmp)}
result={'outcome':'expected_behavioral_red','negative_control':True,'positive_group':False,'source_binding':binding,
    'plan_sha256':sha(HERE/'plan.json'),'baseline_commit':base,'baseline_tree':tree,'baseline_source_sha256':before,
    'test_overlay_sha256':overlay,'setup':setup,'command':command,'returncode':r.returncode,
    'expected_signature':signature,'expected_request':request,'log':log.name,'log_sha256':sha(log),
    'compiler_invocations':captures,'raw_files_sha256':raw,
    'scope':'One actual-c2 episode-id lookup with the frozen public test overlay. Actual compile/link succeeded; the specific wrong movie result is required. Not a positive test or a full application build.'}
(o/'negative-control.json').write_text(json.dumps(result,indent=2)+'\n')
shutil.rmtree(b)
print('PASS expected behavioral baseline RED: episode-id; actual compile/link successful',flush=True)
