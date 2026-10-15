#!/usr/bin/env python3
"""Portable execution of the frozen actual Sources owner fixture, no product edits."""
import argparse,hashlib,json,os,platform,re,subprocess,tempfile
from pathlib import Path

p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
p.add_argument('--mode',choices=('normal','asan','both'),default='both');a=p.parse_args()
here=Path(__file__).resolve().parent;source=a.source.resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
watched=['src/streams.c','src/streams.h','src/addons.c','src/addons.h']
before={r:sha(source/r) for r in watched}
records=[];darwin=platform.system()=='Darwin';cc=os.environ.get('CC','cc')
for mode in (('normal','asan') if a.mode=='both' else (a.mode,)):
 for ui in ('tv','mobile'):
  run=out/(mode+'-'+ui);run.mkdir()
  flags=['-O1','-g','-ffunction-sections','-fdata-sections','-I'+str(here.parent/'dispatch/sdk'),'-I'+str(source/'src'),'-I'+str(out),'-Wno-deprecated-declarations','-DSTREAMS_SOURCE="'+str(source/'src/streams.c')+'"','-DADDONS_SOURCE="'+str(source/'src/addons.c')+'"']
  if ui=='mobile':flags+=['-DNV_TOUCH_UI=1','-DNV_ANDROID=1']
  else:flags+=['-DNV_TPK=1']
  if not darwin:flags+=['-DSDLK_i=105']
  san=[] if mode=='normal' else ['-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-sanitize-recover=all']
  if darwin:flags[0:0]=['-I/opt/homebrew/include','-I/opt/homebrew/include/SDL2']
  flags+=san;commands=[]
  def build(cmd,expected=0):
   result=subprocess.run(cmd,text=True,capture_output=True);commands.append({'arguments':cmd,'returncode':result.returncode,'expected_link_probe':expected is None})
   (run/('compile-'+str(len(commands))+'.log')).write_text(result.stdout+result.stderr)
   if expected==0:assert result.returncode==0,result.stderr
   return result
  objects=[]
  for name in ('streams_fixture','queue_fixture','ancillary'):
   obj=run/(name+'.o');build([cc,*flags,'-c',str(here/(name+'.c')),'-o',str(obj)]);objects.append(obj)
  link=[cc,*map(str,objects),'-Wl,-dead_strip' if darwin else '-Wl,--gc-sections','-lm','-lpthread',*san,'-o',str(run/'dispatch')]
  initial=build(link,expected=None)
  pattern=r'^\s*"([^"\n]+)", referenced from:' if darwin else r"undefined reference to [`']([^'`]+)['`]"
  missing=sorted(set(re.findall(pattern,initial.stderr,re.M)))
  assert initial.returncode==0 or missing,initial.stderr
  traps='#include <stdio.h>\n#include <stdlib.h>\n'
  for i,name in enumerate(missing):
   traps+='void nv_unused_'+str(i)+'(void) __asm__('+json.dumps(name)+');\nvoid nv_unused_'+str(i)+'(void) {fprintf(stderr,'+json.dumps('UNEXPECTED dependency: '+name+'\n')+');abort();}\n'
  (run/'unreached-traps.c').write_text(traps)
  build([cc,*flags,'-c',str(run/'unreached-traps.c'),'-o',str(run/'traps.o')]);link.insert(2,str(run/'traps.o'));build(link)
  with tempfile.TemporaryDirectory(prefix='issue513-') as data:
   env=dict(os.environ,NUVIO_DADOS=data,ASAN_OPTIONS='detect_leaks=0:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
   result=subprocess.run([str(run/'dispatch')],env=env,text=True,capture_output=True,timeout=60)
  log=result.stdout+result.stderr;(run/'result.log').write_text(log)
  passes=[line for line in result.stdout.splitlines() if line.startswith('PASS ')]
  assert result.returncode==0 and len(passes)==18 and 'FAIL ' not in result.stdout and 'UNEXPECTED dependency:' not in log,log
  records.append({'mode':mode,'ui':ui,'returncode':0,'assertions':18,'actual_owner_guard':'complete unchanged product streams.c and addons.c',
                  'source_sha256':before,'commands':commands,'sanitizer_flags':san,'trap_symbols':missing,'traps_fired':False,'log_sha256':sha(run/'result.log')})
  print('PASS actual513owner',mode,ui,'18assertions',flush=True)
assert before=={r:sha(source/r) for r in watched}
(out/'report.json').write_text(json.dumps({'outcome':'passed','source':str(source),'source_sha256':before,'fixture_sha256':{n:sha(here/n) for n in ('streams_fixture.c','queue_fixture.c','ancillary.c')},'source_unchanged':True,'records':records},indent=2)+'\n')
