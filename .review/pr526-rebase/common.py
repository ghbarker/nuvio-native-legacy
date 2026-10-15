from pathlib import Path
import hashlib,json,os,subprocess
HERE=Path(__file__).resolve().parent
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def git(source,*args):return subprocess.check_output(['git','-C',str(source),*args],text=True).strip()
def approved():
 p=json.loads((HERE/'plan.json').read_text());assert p['frozen'] and p['root_reviewed'] and p['ci_execution_permitted']
 assert os.environ['SYNOPSIS_PLAN_SHA256']==sha(HERE/'plan.json')
 assert sha(HERE/'source-manifest.json')==p['source_manifest_sha256']
 for name,digest in p['recipe_inputs'].items():assert sha(HERE/name)==digest,name
 return p
def source_binding(source,proof):
 p=approved();v=json.loads(Path(proof).read_text());m=json.loads((HERE/'source-manifest.json').read_text())
 assert v['verified'] and v['source_commit']==m['head']==p['source_commit']
 assert v['tree']==m['tree']==p['source_tree'] and v['base']==m['base']==p['base']
 assert v['review_commit']==os.environ['GITHUB_SHA'] and v['manifest_sha256']==p['source_manifest_sha256']
 assert git(source,'rev-parse','HEAD')==m['head'] and git(source,'rev-parse','HEAD^{tree}')==m['tree']
 status=git(source,'status','--porcelain')
 if status:
  raw=subprocess.check_output(['git','-C',str(source),'status','--porcelain=v1','--untracked-files=all','-z'],text=True)
  fields=raw.split('\0');items=[];i=0
  while i<len(fields) and fields[i]:
   row=fields[i];i+=1;relative=row[3:];path=Path(source)/relative
   item={'status':row[:2],'path':relative}
   if any(c in row[:2]for c in 'RC') and i<len(fields):item['previous_path']=fields[i];i+=1
   if path.is_file() and not path.is_symlink():item.update(bytes=path.stat().st_size,sha256=sha(path))
   else:item['kind']='symlink-or-directory-or-missing'
   items.append(item)
  destination=Path(os.environ.get('GITHUB_WORKSPACE',os.getcwd()))/'results/source-dirty-status.json';destination.parent.mkdir(parents=True,exist_ok=True)
  destination.write_text(json.dumps({'source_commit':m['head'],'dirty_paths':items,'file_contents_recorded':False},indent=2)+'\n')
  print('Unexpected source paths (no contents):',json.dumps(items),flush=True)
 assert not status,'Source worktree is dirty; preserved exact path/digest diagnostics; validation refused'

 for e in m['paths']:assert sha(Path(source)/e['path'])==e['sha256'],e['path']
 return p,v,m
def record(out,label,command,cwd=None,env=None,timeout=900):
 out=Path(out);out.mkdir(parents=True,exist_ok=True);log=out/(label+'.log')
 with log.open('w')as f:r=subprocess.run(command,cwd=cwd,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=timeout)
 if r.returncode:print('\n'.join(log.read_text(errors='replace').splitlines()[-65:]),flush=True)
 assert r.returncode==0,(label,r.returncode)
 return {'name':label,'command':command,'returncode':r.returncode,'log':log.name,'log_sha256':sha(log)}
