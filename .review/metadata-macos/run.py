#!/usr/bin/env python3
"""Run one source-bound macOS matrix lane, retaining failures as failures."""
from pathlib import Path
import argparse,concurrent.futures,hashlib,json,os,platform,re,shlex,signal,subprocess,sys,time,traceback

def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def git(*args):return subprocess.check_output(['git',*args],text=True).strip()
def save(path,value):Path(path).write_text(json.dumps(value,indent=2)+'\n')
def require(ok,message):
    if not ok:raise AssertionError(message)
def binding(plan,kit):
    require(plan['source_frozen'] and plan['root_approved'] and plan['execution_enabled'],'root/source gates closed')
    require(sha(kit/'plan.json')==os.environ['METADATA_PLAN_SHA256'],'plan hash mismatch')
    require(platform.system()=='Darwin','macOS required')
    require(git('rev-parse','HEAD')==os.environ['GITHUB_SHA'],'actual remote review HEAD mismatch')
    require(git('rev-list','--parents','-n','1','HEAD').split()[1:]==[plan['base']],'review must directly parent approved base')
    require(os.environ['GITHUB_REF']=='refs/heads/'+plan['branch'],'branch mismatch')
    expected=set(plan['source_files'])|set(plan['review_paths'])
    require(set(git('diff','--name-only',plan['base'],'HEAD').splitlines())==expected,'unexpected source or recipe delta')
    for name,fact in plan['source_files'].items():
        if fact.get('deleted'):
            require(not Path(name).exists(),'deleted source reappeared: '+name);continue
        entry=git('ls-tree','HEAD','--',name).split('\t')[0].split()
        require(entry==[fact['git_mode'],'blob',fact['git_blob']] and sha(name)==fact['sha256'],'source blob/mode/hash mismatch: '+name)
    for name,digest in plan['recipe_sha256'].items():require(sha(kit/name)==digest,'recipe mismatch: '+name)
    require(not git('status','--porcelain','--untracked-files=all'),'source checkout changed')
def run(command,log,env,timeout):
    started=time.monotonic()
    with log.open('wb') as stream:
        child=subprocess.Popen(command,env=env,stdout=stream,stderr=subprocess.STDOUT,start_new_session=True)
        try:code=child.wait(timeout=timeout);timed_out=False
        except subprocess.TimeoutExpired:
            # A genuine SDL/dyld startup stall is a failed runtime, never a pass.
            sample=subprocess.run(['sample',str(child.pid),'1','1'],stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
            log.with_suffix('.sample.txt').write_bytes(sample.stdout)
            os.killpg(child.pid,signal.SIGKILL);child.wait();code=124;timed_out=True
    return {'command':command,'returncode':code,'timed_out':timed_out,'seconds':time.monotonic()-started,'log_sha256':sha(log)}
def main():
    ap=argparse.ArgumentParser();ap.add_argument('--lane',required=True);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args()
    kit=Path(__file__).resolve().parent;plan=json.loads((kit/'plan.json').read_text());binding(plan,kit)
    lane=next(x for x in plan['lanes'] if x['id']==a.lane);out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
    prefix=Path(subprocess.check_output(['brew','--prefix'],text=True).strip())
    sdl3=Path(subprocess.check_output(['brew','--prefix','sdl3'],text=True).strip())/'lib'
    tools=out/'tools';tools.mkdir();wrapper=tools/'cc'
    wrapper.write_text('#!/bin/sh\nexec '+shlex.quote(sys.executable)+' '+shlex.quote(str(kit/'cc.py'))+' "$@"\n');wrapper.chmod(0o755)
    env=dict(os.environ,PATH=str(tools)+os.pathsep+os.environ['PATH'],CC=str(wrapper),METADATA_REAL_CC='/usr/bin/cc',
        METADATA_RPATHS=os.pathsep.join([str(prefix/'lib'),str(sdl3)]),PYTHONDONTWRITEBYTECODE='1',
        ASAN_OPTIONS='detect_leaks=0:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1',
        SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy',NUVIO_CFLAGS='-I'+str(prefix/'include')+' -I'+str(prefix/'include/SDL2'))
    results=[];receipt={'passed':False,'review_commit':os.environ['GITHUB_SHA'],'base':plan['base'],
        'local_candidate_provenance':plan['local_candidate_provenance'],'lane':lane,'plan_sha256':sha(kit/'plan.json'),
        'workflow_run_id':os.environ['GITHUB_RUN_ID'],'workflow_run_attempt':os.environ['GITHUB_RUN_ATTEMPT'],
        'compiler':subprocess.check_output(['/usr/bin/cc','--version'],text=True),'results':results}
    save(out/'receipt.json',receipt)
    def public_test(spec):
        d=out/spec['name'];d.mkdir();proof=d/'compiler';env_test=dict(env,METADATA_CC_PROOF=str(proof),METADATA_WRAP_RUNTIME='1',SANITIZE='1' if lane['mode']=='asan' else '0')
        fact={'name':spec['name'],'passed':False};results.append(fact)
        try:
            fact.update(run(['/bin/bash',spec['script']],d/'run.log',env_test,spec.get('timeout_seconds',720)))
            text=(d/'run.log').read_text(errors='replace');require(fact['returncode']==0,'public script failed')
            require(not re.search(r'ERROR: (?:AddressSanitizer|LeakSanitizer)|runtime error:|SUMMARY: .*Sanitizer',text),'sanitizer diagnostic')
            for marker,count in spec['markers'].items():require(text.count(marker)==count,'completion marker mismatch: '+marker)
            compiles=[json.loads(p.read_text()) for p in proof.glob('*-compiler.json')]
            source_compiles=[x for x in compiles if x.get('dependencies')];links=[x for x in compiles if x.get('binary_sha256')]
            require(len(source_compiles)==spec['compile_count'] and len(links)==1,'actual compiler/link count mismatch')
            for x in source_compiles:
                require(x['returncode']==0 and x['source_review']==os.environ['GITHUB_SHA'],'actual compile failed/source changed')
                flags={s for flag in x['arguments'] if flag.startswith('-fsanitize=') for s in flag.split('=',1)[1].split(',')}
                require({'address','undefined'}<=flags if lane['mode']=='asan' else not flags,'actual sanitizer flags mismatch')
            executions=[json.loads(p.read_text()) for p in sorted(proof.glob('*-runtime.json'))]
            require([x['arguments'] for x in executions]==spec['arguments'],'actual execution arguments/count mismatch')
            require(all(x['returncode']==0 and not x['timed_out'] and x['binary_sha256']==links[0]['binary_sha256'] for x in executions),'actual runtime failed')
            fact.update(passed=True,compiler_count=len(source_compiles),actual_executions=len(executions),binary_sha256=links[0]['binary_sha256'])
        except Exception as exc:
            fact.update(error=str(exc),traceback=traceback.format_exc());print(spec['name']+': FAIL '+str(exc),flush=True)
        else:print(spec['name']+': PASS',flush=True)
        save(d/'verdict.json',fact);save(out/'receipt.json',receipt)
    if lane['kind']=='tests':
        for name in lane['suites']:public_test(plan['suites'][name])
    else:
        # This is a real compile and link of every translation unit, with no GUI runtime claim.
        env_app=dict(env,METADATA_CC_PROOF=str(out/'compiler'),METADATA_WRAP_RUNTIME='0');objects=out/'objects';objects.mkdir()
        flags=['-std=gnu11','-O1','-g','-Wno-deprecated-declarations','-I'+str(prefix/'include'),'-I'+str(prefix/'include/SDL2'),'-Isrc']
        flags+=['-D'+name+'=""' for name in ['NV_SUPABASE_URL','NV_SUPABASE_ANON_KEY','NV_TV_LOGIN_BASE','NV_TRAKT_CLIENT_ID','NV_TRAKT_CLIENT_SECRET','NV_SIMKL_CLIENT_ID','NV_SIMKL_APP','NV_TMDB_API_KEY']]
        if lane['ui']=='touch':flags+=['-DNV_TOUCH_UI=1']
        def one(pair):
            index,path=pair;obj=objects/(str(index)+'.o');fact=run([str(wrapper),*flags,'-c',path,'-o',str(obj)],objects/(str(index)+'.log'),env_app,180)
            require(fact['returncode']==0,'full app compile failed: '+path);return str(obj)
        try:
            with concurrent.futures.ThreadPoolExecutor(max_workers=3) as pool:built=list(pool.map(one,enumerate(plan['translation_units'])))
            binary=out/('app-'+lane['ui']);fact=run([str(wrapper),*built,'-L'+str(prefix/'lib'),'-lSDL2','-lSDL2_image','-lSDL2_ttf','-lz','-framework','OpenGL','-o',str(binary)],out/'link.log',env_app,180)
            require(fact['returncode']==0,'full app link failed')
            results.append({'name':'full-'+lane['ui'],'passed':True,'translation_units':len(built),'binary_sha256':sha(binary),'bytes':binary.stat().st_size})
        except Exception as exc:results.append({'name':'full-'+lane['ui'],'passed':False,'error':str(exc),'traceback':traceback.format_exc()})
    binding(plan,kit);receipt['passed']=bool(results) and all(x['passed'] for x in results)
    receipt['raw_files_sha256']={str(p.relative_to(out)):sha(p) for p in out.rglob('*') if p.is_file() and p.name!='receipt.json' and p.suffix!='.o' and not p.name.startswith('app-')}
    save(out/'receipt.json',receipt);require(receipt['passed'],'one or more actual checks failed')
if __name__=='__main__':main()
