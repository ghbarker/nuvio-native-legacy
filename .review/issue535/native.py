"""Fresh complete streams/cache/audio paths and exact small repository regressions."""
from pathlib import Path
import argparse,json,os,platform,re,shlex,subprocess,sys
from common import HERE,sha,source_binding,record
p=argparse.ArgumentParser()
for key in ['source','proof','output']:p.add_argument('--'+key,type=Path,required=True)
a=p.parse_args();s=a.source.resolve();o=a.output.resolve();o.mkdir(parents=True,exist_ok=False)
plan,v,m=source_binding(s,a.proof);assert platform.system()=='Darwin';cc='/usr/bin/cc';assert Path(cc).is_file()
rows=[];cache=[];modules=[]
for sanitize in [False,True]:
 mode='asan'if sanitize else'normal';d=o/('cache-'+mode);cmd=[sys.executable,str(HERE/'fixtures/run.py'),'--source',str(s),'--output',str(d),'--cc',cc]
 if sanitize:cmd.append('--sanitize')
 rows.append(record(o,'cache-'+mode,cmd,timeout=480));r=json.loads((d/'receipt.json').read_text())
 assert r['outcome']=='passed'and r['source_commit']==m['head']and r['source_tree']==m['tree']and r['source_clean']
 assert r['modes']==['tv','mobile']and r['checks_per_mode']==135 and r['combined_cache_audio_checks_per_mode']==30 and r['existing_offline_resolver_calls_per_mode']==13
 assert r['sanitizers']==(['address','undefined']if sanitize else[])
 for rel,digest in r['product_dependency_sha256'].items():assert sha(s/rel)==digest
 for rel,digest in r['fixture_sha256'].items():assert sha(HERE/'fixtures'/rel)==digest
 assert len(r['records'])==18
 for raw in r['records']:
  assert raw['returncode']==0 and sha(d/raw['log'])==raw['log_sha256']
  command=raw['command'];is_execute=raw['label'].endswith('-execute')
  if is_execute:
   assert sha(Path(command[0]))==raw['binary_sha256']and Path(command[0]).stat().st_size==raw['binary_bytes']
  else:
   assert command[0]==cc
   assert ('-fsanitize=address,undefined'in command)==sanitize
   for token in command:
    if token.startswith('-DNV_STREAMS_SOURCE='):assert token=='-DNV_STREAMS_SOURCE="'+str(s/'src/streams.c')+'"'
 for ui in ['tv','mobile']:
  compiler=[x for x in r['records']if x['label']==ui+'-0-cache_refresh'][0]['command']
  assert ('-DNV_ANDROID=1'in compiler)==(ui=='mobile')and('-DNV_TOUCH_UI=1'in compiler)==(ui=='mobile')
 cache.append({'mode':mode,'receipt':str(d.relative_to(o)/'receipt.json'),'receipt_sha256':sha(d/'receipt.json'),'runtime_groups':2,'retained_checks_each':135,'combined_audio_checks_each':30,'existing_resolver_calls_each':13,'dependency_sha256':r['product_dependency_sha256']})
 # These repository scripts remain unchanged; only typed SDK headers are supplied.
 for suite in ['stream_parser','streamfit_parser','linguas']:
  name=suite+'-'+mode;d=o/name;d.mkdir();trace=d/'compiler';trace.mkdir();bins=d/'bin';bins.mkdir();wrapper=bins/'cc';wrapper.write_bytes((HERE/'module_cc.py').read_bytes());wrapper.chmod(0o755)
  env=dict(os.environ,PATH=str(bins)+os.pathsep+os.environ['PATH'],C_INCLUDE_PATH=str(HERE/'fixtures/sdk'),SANITIZE='1'if sanitize else'0',REVIEW_REAL_CC=cc,REVIEW_CC_OUTPUT=str(trace),ASAN_OPTIONS='detect_leaks=0:abort_on_error=1',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
  if suite!='linguas':cmd=['/bin/bash','tests/'+suite+'.sh']
  else:
   cmd=[str(wrapper),'-std=gnu11','-O1','-g','-Wall','-Wextra','tests/linguas.c','src/linguas.c','-Isrc','-o',str(d/'linguas')]
   if sanitize:cmd[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-sanitize-recover=all']
  raw=record(d,'suite',cmd,cwd=s,env=env,timeout=120)
  if suite=='linguas':rows.append(record(d,'execute',[str(d/'linguas')],cwd=s,env=env,timeout=20))
  traces=list(trace.glob('*.json'));assert len(traces)==1;inv=json.loads(traces[0].read_text());assert inv['returncode']==0 and inv['compiler']==cc and inv['added_linux_flags']==[] and inv['effective_arguments']==inv['original_arguments']
  flags=inv['original_arguments'];assert any(x.startswith('-fsanitize=address,undefined')for x in flags)==sanitize
  if suite!='linguas':assert '-fno-sanitize-recover=all'in flags if sanitize else True
  expected={'tests/linguas.c','src/linguas.c'}if suite=='linguas'else{'tests/'+suite+'.c','src/stream_parse.c','src/js.c','src/badges.c'}
  assert {x for x in flags if x.endswith('.c')}==expected
  output=(d/'suite.log').read_text()+(d/'execute.log').read_text()if suite=='linguas'else(d/'suite.log').read_text()
  marker={'linguas':'linguas ok','stream_parser':'PASS ASan/UBSan: parser isolado, 100 fontes, MP4/DV na última posição.','streamfit_parser':'streamfit parser: PASS (33 exact-size/provenance cases)'}[suite]
  assert output.splitlines().count(marker)==1,(name,output)
  assert not re.search(r'ERROR: AddressSanitizer|runtime error:|SUMMARY: (AddressSanitizer|UndefinedBehaviorSanitizer)',output)
  modules.append({'suite':suite,'mode':mode,'script_unchanged':suite!='linguas','compiler_receipt':str(traces[0].relative_to(o)),'compiler_receipt_sha256':sha(traces[0]),'binary_sha256':inv['binary_sha256'],'binary_bytes':inv['binary_bytes'],'record':raw,'success_marker':marker})
source_binding(s,a.proof)
assert len(cache)==2 and len(modules)==6
result={'outcome':'passed','source_binding':v,'plan_sha256':sha(HERE/'plan.json'),'compiler':subprocess.check_output([cc,'--version'],text=True).splitlines()[0],'cache_groups':cache,'module_groups':modules,'records':rows,'runtime_groups':10,'scope':'Complete production streams.c + real parser/language/cache code through typed offline GPU/provider boundaries, TV/mobile normal and ASan/UBSan. Original135 checks and13 resolver calls preserved per mode;30 new combined cache/audio checks. Three current small repository parser/language suites normal+ASan. No full app build, real provider network, quota, device playback or SDK build in this job.'}
(o/'native-verified.json').write_text(json.dumps(result,indent=2)+'\n');print('PASS current cache135+audio30 TV/mobile normal+ASan and6 exact small-module groups',flush=True)
