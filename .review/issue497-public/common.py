"""Closed #497 source and recipe binding; no prior execution is a pass."""
from pathlib import Path
import hashlib,json,os,subprocess
HERE=Path(__file__).resolve().parent
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def git(source,*args):return subprocess.check_output(['git','-C',str(source),*args],text=True).strip()
def approved():
 p=json.loads((HERE/'plan.json').read_text())
 assert p['frozen'] and p['root_reviewed'] and p['ci_execution_permitted']
 assert os.environ['STARTUP497_PLAN_SHA256']==sha(HERE/'plan.json')
 assert sha(HERE/'source-manifest.json')==p['source_manifest_sha256']
 for name,digest in p['recipe_inputs'].items():assert sha(HERE/name)==digest,name
 return p
def source_binding(source,proof):
 p=approved();v=json.loads(Path(proof).read_text());m=json.loads((HERE/'source-manifest.json').read_text())
 assert v['verified'] and v['product_only'] and v['source_clean']
 assert v['source_commit']==m['head']==p['source_commit']
 assert v['tree']==m['tree']==p['source_tree'] and v['base']==m['base']==p['base']
 assert v['source_parents']==m['parents']==[m['base']]
 assert v['review_commit']==os.environ['GITHUB_SHA'] and v['manifest_sha256']==p['source_manifest_sha256']
 assert v['workflow_run_id']==int(os.environ['GITHUB_RUN_ID']) and v['workflow_run_attempt']==int(os.environ['GITHUB_RUN_ATTEMPT'])
 assert git(source,'rev-parse','HEAD')==m['head'] and git(source,'rev-parse','HEAD^{tree}')==m['tree']
 assert git(source,'rev-list','--parents','-n','1','HEAD').split()==[m['head'],m['base']]
 assert git(source,'diff','--name-only',m['base'],m['head']).splitlines()==sorted(e['path']for e in m['paths'])
 assert not git(source,'status','--porcelain'),'Actual source worktree changed'
 for e in m['paths']:assert sha(Path(source)/e['path'])==e['sha256'],e['path']
 assert v['tests_unchanged'] and v['all_non_source_paths_unchanged'] and len(m['paths'])==4
 return p,v,m
def record(out,label,command,cwd=None,env=None,timeout=900):
 out=Path(out);out.mkdir(parents=True,exist_ok=True);log=out/(label+'.log')
 with log.open('w')as f:r=subprocess.run(command,cwd=cwd,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=timeout)
 if r.returncode:print('\n'.join(log.read_text(errors='replace').splitlines()[-65:]),flush=True)
 assert r.returncode==0,(label,r.returncode)
 return {'name':label,'command':command,'returncode':r.returncode,'log':log.name,'log_sha256':sha(log)}
