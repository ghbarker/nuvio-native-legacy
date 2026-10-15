"""Portable replay of reviewed episode transport/backend fixtures; no media I/O."""
from pathlib import Path
import argparse, hashlib, json, os, subprocess, sys

p=argparse.ArgumentParser()
p.add_argument('--source',type=Path,required=True)
p.add_argument('--output',type=Path,required=True)
p.add_argument('--cc',default='cc')
p.add_argument('--sanitize',action='store_true')
a=p.parse_args();s=a.source.resolve();o=a.output.resolve();o.mkdir(parents=True,exist_ok=True);r=Path(__file__).resolve().parent
sha=lambda f:hashlib.sha256(Path(f).read_bytes()).hexdigest()
git=lambda *args:subprocess.check_output(['git','-C',str(s),*args],text=True).strip()
head=git('rev-parse','HEAD');tree=git('rev-parse','HEAD^{tree}');assert not git('status','--porcelain')
source_inputs={str(f.relative_to(s)):sha(f)for f in(s/'src').rglob('*')if f.is_file()and f.suffix in ['.c','.h','.inc']}
fixture_inputs={str(f.relative_to(r)):sha(f)for f in r.rglob('*')if f.is_file()and f.suffix in ['.py','.c','.h','.inc']}
groups=[]
def execute(group,driver,extra):
 d=o/group;command=[sys.executable,str(r/driver),'--source',str(s),'--output',str(d),'--cc',a.cc,*extra]
 log=o/(group+'.log')
 with log.open('w')as stream:z=subprocess.run(command,stdout=stream,stderr=subprocess.STDOUT,timeout=360,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1:print_stacktrace=1'})
 assert z.returncode==0,(group,log.read_text()[-5000:])
 receipt=d/('contracts-receipt.json'if group=='reload-backend'else'syntax-receipt.json'if group=='backend-syntax'else'result.json')
 q=json.loads(receipt.read_text())
 if 'outcome'in q:assert q['outcome']=='passed'
 if 'records'in q:
  for entry in q['records']:
   if entry.get('label')=='link-probe' and entry.get('returncode'):
    assert q.get('unreached_trap_symbols') and q.get('traps_fired') is False
    continue  # reviewed discovery link; final link and executed fixture still must succeed
   assert all(entry.get(key,0)==0 for key in ['returncode','exit_code','compile_returncode','run_returncode'])
   assert entry.get('passed',True)
 if group.startswith('player-'):
  layouts=66 if group=='player-tv' else 264
  text=(d/'run.log').read_text();assert 'PASS 97 actual transport/episode activation checks'in text and ('PASS '+str(layouts)+' actual rendered transport layouts')in text
  assert not q['traps_fired']
 if group=='reload-backend':assert len(q['records'])==3 and all(x['sanitizers']==a.sanitize for x in q['records'])
 if group=='backend-syntax':assert len(q['records'])==5
 logs={str(f.relative_to(o)):sha(f)for f in d.rglob('*.log')};logs[str(log.relative_to(o))]=sha(log)
 groups.append({'group':group,'passed':True,'commands':[command],'receipt':str(receipt.relative_to(o)),'receipt_sha256':sha(receipt),'logs_sha256':logs})
san=['--san']if a.sanitize else[]
for name,flags in [('tv',[]),('mobile',['--touch']),('android',['--touch','--android'])]:
 execute('player-'+name,'build-player.py',['--real-pointer','--render','--episode-buttons',*flags,*san])
for name,flags in [('tv',[]),('android',['--android'])]:execute('app-'+name,'run-app-route.py',[*flags,*san])
execute('selectors','run-selectors.py',['--previous',*san])
execute('posplay','run-posplay.py',san)
execute('reload-backend','reload-backend/run.py',['--sanitize']if a.sanitize else[])
if not a.sanitize:execute('backend-syntax','reload-backend/syntax.py',[])
assert source_inputs=={f:sha(s/f)for f in source_inputs}
assert not git('status','--porcelain')and git('rev-parse','HEAD')==head and git('rev-parse','HEAD^{tree}')==tree
record={'outcome':'passed','source_commit':head,'source_tree':tree,'source_clean':True,'sanitizers':['address','undefined']if a.sanitize else[],'groups':groups,'source_inputs_sha256':source_inputs,'fixture_files_sha256':fixture_inputs,'compiler':subprocess.check_output([a.cc,'--version'],text=True).splitlines()[0],'limits':'Actual full player events/drawing logic, actual pointer and episode/profile modules, verbatim app routing/backend helper contracts with typed idle media/font/GPU/JNI boundaries. No media HTTP, provider account, physical decoder or GPU-pixel claim. Five backend syntax branches use host SDK type declarations; standard Android/full macOS builds are separate producer gates.'}
(o/'receipt.json').write_text(json.dumps(record,indent=2)+'\n')
print('PASS episode transport actual-source groups='+str(len(groups)),flush=True)
