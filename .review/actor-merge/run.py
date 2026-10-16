#!/usr/bin/env python3
"""Compare the unmodified cast assertion, never treat setup failure as evidence."""
from pathlib import Path
import argparse,hashlib,json,os,re,resource,shlex,shutil,subprocess,sys
sha=lambda path:hashlib.sha256(Path(path).read_bytes()).hexdigest()
def git(source,*args):return subprocess.check_output(['git','-C',str(source),*args],text=True).strip()
def require(ok,message):
    if not ok:raise AssertionError(message)
def save(path,value):Path(path).write_text(json.dumps(value,indent=2)+'\n')
def main():
    ap=argparse.ArgumentParser();ap.add_argument('--variant',choices=('baseline','current'),required=True);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args()
    kit=Path(__file__).resolve().parent;driver=kit.parent.parent;plan=json.loads((kit/'plan.json').read_text());out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
    require(plan['root_approved'] and plan['execution_enabled'],'closed root gates')
    require(sha(kit/'plan.json')==os.environ['ACTOR_PLAN_SHA256'],'plan hash mismatch')
    review=git(driver,'rev-parse','HEAD');require(review==os.environ['GITHUB_SHA'],'actual review mismatch')
    require(git(driver,'rev-parse','HEAD^')==plan['base'],'baseline review must directly parent Logs21')
    require(os.environ['GITHUB_REF']=='refs/heads/'+plan['branch'],'workflow branch mismatch')
    require(set(git(driver,'diff','--name-only',plan['base'],'HEAD').splitlines())==set(plan['baseline_test_overlays'])|set(plan['review_paths']),'baseline has unexpected source/recipe delta')
    for name,digest in plan['recipe_sha256'].items():require(sha(kit/name)==digest,'recipe changed: '+name)
    source=driver if a.variant=='baseline' else driver.parent/'current-source'
    if a.variant=='current':
        require(git(source,'rev-parse','HEAD')==plan['current_remote_source'],'current source commit mismatch')
        require(git(source,'rev-parse','HEAD^{tree}')==plan['current_source_tree'],'current complete source tree mismatch')
    for name,fact in plan['baseline_test_overlays'].items():
        require(git(source,'rev-parse','HEAD:'+name)==fact['git_blob'] and sha(source/name)==fact['sha256'],'fixture overlay mismatch: '+name)
    require(sha(source/plan['script'])==plan['script_sha256'],'public script changed')
    require(not git(driver,'status','--porcelain','--untracked-files=all') and not git(source,'status','--porcelain','--untracked-files=all'),'dirty source before comparison')
    resource.setrlimit(resource.RLIMIT_CORE,(0,0));require(resource.getrlimit(resource.RLIMIT_AS)==(-1,-1),'runtime address space must be uncapped')
    compiler=shutil.which('clang');require(compiler is not None,'clang missing')
    tools=out/'tools';tools.mkdir();wrapper=tools/'cc';wrapper.write_text('#!/bin/sh\nexec '+shlex.quote(sys.executable)+' '+shlex.quote(str(kit/'cc_record.py'))+' "$@"\n');wrapper.chmod(0o755)
    proof=out/'compiler';env=dict(os.environ,PATH=str(tools)+os.pathsep+os.environ['PATH'],CC=str(wrapper),ACTOR_SOURCE=str(source),ACTOR_CC_PROOF=str(proof),ACTOR_REAL_CC=compiler,ACTOR_LOCAL_CANDIDATE=plan['base'] if a.variant=='baseline' else plan['current_local_provenance'],SANITIZE='0',SDL_VIDEODRIVER='dummy',PYTHONDONTWRITEBYTECODE='1')
    r={'variant':a.variant,'review_commit':review,'actual_source_commit':git(source,'rev-parse','HEAD'),'actual_source_tree':git(source,'rev-parse','HEAD^{tree}'),'baseline_product_code':plan['base'],'current_product_source':plan['current_remote_source'],'plan_sha256':sha(kit/'plan.json'),'workflow_run_id':os.environ['GITHUB_RUN_ID'],'workflow_run_attempt':os.environ['GITHUB_RUN_ATTEMPT'],'expected_actor_assertion_reproduced':False,'product_fixed':False}
    save(out/'receipt.json',r)
    got=subprocess.run(['bash',plan['script']],cwd=source,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=180);(out/'run.log').write_bytes(got.stdout);r.update(script_returncode=got.returncode,log_sha256=sha(out/'run.log'));save(out/'receipt.json',r)
    c=json.loads((proof/'receipt.json').read_text());require(c['returncode']==0 and c['dependency_exit_code']==0 and c['elf'],'actual compilation/link did not succeed')
    require(c['source_review']==r['actual_source_commit'],'compiler source mismatch')
    require(not any(x.startswith('-fsanitize=') for x in c['command']),'normal comparison changed sanitizer flags')
    require({'src/descoberta.c','src/catalogo.c','tests/detalheanime.c','tests/addons_manifest_notifications_stub.inc'}<=set(c['dependencies']),'actual graph incomplete')
    for name,fact in c['dependencies'].items():require(git(source,'rev-parse','HEAD:'+name)==fact['git_blob'] and sha(source/name)==fact['sha256'],'source dependency mismatch: '+name)
    require(sha(proof/'compile.log')==c['compile_log_sha256'] and sha(proof/'dependencies.log')==c['dependency_log_sha256'],'compiler raw log mismatch')
    runs=sorted(proof.glob('runtime-*.json'));require(len(runs)==1,'actual execution count mismatch');runtime=json.loads(runs[0].read_text());text=runs[0].with_suffix('.log').read_text()
    require(runtime['arguments']==[] and runtime['returncode']==plan['expected_runtime_returncode'] and got.returncode!=0,'expected assertion signal not observed')
    require(runtime['binary_sha256']==c['binary_sha256'] and runtime['log_sha256']==sha(runs[0].with_suffix('.log')),'actual binary/log join mismatch')
    require(runtime['runtime_limits']['address_space']==[-1,-1] and runtime['runtime_limits']['core']==[0,0],'runtime limits mismatch')
    require(plan['expected_assertion'] in text and 'Assertion' in text,'wrong assertion failure')
    require(plan['required_prior_control'] in text and plan['required_actual_result'] in text,'prior control or actual four-actor result missing')
    require(not re.search(r'ERROR: (?:AddressSanitizer|LeakSanitizer)|runtime error:|Segmentation fault',text),'unrelated runtime failure')
    require(not git(driver,'status','--porcelain','--untracked-files=all') and not git(source,'status','--porcelain','--untracked-files=all'),'source changed')
    r.update(expected_actor_assertion_reproduced=True,source_clean_before_after=True,actual_binary_sha256=c['binary_sha256'],actual_runtime_returncode=runtime['returncode'],compiler_receipt_sha256=sha(proof/'receipt.json'),runtime_receipt_sha256=sha(runs[0]))
    r['raw_files_sha256']={str(p.relative_to(out)):sha(p) for p in out.rglob('*') if p.is_file() and p.name!='receipt.json'};save(out/'receipt.json',r)
    print(a.variant+': REPRODUCED exact actor assertion after prior control; product remains unfixed')
if __name__=='__main__':main()
