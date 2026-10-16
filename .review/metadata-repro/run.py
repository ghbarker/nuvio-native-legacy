#!/usr/bin/env python3
"""Confirm exact baseline regressions; successful CI never means a product fix."""
import argparse,hashlib,json,os,pathlib,re,resource,shutil,subprocess,sys,time

def sha(p): return hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest()
def git(*a): return subprocess.check_output(['git',*a],text=True).strip()
def require(ok,msg):
    if not ok: raise AssertionError(msg)
p=argparse.ArgumentParser();p.add_argument('--output',required=True);a=p.parse_args()
root=pathlib.Path.cwd(); kit=pathlib.Path(__file__).resolve().parent
plan=json.loads((kit/'plan.json').read_text()); out=pathlib.Path(a.output).resolve();out.mkdir(parents=True,exist_ok=True)
require(plan['root_approved'] and plan['execution_enabled'],'root execution gates closed')
require(sha(kit/'plan.json')==os.environ['METADATA_REPRO_PLAN_SHA256'],'plan mismatch')
head=git('rev-parse','HEAD');require(head==os.environ['GITHUB_SHA'],'actual review mismatch')
require(git('rev-parse','HEAD^')==plan['parent'],'review must directly descend from frozen #391 test commit')
parent_text=git('cat-file','-p',plan['parent'])
require('parent '+plan['parent_parent']+'\n' in parent_text,'391 frozen test ancestry mismatch')
require(not git('status','--porcelain','--untracked-files=all'),'dirty review before execution')
changed=set(git('diff','--name-only',plan['parent'],'HEAD').splitlines())
require(changed==set(plan['review_paths']),'unexpected review/source delta: '+repr(changed))
for name,h in plan['recipe_sha256'].items(): require(sha(kit/name)==h,'recipe hash '+name)
for name,d in plan['fixture_origins'].items():
    require(git('rev-parse','HEAD:'+name)==d['blob'] and sha(root/name)==d['sha256'],'fixture provenance '+name)
# No original production or build file can differ from the baseline. The only
# added Git paths are the exact public fixtures and this review-only kit.
require(all(x.startswith('tests/') or x.startswith('.review/') or x==plan['workflow'] for x in changed),'production delta')
compiler=shutil.which('clang');require(compiler is not None,'real clang missing')
wrapper=out/'toolchain';wrapper.mkdir(exist_ok=True)
cc=wrapper/'cc';cc.write_text('#!/bin/sh\nexec "'+sys.executable+'" "'+str(kit/'cc_record.py')+'" "$@"\n');cc.chmod(0o755)
records=[]; errors=[]; deadline=time.monotonic()+900
controls391=[('live-nuvio',176),('live-cinemeta',178),('live-nuvio-rebuild',176),('live-cinemeta-rebuild',178)]

def invoke(label,script,args=(),san=False,extra=None,expected_exit=0,markers=(),red=False,required=(),forbidden=()):
    d=out/label;d.mkdir()
    env=os.environ.copy();env.update(PATH=str(wrapper)+os.pathsep+env['PATH'],CC=str(cc),REPRO_SOURCE=str(root),REPRO_REAL_CC=compiler,REPRO_CC_PROOF=str(d/'compiler'),SANITIZE='1' if san else '0',PYTHONDONTWRITEBYTECODE='1',ASAN_OPTIONS='detect_leaks=0:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
    if extra:env.update(extra)
    start=time.monotonic();command=['bash',script,*args]
    r={'label':label,'command':command,'sanitized':san,'expected_exit_code':expected_exit,'expected_baseline_failure':red,'verified':False,'expected_failure_verified':False}
    records.append(r);c=None;log='';phase='fixture_preprocess_or_harness_failure'
    try:
        def no_core():resource.setrlimit(resource.RLIMIT_CORE,(0,0))
        require(deadline>time.monotonic(),'total 15-minute diagnostic budget exhausted')
        try:
            q=subprocess.run(command,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,timeout=min(240,deadline-time.monotonic()),preexec_fn=no_core)
            log=q.stdout;r['exit_code']=q.returncode
        except subprocess.TimeoutExpired as e:
            log=e.stdout or ''
            if isinstance(log,bytes):log=log.decode(errors='replace')
            r['timed_out']=True;phase='bounded_timeout'
            raise AssertionError('public script timeout')
        finally:
            (d/'run.log').write_text(log)
            r['run_log_sha256']=sha(d/'run.log')
        cp=d/'compiler'/'receipt.json'
        require(cp.is_file(),'missing real compiler receipt')
        c=json.loads(cp.read_text());r.update(compiler_receipt_sha256=sha(cp),compiler=c)
        phase='fixture_compile_failure'
        require(c['exit_code']==0 and c['dependency_exit_code']==0,'real compile/link failed')
        phase='compiler_proof_failure'
        require(c['source_review']==head,'compiler source mismatch')
        require(('-fsanitize=address,undefined' in c['command'])==san,'sanitizer compile mode mismatch')
        require(sha(d/'compiler'/'compile.log')==c['compile_log_sha256'] and sha(d/'compiler'/'dependencies.log')==c['dependency_log_sha256'],'raw compiler hashes')
        phase='sanitizer_failure'
        require(not re.search(r'(ERROR: (?:AddressSanitizer|LeakSanitizer)|runtime error:|UndefinedBehaviorSanitizer)',log),'unexpected sanitizer diagnostic')
        phase='unexpected_functional_result'
        require(r['exit_code']==expected_exit,'unexpected exit '+str(r['exit_code']))
        for marker,count in markers:require(log.count(marker)==count,'exact marker count '+repr(marker))
        for marker in required:require(marker in log,'missing signature '+repr(marker))
        for marker in forbidden:require(marker not in log,'unexpected signature '+repr(marker))
        r.update(verified=True,expected_failure_verified=red,classification='expected_baseline_red' if red else 'control_pass')
        print(label+': '+('EXPECTED BASELINE RED' if red else 'CONTROL PASS'),flush=True)
    except Exception as e:
        r.update(classification=phase,error=str(e))
        errors.append({'label':label,'classification':phase,'error':str(e)})
        print(label+': FAIL ['+phase+'] '+str(e)+'\n'+log[-1500:],flush=True)
    finally:
        r['seconds']=time.monotonic()-start
        (d/'group-verdict.json').write_text(json.dumps(r,indent=2)+'\n')
    return log

for san in (False,True):
    mode='asan' if san else 'normal'
    invoke('391-reported-title-'+mode,'tests/episode_report391.sh',san=san,markers=[('episode_report391: '+case+': '+str(total)+' episodes, 7 season tabs, all 22 season-2 ids preserved',1) for case,total in controls391])
    for case,marker in [('movie-control','result id=tt1629757 type=movie title=Chasing Mavericks'),('known-series-control','series detail title=That Time I Got Reincarnated as a Slime type=series episodes=2 movie-requested=0'),('uncertain-detail-control','uncertain detail title=That Time I Got Reincarnated as a Slime type=series episodes=2 movie-requested=0')]:
        invoke('524-'+case+'-'+mode,'tests/title_identity524.sh',[case],san=san,markers=[(marker,1)])
    log=invoke('512-controls-and-mapping-red-'+mode,'tests/trakt_episode_identity_512.sh',san=san,expected_exit=134,red=True,markers=[('trakt_episode_identity_512: PASS',4),('#512 mode=mapped actual=S01E14 expected=S02E02 progress=40',1),('v[0].temporada == esperadoT && v[0].episodio == esperadoE',1)],required=['#512 mode='+case+' actual=' for case in ['normal','missing','duplicate','offline']])
for case in ['tmdb-tv','episode-id']:
    log=invoke('524-'+case+'-red','tests/title_identity524.sh',[case],expected_exit=1,red=True,markers=[('result id=tt9054364 type=movie title=Chasing Mavericks',1),('request https://v3-cinemeta.strem.io/meta/movie/tt9054364.json',1)],forbidden=['result id=tt9054364 type=series'])
source_clean=not git('status','--porcelain','--untracked-files=all')
if not source_clean:errors.append({'label':'source','classification':'source_changed','error':'dirty review after execution'})
if len(records)!=plan['group_count']:errors.append({'label':'groups','classification':'incomplete_groups','error':str(len(records))})
receipt={'reproduction_confirmed':not errors,'product_fixed':False,'source_review':head,'source_tree':git('rev-parse','HEAD^{tree}'),'production_baseline':plan['base'],'parent':plan['parent'],'run_id':os.environ['GITHUB_RUN_ID'],'run_attempt':os.environ['GITHUB_RUN_ATTEMPT'],'plan_sha256':sha(kit/'plan.json'),'reported_title391_controls_only':True,'synthetic391_merge_policy_probe_executed':False,'regression524':'known series identity resolves to wrong movie','regression512':'stable episode TVDB identity not mapped to catalog numbering','caveat512':'tracking playback response is synthetic; catalog episode objects are reduced public data','groups':records,'source_unchanged':source_clean,'errors':errors,'all_groups_collected':len(records)==plan['group_count']}
receipt['raw_files']={str(f.relative_to(out)):sha(f) for f in sorted(out.rglob('*')) if f.is_file() and f.name!='verified.json'}
(out/'verified.json').write_text(json.dumps(receipt,indent=2)+'\n')
if errors:
    print('FAIL: '+str(len(errors))+' diagnostic group/proof errors; no complete reproduction claim',flush=True)
    sys.exit(1)
print('PASS: precise #524 and #512 baseline regressions reproduced; controls pass; no #391 bug claimed',flush=True)
