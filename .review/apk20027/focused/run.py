#!/usr/bin/env python3
"""Actual saved-row store, exact Settings projection and discovery guards."""
import argparse,hashlib,json,os,pathlib,platform,subprocess,time,tempfile,re
p=argparse.ArgumentParser();p.add_argument('--source',required=True);p.add_argument('--out',required=True);p.add_argument('--sanitize',action='store_true');p.add_argument('--baseline',action='store_true');p.add_argument('--reproduce-unknown-policy',action='store_true');a=p.parse_args()
src=pathlib.Path(a.source).resolve();out=pathlib.Path(a.out).resolve();out.mkdir(parents=True,exist_ok=True);here=pathlib.Path(__file__).resolve().parent
settings=(src/'src/ajustes.c').read_text();discovery=(src/'src/descoberta.c').read_text()
def function(text,needle):
 start=text.index(needle);end=text.index('\n}',start)+2;return text[start:end]+'\n'
def block(text,needle):
 start=text.index(needle);start=text.index('{',start);level=0;state='code';i=start
 while i<len(text):
  c=text[i];n=text[i+1]if i+1<len(text)else''
  if state=='line':
   if c=='\n':state='code'
  elif state=='comment':
   if c=='*'and n=='/':state='code';i+=1
  elif state in ['"',"'"]:
   if c=='\\':i+=1
   elif c==state:state='code'
  elif c=='/'and n=='/':state='line';i+=1
  elif c=='/'and n=='*':state='comment';i+=1
  elif c in ['"',"'"]:state=c
  elif c=='{':level+=1
  elif c=='}':
   level-=1
   if not level:return text[start:i+1]+'\n'
  i+=1
 raise ValueError('unclosed production block')
parts={'settings-projection.inc':function(settings,'static int filCmpFora(')+function(settings,'static void filMontarLista('),'discovery-guards.inc':block(discovery,'{ const char *ids[16], *bases[16];')}
for name,text in parts.items():(out/name).write_text(text)
flags=['-O1','-g','-Isrc','-I'+str(out),'-pthread','-ffunction-sections','-fdata-sections','-DFIL_TESTE']
flags+=['-Wl,-dead_strip']if platform.system()=='Darwin'else['-Wl,--gc-sections']
if a.sanitize:
 flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
 if platform.system()=='Linux':flags+=['-no-pie']
cmd=[os.environ.get('CC','cc'),*flags,str(here/'rows.c'),'src/fileiras.c','-o',str(out/'removed-addon-rows')]
env=os.environ.copy()
if a.sanitize:env.update(ASAN_OPTIONS='detect_leaks='+('0'if platform.system()=='Darwin'else'1')+':halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
records=[];baseline=False
def run(c,n,expect_failure=False):
 global baseline
 t=time.monotonic();r=subprocess.run(c,cwd=src,env=env,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=60);f=out/(n+'.log');f.write_text(r.stdout)
 good=(r.returncode!=0 and ('EXPECTED_AVAILABLE_ROW_MISSING'if a.reproduce_unknown_policy else'REMOVED_ROW_STILL_IN_SETTINGS')in r.stdout)if expect_failure else r.returncode==0
 records.append(dict(name=n,command=c,exit_code=r.returncode,expected_failure=expect_failure,expectation_met=good,seconds=round(time.monotonic()-t,3),log=f.name,log_sha256=hashlib.sha256(f.read_bytes()).hexdigest()))
 if expect_failure:baseline=good
 if not good:raise RuntimeError(n+' failed:\n'+r.stdout[-6000:])
try:
 run(cmd,'compile')
 cases=['removed']if a.baseline else ['unknown-disabled']if a.reproduce_unknown_policy else ['removed','reinstall','unknown-disabled','unknown-reinstalled','identity-complete-after-unknown','cached-home','failed-after-removal','pending','failed-manifest','changed-list','other-profile','package-list','disabled','empty-list','raw-base','prefix-boundary','same-profile-reopen','profile-switch','full-home','full-outside']
 for case in cases:
  with tempfile.TemporaryDirectory(prefix='rows-data-',dir=out)as d:run([str(out/'removed-addon-rows'),case,d],case,a.baseline or a.reproduce_unknown_policy)
finally:
 receipt={'all_passed':not(a.baseline or a.reproduce_unknown_policy) and len(records)>1 and all(r['expectation_met']for r in records),'baseline_reproduced':baseline if a.baseline else False,'unknown_policy_regression_reproduced':baseline if a.reproduce_unknown_policy else False,'sanitized':a.sanitize,'source':str(src),'source_sha256':{f:hashlib.sha256((src/f).read_bytes()).hexdigest()for f in ['src/fileiras.c','src/fileiras.h','src/ajustes.c','src/descoberta.c']},'fixture_sha256':{p.name:hashlib.sha256(p.read_bytes()).hexdigest()for p in here.glob('*')if p.is_file()},'extracted_sha256':{name:hashlib.sha256((out/name).read_bytes()).hexdigest()for name in parts},'boundaries':'Actual fileiras.c unchanged; exact filCmpFora/filMontarLista and existing discovery reconciliation block extracted byte-for-byte from supplied source. External boundaries only isolate storage paths/writes, addon snapshot getters and unrelated discovery catalogue flush. No production predicate replacement.','records':records}
 if (out/'removed-addon-rows').exists():receipt['binary_sha256']=hashlib.sha256((out/'removed-addon-rows').read_bytes()).hexdigest()
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(json.dumps({'all_passed':not(a.baseline or a.reproduce_unknown_policy),'baseline_reproduced':baseline if a.baseline else False,'unknown_policy_regression_reproduced':baseline if a.reproduce_unknown_policy else False,'cases':len(records)-1,'receipt':str(out/'receipt.json')}))
