"""Exact reviewed public source and test bindings for independent candidate lanes."""
from pathlib import Path
import hashlib,json,os,subprocess
HERE=Path(__file__).resolve().parent

def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def git(source,*args):return subprocess.check_output(['git','-C',str(source),*args],text=True).strip()
def approved():
 p=json.loads((HERE/'plan.json').read_text())
 assert p['frozen']and p['root_reviewed']and p['ci_execution_permitted']and p['remote_source_pins_ready']
 assert sha(HERE/'plan.json')==os.environ['REBASE_PLAN_SHA256']
 assert sha(HERE/'source-candidates.json')==p['source_manifest_sha256']
 for relative,digest in p['recipe_inputs'].items():assert sha(HERE/relative)==digest,relative
 return p

def source_binding(source,proof):
 p=approved();v=json.loads(Path(proof).read_text());all_sources=json.loads((HERE/'source-candidates.json').read_text());key=str(v['candidate_pr']);m=all_sources[key]
 if os.environ.get('REBASE_PR'):assert key==os.environ['REBASE_PR']
 assert v['verified']and v['source_clean']and v['source_commit']==m['head']and v['tree']==m['tree']
 assert v['base']==m['base']and v['source_parents']==m['parents']
 assert v['review_commit']==os.environ['GITHUB_SHA']and v['workflow_run_id']==int(os.environ['GITHUB_RUN_ID'])and v['workflow_run_attempt']==int(os.environ['GITHUB_RUN_ATTEMPT'])
 assert v['manifest_sha256']==p['source_manifest_sha256']
 assert git(source,'rev-parse','HEAD')==m['head']and git(source,'rev-parse','HEAD^{tree}')==m['tree']
 assert git(source,'rev-list','--parents','-n','1','HEAD').split()==[m['head'],*m['parents']]
 subprocess.run(['git','-C',str(source),'merge-base','--is-ancestor',m['base'],m['head']],check=True)
 assert not git(source,'status','--porcelain'),'Actual candidate worktree changed'
 assert git(source,'diff','--name-only',m['base'],m['head']).splitlines()==m['exact_diff_paths']
 assert m['source_identity_kind']=='actual-remote'
 for e in m['paths']+m['reviewed_test_paths']+m['compiled_source_inputs']+m['rust_inputs']:
  assert git(source,'rev-parse','HEAD:'+e['path'])==e['blob'],e['path']
  if 'sha256'in e:assert sha(Path(source)/e['path'])==e['sha256'],e['path']
 return p,v,m

def record(out,label,command,cwd=None,env=None,timeout=900):
 out=Path(out);out.mkdir(parents=True,exist_ok=True);log=out/(label+'.log')
 with log.open('w')as f:r=subprocess.run(command,cwd=cwd,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=timeout)
 if r.returncode:print('\n'.join(log.read_text(errors='replace').splitlines()[-65:]),flush=True)
 assert r.returncode==0,(label,r.returncode)
 return {'name':label,'command':command,'returncode':r.returncode,'log':log.name,'log_sha256':sha(log)}
