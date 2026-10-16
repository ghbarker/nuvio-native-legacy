#!/usr/bin/env python3
"""Validate the exact #524 overlay; collect every actual normal/ASan group."""
import argparse,hashlib,json,os,re,resource,shlex,shutil,signal,subprocess,sys,time,traceback
from pathlib import Path

def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def git(*args):return subprocess.check_output(['git',*args],text=True).strip()
def save(path,value):Path(path).write_text(json.dumps(value,indent=2)+'\n')
def require(condition,message):
    if not condition:raise AssertionError(message)
BAD=r'ERROR: (?:AddressSanitizer|LeakSanitizer)|UndefinedBehaviorSanitizer|runtime error:|Assertion .* failed|Segmentation fault|Aborted'

def run_bounded(command,env,timeout):
    process=subprocess.Popen(command,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,
        text=True,start_new_session=True)
    try:
        output,_=process.communicate(timeout=timeout)
        return process.returncode,output,False
    except subprocess.TimeoutExpired:
        os.killpg(process.pid,signal.SIGKILL)
        output,_=process.communicate()
        return process.returncode,output,True

def verify_runtime(suite,case,text):
    if suite=='title_identity524':
        if case in ('tmdb-tv','episode-id'):
            require(text.count('result id=tt9054364 type=series title=That Time I Got Reincarnated as a Slime')==1,'wrong series lookup result')
            require('request https://v3-cinemeta.strem.io/meta/movie/tt9054364.json' not in text,'wrong movie lookup fallback')
    elif suite=='detalheanime':
        require(text.count('detalheanime: tudo ok\n')==1,'metadata completion marker missing')
    elif suite=='player_identity524':
        movie=case in ('movie-control','late-movie-control')
        expected_id='tt1629757' if movie else 'tt9054364'
        parent=2 if case=='next-opening-control' else 1
        refresh=2 if case in ('next-opening-control','retained-series-control') else 1
        require(text.count('boundaries parental=%d id=%s refresh=%d\n'%(parent,expected_id,refresh))==1,'player boundaries mismatch')
        episode=1 if case=='known-series-default' else 3 if case=='next-opening-control' else 2
        key='tt1629757' if movie else 'tt9054364_s1e%d'%episode
        require('progress expected=%s exists=1 type=%s;'%(key,'movie' if movie else 'series') in text,'real progress missing or wrong type')
        payloads=[json.loads(line[len('stats payload '):]) for line in text.splitlines() if line.startswith('stats payload ')]
        require(len(payloads)==1,'actual serialized stats payload count mismatch')
        payload=payloads[0]
        if movie:
            require(payload.get('movie',{}).get('ids',{}).get('imdb')==expected_id and 'show' not in payload,'movie stats mismatch')
        else:
            require(payload.get('show',{}).get('ids',{}).get('imdb')==expected_id and
                payload.get('episode',{}).get('season')==1 and payload['episode'].get('number')==episode and 'movie' not in payload,'series stats mismatch')
            require('series bare-movie progress exists=0' in text,'false bare movie progress exists')
        if case=='retained-series-control':require(text.count('retained source and episode unchanged without another load or seek\n')==1,'retained control missing')
        if case=='next-opening-control':require(text.count('next opening keeps its new source and explicit episode\n')==1,'next control missing')
    elif suite=='player_identity524_guards':
        require(text.count('guard '+case+' actual=')==1,'guard execution marker missing')
        promote=case in ('pending-before-meta','reordered-series','fresh-episode-source')
        movie=case=='known-movie-conflict'
        actual='1:2' if promote else '0:0'
        require('guard %s actual=%s expected=%s'%(case,actual,actual) in text,'guard episode mismatch')
        marker=re.findall(r'^guard boundaries parental=(\d+) id=([^ ]+) refresh=(\d+) tracker=(\d+) bare-progress=(\d+) episode-progress=(\d+)$',text,re.M)
        require(len(marker)==1,'guard boundary marker count mismatch')
        parent,identity,refresh,tracker,bare,episode=marker[0]
        require(parent=='1' and identity==('tt1629757' if movie else 'tt9054364'),'guard parental identity mismatch')
        require(bare=='0' and episode==('1' if promote else '0'),'guard persistence mismatch')
        require(refresh==tracker==('1' if promote or movie else '0'),'guard remote boundary mismatch')
        if promote:require('type=series title=Frozen original title poster=https://fixture.invalid/frozen.jpg' in text,'frozen series metadata changed')
        if movie:
            require('type=movie title=Frozen original title poster=https://fixture.invalid/frozen.jpg' in text,'frozen movie metadata changed')
            require('progresso local ignorado: sem copia do tipo movie para tt1629757' in text,'contradictory local-write guard missing')

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--group',choices=('short','player'),required=True);parser.add_argument('--output',required=True)
    args=parser.parse_args();root=Path.cwd().resolve();kit=Path(__file__).resolve().parent
    out=Path(args.output).resolve();out.mkdir(parents=True,exist_ok=True)
    plan=json.loads((kit/'plan.json').read_text());head=git('rev-parse','HEAD')
    require(plan['root_approved'] and plan['execution_enabled'],'root launch gates are closed')
    require(sha(kit/'plan.json')==os.environ['IDENTITY524_FINAL_PLAN_SHA256'],'plan digest mismatch')
    require(head==os.environ['GITHUB_SHA'],'workflow review mismatch')
    require(os.environ['GITHUB_REF']=='refs/heads/'+plan['branch'],'workflow branch mismatch')
    require(git('rev-parse','HEAD^')==plan['base'],'composite must directly parent frozen base49a')
    require(set(git('diff','--name-only',plan['base'],'HEAD').splitlines())==set(plan['source_files'])|set(plan['review_paths']),'unexpected source or recipe delta')
    for name,fact in plan['source_files'].items():
        entry=git('ls-tree','HEAD','--',name).split('\t')[0].split()
        require(entry==[fact['git_mode'],'blob',fact['git_blob']] and sha(root/name)==fact['sha256'],'source blob/mode/hash mismatch: '+name)
    for name,expected in plan['recipe_sha256'].items():require(sha(kit/name)==expected,'recipe digest mismatch: '+name)
    require(not git('status','--porcelain','--untracked-files=all'),'dirty source before execution')
    compiler=shutil.which('clang');require(compiler is not None,'real clang missing')
    resource.setrlimit(resource.RLIMIT_CORE,(0,0));require(resource.getrlimit(resource.RLIMIT_AS)==(-1,-1),'address space must be uncapped')
    bindir=out/'toolchain';bindir.mkdir();cc=bindir/'cc'
    cc.write_text('#!/bin/sh\nexec '+shlex.quote(sys.executable)+' '+shlex.quote(str(kit/'cc_record.py'))+' "$@"\n');cc.chmod(0o755)
    suites=[s for s in plan['suites'] if s['group']==args.group]
    groups=[];errors=[];deadline=time.monotonic()+1200
    receipt={'review_commit':head,'source_base':plan['base'],'local_candidate_provenance':plan['local_candidate_provenance'],
        'source_binding':'Exact14 overlay blobs/modes plus4 recipe files on direct frozen base49a; local SHA is provenance only',
        'plan_sha256':sha(kit/'plan.json'),'source_files':plan['source_files'],'workflow_run_id':os.environ['GITHUB_RUN_ID'],
        'workflow_run_attempt':os.environ['GITHUB_RUN_ATTEMPT'],'group':args.group,'groups':groups,'errors':errors,
        'passed':False,'sdk_or_device_validated':False,'all524_jobs_validated':False}
    save(out/'receipt.json',receipt)
    for mode in plan['modes']:
        for suite in suites:
            label=suite['name']+'-'+mode;directory=out/label;directory.mkdir();proof=directory/'compiler'
            group={'label':label,'mode':mode,'suite':suite['name'],'command':['bash',suite['script']],'passed':False}
            groups.append(group);started=time.monotonic();log=''
            env=dict(os.environ,PATH=str(bindir)+os.pathsep+os.environ['PATH'],CC=str(cc),IDENTITY524_SOURCE=str(root),
                IDENTITY524_CC_PROOF=str(proof),IDENTITY524_REAL_CC=compiler,IDENTITY524_CC_PHASES=str(suite['compile_phases']),
                SANITIZE='1' if mode=='asan' else '0',SDL_VIDEODRIVER='dummy',ASAN_OPTIONS='detect_leaks=0:halt_on_error=1',
                UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1',PYTHONDONTWRITEBYTECODE='1')
            try:
                require(deadline>time.monotonic(),'job validation budget exhausted')
                result,log,timed_out=run_bounded(group['command'],env,min(suite['timeout'],deadline-time.monotonic()))
                group.update(script_returncode=result,timed_out=timed_out)
                (directory/'run.log').write_text(log);group['log_sha256']=sha(directory/'run.log')
                require(not timed_out,'bounded public-script timeout')
                require(result==0,'public script failed: '+str(result))
                require(not re.search(BAD,log),'sanitizer, assertion or signal diagnostic')
                records=[json.loads((proof/('compiler-%d.json'%phase)).read_text()) for phase in range(1,suite['compile_phases']+1)]
                require(len(list(proof.glob('compiler-*.json')))==len(records),'unexpected actual compile count')
                for phase,fact in enumerate(records,1):
                    require(fact['returncode']==0 and fact['dependency_exit_code']==0 and fact['elf'],'actual compilation failed')
                    require(fact['source_review']==head and fact['phase']==phase and fact['phase_count']==suite['compile_phases'],'compiler source/phase mismatch')
                    require(('-fsanitize=address,undefined' in fact['command'])==(mode=='asan'),'sanitizer mode mismatch')
                    require(('-c' in fact['command'])==(phase<len(records)),'actual compiler phase mismatch')
                    require(sha(proof/('compiler-%d.log'%phase))==fact['compile_log_sha256'] and
                        sha(proof/('compiler-%d.dependencies.log'%phase))==fact['dependency_log_sha256'],'raw compiler digest mismatch')
                    for name,dependency in fact['dependencies'].items():
                        changed=plan['source_files'].get(name)
                        blob=changed['git_blob'] if changed else git('rev-parse',plan['base']+':'+name)
                        require(dependency['git_blob']==blob and sha(root/name)==dependency['sha256'],'actual dependency mismatch: '+name)
                    for path,digest in fact['direct_inputs'].items():
                        original=Path(path)
                        if original.suffix=='.c':require(sha(original)==digest and str(original.relative_to(root)) in fact['dependencies'],'direct C input mismatch')
                dependencies=set().union(*(set(record['dependencies']) for record in records))
                require(set(suite['required_dependencies'])<=dependencies,'required actual sources missing')
                if len(records)==3:
                    require(set(records[0]['direct_inputs'])=={str(root/'src/video.c')},'video phase source mismatch')
                    require(set(records[1]['direct_inputs'])=={str(root/'src/player.c')},'player phase source mismatch')
                    expected={str(p) for p in root.glob('src/*.c') if p.name not in ('main.c','player.c','video.c')}
                    expected|={str(p) for p in root.glob('src/dts/*.c')};expected.add(str(root/suite['fixture']))
                    require({p for p in records[-1]['direct_inputs'] if p.endswith('.c')}==expected,'full-source final link graph mismatch')
                    require({Path(p).name:d for p,d in records[-1]['direct_inputs'].items() if p.endswith('.o')}==
                        {'video.o':records[0]['output_sha256'],'player.o':records[1]['output_sha256']},'real intermediate object hash mismatch')
                runs=sorted(proof.glob('runtime-*.json'));require(len(runs)==len(suite['arguments']),'actual execution count mismatch')
                for path,arguments in zip(runs,suite['arguments']):
                    actual=json.loads(path.read_text());require(actual['arguments']==arguments and actual['returncode']==0,'actual scenario failed: '+repr(arguments))
                    require(actual['binary_sha256']==records[-1]['output_sha256'] and sha(path.with_suffix('.log'))==actual['log_sha256'],'executed binary/log hash mismatch')
                    require(actual['runtime_limits']['address_space']==[-1,-1] and actual['runtime_limits']['core']==[0,0],'actual runtime limits mismatch')
                    text=path.with_suffix('.log').read_text();require(not re.search(BAD,text),'actual runtime diagnostic')
                    verify_runtime(suite['name'],arguments[0] if arguments else '',text)
                if suite['arguments']!=[[]]:
                    require(re.findall(r'^PASS (.+)$',log,re.M)==[a[0] for a in suite['arguments']],'ordered scenario pass markers mismatch')
                    require(not re.search(r'^FAIL ',log,re.M),'functional failure marker')
                require(not git('status','--porcelain','--untracked-files=all'),'source modified during execution')
                group.update(passed=True,actual_executions=len(runs),actual_compile_phases=len(records),binary_sha256=records[-1]['output_sha256'])
                print(label+': PASS ('+str(len(runs))+' actual executions)',flush=True)
            except Exception as exc:
                if not (directory/'run.log').exists():(directory/'run.log').write_text(log)
                group['error']={'type':type(exc).__name__,'message':str(exc),'traceback':traceback.format_exc()};errors.append({'label':label,**group['error']})
                print(label+': FAIL '+str(exc)+'\n'+log[-2000:],flush=True)
            finally:
                group['seconds']=time.monotonic()-started;save(directory/'verdict.json',group);save(out/'receipt.json',receipt)
    expected=plan['expected_by_group'][args.group]
    receipt['all_groups_collected']=len(groups)==expected['groups'];receipt['source_clean_before_after']=not git('status','--porcelain','--untracked-files=all')
    receipt['actual_binary_executions']=sum(group.get('actual_executions',0) for group in groups)
    receipt['passed']=receipt['all_groups_collected'] and receipt['source_clean_before_after'] and not errors and all(group['passed'] for group in groups) and receipt['actual_binary_executions']==expected['executions']
    receipt['raw_files_sha256']={str(p.relative_to(out)):sha(p) for p in out.rglob('*') if p.is_file() and p.name!='receipt.json'}
    save(out/'receipt.json',receipt);require(receipt['passed'],'positive524 validation failed; all available group evidence retained')
    print('PASS identity524 '+args.group+': normal+ASan groups and all actual executions verified',flush=True)

if __name__=='__main__':main()
