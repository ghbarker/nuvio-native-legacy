#!/usr/bin/env python3
"""Replay the existing focused #475/#362 producers against exact source layers."""
from pathlib import Path
import argparse,concurrent.futures,hashlib,json,os,platform,subprocess,sys
HERE=Path(__file__).resolve().parent
sys.path.insert(0,str(HERE.parent))
from require_plan import load_plan,verify_source
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
def write(path,data):path.write_text(json.dumps(data,indent=2)+'\n')
p=argparse.ArgumentParser(description=__doc__)
for key in ['product','test','product-proof','test-proof','output']:p.add_argument('--'+key,type=Path,required=True)
p.add_argument('--cc',default='/usr/bin/cc');a=p.parse_args();assert platform.system()=='Linux','This bounded feature stage uses the proven system-libass Linux producer'
plan,_=load_plan(HERE.parent/'plan.json');out=a.output.resolve();assert not out.exists();out.mkdir(parents=True)
manifest=HERE/'manifest.json';assert sha(manifest)==plan['recipe_pins']['feature-contracts/manifest.json'];graph=json.loads(manifest.read_text())
for rel,expected in graph['inputs'].items():assert sha(HERE/rel)==expected,rel
cc=str(Path(a.cc).resolve());assert Path(cc).is_file() and os.path.isabs(cc)
bindings={};sources={}
for layer in ['product','test']:
 source=getattr(a,layer).resolve();proofpath=getattr(a,layer+'_proof');proof=json.loads(proofpath.read_text());verify_source(plan,proof,layer,source);sources[layer]=source;bindings[layer]={k:proof[k]for k in ['source_commit','tree','review_commit','manifest_sha256','bundle_sha256']};bindings[layer]['proof_sha256']=sha(proofpath)
recipes=[('collection-state',None),('subtitle-style',None),('collection-settings','all'),('subtitle-outline','tv'),('subtitle-outline','mobile')]
cases=[(layer,san,name,ui)for layer in ['product','test']for san in [False,True]for name,ui in recipes]
def run(case):
 layer,san,name,ui=case;label=name+'-'+layer+('-'+ui if ui else'')+('-asan'if san else'-normal');directory=out/label;command=[sys.executable,str(HERE/name/'run.py'),'--source',str(sources[layer]),'--output',str(directory),'--cc',cc]+(['--platform',ui]if ui else[])+(['--sanitize']if san else[])+(['--private']if layer=='test'else[])
 try:
  result=subprocess.run(command,text=True,capture_output=True,timeout=900);log=result.stdout+result.stderr;rc=result.returncode;timed=False
 except subprocess.TimeoutExpired as error:
  decode=lambda v:v.decode(errors='replace')if isinstance(v,bytes)else(v or'');log=decode(error.stdout)+decode(error.stderr)+'\nTIMEOUT after 900 seconds\n';rc=-1;timed=True
 logfile=out/(label+'.log');logfile.write_text(log);record={'label':label,'layer':layer,'command':command,'returncode':rc,'timed_out':timed,'log_sha256':sha(logfile),'private':layer=='test','sanitizers':'address,undefined'if san else None}
 if rc==0:
  try:
   receipt=directory/'receipt.json';value=json.loads(receipt.read_text());assert value.get('passed') is True or value.get('outcome')=='passed',label
   assert value['source_commit']==bindings[layer]['source_commit']and value.get('source_tree',value.get('tree'))==bindings[layer]['tree'],label
   assert value['private']==(layer=='test'),label
   record['receipt_relative']=str(receipt.relative_to(out));record['receipt_sha256']=sha(receipt)
  except (AssertionError,KeyError,OSError,ValueError)as error:
   record.update(process_returncode=0,returncode=1,receipt_verification_error=str(error));rc=1
 record['passed']=rc==0 and not timed;return record
records=[]
with concurrent.futures.ThreadPoolExecutor(max_workers=2)as pool:
 futures=[pool.submit(run,case)for case in cases]
 for future in concurrent.futures.as_completed(futures):
  records.append(future.result());write(out/'feature-preflight-status.json',{'source_bindings':bindings,'records':records,'complete':len(records)==len(cases),'outcome':'running'});print(records[-1]['label'],records[-1]['passed'],flush=True)
for layer,source in sources.items():verify_source(plan,json.loads(getattr(a,layer+'_proof').read_text()),layer,source)
for rel,expected in graph['inputs'].items():assert sha(HERE/rel)==expected,rel
outputs={str(path.relative_to(out)):sha(path)for path in out.rglob('*')if path.is_file()and path.suffix in ['.json','.log']and path.name!='feature-preflight-status.json'}
receipt={'outcome':'passed'if all(r['passed']for r in records)else'failed','source_bindings':bindings,'groups':len(records),'records':sorted(records,key=lambda r:r['label']),'fixture_manifest_sha256':sha(manifest),'compiler':cc,'compiler_version':subprocess.check_output([cc,'--version'],text=True).splitlines()[0],'libass_version':subprocess.check_output(['pkg-config','--modversion','libass'],text=True).strip(),'raw_receipt_log_sha256':outputs,'scope':'Fresh owner behavioral suites with documented compatibility adapters for caption/style/libass and collection Settings/profile/view/touch on both combined layers; actual source handlers/storage, accepted offline account/catalog/render boundaries. No whole-app or physical-device claim.'};write(out/'feature-contracts-verified.json',receipt);assert receipt['outcome']=='passed'
