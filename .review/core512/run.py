#!/usr/bin/env python3
"""Collect all sixteen source-bound core512 groups without masking failures."""
import argparse,hashlib,json,os,re,resource,shlex,shutil,subprocess,sys,time,traceback
from pathlib import Path
sha=lambda path:hashlib.sha256(Path(path).read_bytes()).hexdigest()
def git(*args):return subprocess.check_output(['git',*args],text=True).strip()
def require(ok,message):
    if not ok:raise AssertionError(message)
def save(path,value):Path(path).write_text(json.dumps(value,indent=2)+'\n')
def main():
    ap=argparse.ArgumentParser();ap.add_argument('--output',required=True);args=ap.parse_args()
    root=Path.cwd().resolve();kit=Path(__file__).resolve().parent;out=Path(args.output).resolve();out.mkdir(parents=True,exist_ok=True)
    plan=json.loads((kit/'plan.json').read_text());head=git('rev-parse','HEAD')
    require(plan['root_approved'] and plan['execution_enabled'],'closed root gates')
    require(sha(kit/'plan.json')==os.environ['CORE512_PLAN_SHA256'],'plan hash mismatch')
    require(head==os.environ['GITHUB_SHA'],'actual review HEAD mismatch')
    require(os.environ['GITHUB_REF']=='refs/heads/'+plan['branch'],'review branch mismatch')
    require(git('rev-parse','HEAD^')==plan['base'],'composite review must directly parent base49a')
    changed=set(git('diff','--name-only',plan['base'],'HEAD').splitlines())
    require(changed==set(plan['source_files'])|set(plan['review_paths']),'unexpected composite source/recipe delta')
    for name,fact in plan['source_files'].items():
        entry=git('ls-tree','HEAD','--',name).split('\t')[0].split()
        require(entry==[fact['git_mode'],'blob',fact['git_blob']] and sha(root/name)==fact['sha256'],'source blob/mode/hash mismatch: '+name)
    for name,expected in plan['recipe_sha256'].items():require(sha(kit/name)==expected,'recipe changed: '+name)
    require(not git('status','--porcelain','--untracked-files=all'),'dirty source before validation')
    cc_real=shutil.which('clang');require(cc_real is not None,'real clang missing')
    resource.setrlimit(resource.RLIMIT_CORE,(0,0));require(resource.getrlimit(resource.RLIMIT_AS)[0]==resource.RLIM_INFINITY,'runtime must be uncapped')
    tools=out/'toolchain';tools.mkdir();cc=tools/'cc'
    cc.write_text('#!/bin/sh\nexec '+shlex.quote(sys.executable)+' '+shlex.quote(str(kit/'cc_record.py'))+' "$@"\n');cc.chmod(0o755)
    groups=[];errors=[]
    receipt={'review_commit':head,'source_base':plan['base'],'local_candidate_provenance':plan['local_candidate_provenance'],
        'source_binding':'Exact composite36 source blobs/modes plus4 recipe files on direct base49a; candidate SHA is provenance only',
        'source_files':plan['source_files'],'plan_sha256':sha(kit/'plan.json'),'workflow_run_id':os.environ['GITHUB_RUN_ID'],
        'workflow_run_attempt':os.environ['GITHUB_RUN_ATTEMPT'],'groups':groups,'errors':errors,'passed':False,
        'full_app_or_sdk_validated':False,'extras_validated':False}
    save(out/'receipt.json',receipt)
    for mode in plan['modes']:
        for suite in plan['suites']:
            label=suite['name']+'-'+mode;directory=out/label;directory.mkdir();started=time.monotonic()
            group={'label':label,'mode':mode,'suite':suite['name'],'command':['bash',suite['script']],'passed':False};groups.append(group)
            env=dict(os.environ,PATH=str(tools)+os.pathsep+os.environ['PATH'],CC=str(cc),CORE512_SOURCE=str(root),
                CORE512_LOCAL_CANDIDATE=plan['local_candidate_provenance'],CORE512_CC_PROOF=str(directory/'compiler'),CORE512_REAL_CC=cc_real,
                SANITIZE='1' if mode=='asan' else '0',SDL_VIDEODRIVER='dummy',ASAN_OPTIONS='detect_leaks=0:halt_on_error=1',
                UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1',PYTHONDONTWRITEBYTECODE='1')
            log=''
            try:
                try:
                    got=subprocess.run(group['command'],env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,timeout=180)
                    log=got.stdout;group['script_returncode']=got.returncode
                except subprocess.TimeoutExpired as exc:
                    log=exc.stdout or '';log=log.decode(errors='replace') if isinstance(log,bytes) else log
                    group['timed_out']=True;raise AssertionError('bounded public-script timeout')
                finally:
                    (directory/'run.log').write_text(log);group['log_sha256']=sha(directory/'run.log')
                require(got.returncode==0,'public script returned '+str(got.returncode))
                require(not re.search(r'ERROR: (?:AddressSanitizer|LeakSanitizer)|UndefinedBehaviorSanitizer|runtime error:|Assertion .* failed|Segmentation fault|Aborted',log),'sanitizer/runtime failure')
                proof=directory/'compiler';facts=json.loads((proof/'receipt.json').read_text())
                group['compiler_receipt_sha256']=sha(proof/'receipt.json')
                require(facts['returncode']==0 and facts['dependency_exit_code']==0 and facts['elf'],'real compile/link failed')
                require(facts['source_review']==head and facts['local_candidate_provenance']==plan['local_candidate_provenance'],'compiler source mismatch')
                require(('-fsanitize=address,undefined' in facts['command'])==(mode=='asan'),'sanitizer flags mismatch')
                require(sha(proof/'compile.log')==facts['compile_log_sha256'] and sha(proof/'dependencies.log')==facts['dependency_log_sha256'],'compiler log hash mismatch')
                require(set(suite['required_dependencies'])<=set(facts['dependencies']),'required real production dependencies omitted')
                for name,fact in facts['dependencies'].items():
                    expected=plan['source_files'].get(name)
                    blob=expected['git_blob'] if expected else git('rev-parse',plan['base']+':'+name)
                    require(fact['git_blob']==blob and sha(root/name)==fact['sha256'],'actual dependency mismatch: '+name)
                executions=sorted(proof.glob('runtime-*.json'));require(len(executions)==len(suite['arguments']),'actual execution count mismatch')
                for path,arguments in zip(executions,suite['arguments']):
                    actual=json.loads(path.read_text());require(actual['arguments']==arguments and actual['returncode']==0,'actual scenario failed: '+repr(arguments))
                    require(actual['binary_sha256']==facts['binary_sha256'] and sha(path.with_suffix('.log'))==actual['log_sha256'],'executed binary/log hash mismatch')
                    require(actual['runtime_limits']['address_space']==[-1,-1] and actual['runtime_limits']['core']==[0,0],'runtime limit mismatch')
                    text=path.with_suffix('.log').read_text()
                    marker=suite['marker']+(arguments[0]+' (' if suite['name']=='catalog_episode_identity512' else '')
                    require(text.count(marker)==1,'actual completion marker missing or repeated: '+marker)
                    require(not re.search(r'ERROR: (?:AddressSanitizer|LeakSanitizer)|UndefinedBehaviorSanitizer|runtime error:|Assertion .* failed|Segmentation fault|Aborted',text),'actual execution sanitizer/runtime diagnostic')
                require(not git('status','--porcelain','--untracked-files=all'),'source modified during validation')
                group.update(passed=True,actual_executions=len(executions),binary_sha256=facts['binary_sha256'])
                print(label+': PASS',flush=True)
            except Exception as exc:
                group['error']={'type':type(exc).__name__,'message':str(exc),'traceback':traceback.format_exc()};errors.append({'label':label,**group['error']})
                print(label+': FAIL '+str(exc)+'\n'+log[-2000:],flush=True)
            finally:
                group['seconds']=time.monotonic()-started;save(directory/'verdict.json',group);save(out/'receipt.json',receipt)
    receipt['all_groups_collected']=len(groups)==plan['group_count'];receipt['source_clean_before_after']=not git('status','--porcelain','--untracked-files=all')
    receipt['actual_binary_executions']=sum(g.get('actual_executions',0) for g in groups)
    receipt['passed']=receipt['all_groups_collected'] and receipt['source_clean_before_after'] and not errors and all(g['passed'] for g in groups) and receipt['actual_binary_executions']==plan['expected_binary_executions']
    receipt['raw_files_sha256']={str(p.relative_to(out)):sha(p) for p in out.rglob('*') if p.is_file() and p.name!='receipt.json'}
    save(out/'receipt.json',receipt);require(receipt['passed'],'Core512 validation failed; all available groups retained')
    print('PASS core512:16 normal/ASan groups;34 actual executions; unchanged source',flush=True)
if __name__=='__main__':main()
