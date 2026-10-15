"""Run only actual startup, profile ownership, progress and Home-window contracts."""
from pathlib import Path
import argparse,json,os,platform,subprocess,sys
from common import HERE,sha,source_binding,record
p=argparse.ArgumentParser()
for key in ['source','proof','output']:p.add_argument('--'+key,type=Path,required=True)
a=p.parse_args();s=a.source.resolve();o=a.output.resolve()
assert not o.exists();o.mkdir(parents=True)
plan,v,m=source_binding(s,a.proof);assert platform.system()=='Linux'
cc='/usr/bin/cc';rows=[];contracts=[]
def capture(directory,label,command,**kwargs):
 row=record(directory,label,command,**kwargs);row['log']=str((directory/row['log']).relative_to(o));return row
profile_cases=['private-state','sync-cycle-identity','cw-session-race','cwremover','herosinopse_corrida','localizar_corrida','discovery-reopen','discovery-reopen-boot-first','discovery-reopen-signedout-bootstrap']
profile_bins=profile_cases[:7]
flags=['-std=gnu11','-D_GNU_SOURCE','-O1','-g','-Isrc','-Itests','-I'+str(HERE/'startup/sdk'),'-pthread','-ffunction-sections','-fdata-sections']
for mode in ['normal','asan']:
 d=o/mode;d.mkdir();san=mode=='asan'
 env=dict(os.environ,SANITIZE=str(int(san)),ASAN_OPTIONS='detect_leaks=0:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
 startup=d/'startup'
 rows.append(capture(d,'startup',[sys.executable,str(HERE/'startup/run.py'),'--source',str(s),'--output',str(startup),'--cc',cc]+(['--sanitize']if san else[]),env=env,timeout=420))
 r=json.loads((startup/'receipt.json').read_text())
 assert r['passed'] and r['source_commit']==m['head'] and r['source_tree']==m['tree']
 assert r['sanitizers']==(['address','undefined']if san else[]) and r['groups']==['discovery','sync']
 contracts.append({'kind':'startup','mode':mode,'cases':7,'receipt':str((startup/'receipt.json').relative_to(o)),'sha256':sha(startup/'receipt.json')})
 # Existing nine assertion scenarios stay byte-for-byte intact. Only a compile
 # wrapper adds the proven non-PIE sanitizer flags on Linux.
 ccbin=d/'cc-bin';ccbin.mkdir();wrapper=ccbin/'cc'
 wrapper.write_text('#!/bin/sh\nexec /usr/bin/cc '+('-fno-pie -no-pie 'if san else'')+'"$@"\n');wrapper.chmod(0o755)
 data_root=d/'profile-data';data_env=dict(env,NUVIO_PROFILE_RESULTS=str(data_root),NUVIO_SYNTAX_SDK=str(HERE/'startup/sdk'),PATH=str(ccbin)+os.pathsep+os.environ['PATH'])
 rows.append(capture(d,'profile-data',['/bin/bash','-x',str(HERE/'profile-data/run.sh'),str(s)],env=data_env,timeout=420))
 trace=(d/'profile-data.log').read_text()
 for name in profile_cases:assert name+': PASS'in trace,name
 assert ('-fsanitize=address,undefined'in trace)==san
 data=data_root/'data'
 details={'passed':True,'source_binding':v,'mode':mode,'cases':profile_cases,
          'scenario_logs':{name+'.log':sha(data/(name+'.log'))for name in profile_cases},
          'binaries':{name:sha(data/name)for name in profile_bins},'compiler_wrapper_sha256':sha(wrapper)}
 for name in profile_cases:
  text=(data/(name+'.log')).read_text()
  assert not any(token in text for token in ['ERROR: AddressSanitizer','runtime error:'])
 receipt=d/'profile-data-verified.json';receipt.write_text(json.dumps(details,indent=2)+'\n')
 contracts.append({'kind':'profile-data','mode':mode,'cases':9,'receipt':str(receipt.relative_to(o)),'sha256':sha(receipt)})
 cf=flags+(['-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie']if san else[])
 ld=['-Wl,--gc-sections']+(['-no-pie']if san else[])
 for name,units in [
  ('syncprog',['src/syncprog.c','src/progresso.c','src/js.c','src/jsw.c','tests/syncprog.c']),
  ('homejanelas',['src/cwordem.c',str(HERE/'home-window/adapter.c'),str(HERE/'homejanelas-dependencies.c'),'src/cotacat.c','src/js.c','src/metaprov.c','src/colecoes.c','src/redeurl.c','src/catordem.c'])]:
  binary=d/name;extra=[]
  if name=='homejanelas':
   generated=d/'home-window-generated'
   rows.append(capture(d,'home-window-adapter',[sys.executable,str(HERE/'home-window/prepare.py'),'--source',str(s),'--out',str(generated)],timeout=60))
   extra=['-DHOME_WINDOW_BODY=\"'+str(generated/'home-window-body.inc')+'\"','-DDISCOVERY_SOURCE=\"'+str(s/'src/descoberta.c')+'\"']
  rows.append(capture(d,name+'-compile',[cc,*cf,*extra,*units,*ld,'-o',str(binary)],cwd=s,env=env,timeout=240))
  rows.append(capture(d,name+'-run',[str(binary)],cwd=s,env=env,timeout=90))
  contracts.append({'kind':name,'mode':mode,'binary':str(binary.relative_to(o)),'sha256':sha(binary),'test_sha256':sha(s/'tests'/(name+'.c'))})
 source_binding(s,a.proof)
result={'outcome':'passed','source_binding':v,'plan_sha256':sha(HERE/'plan.json'),'contracts':contracts,'gates':rows,'scope':'Fresh actual discovery4/sync3, nine original profile ownership scenarios, actual syncprog and Home-window assertions in normal and ASan/UBSan modes. No unrelated feature or JVM suites.'}
(o/'affected-verified.json').write_text(json.dumps(result,indent=2)+'\n')
print('PASS focused startup/profile/progress/Home normal+ASan',flush=True)
