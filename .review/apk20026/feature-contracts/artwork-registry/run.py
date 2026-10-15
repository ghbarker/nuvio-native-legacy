from pathlib import Path
import argparse,hashlib,json,subprocess,sys
r=Path(__file__).resolve().parent;p=argparse.ArgumentParser()
for n in['source','output']:p.add_argument('--'+n,type=Path,required=True)
p.add_argument('--cc',required=True);p.add_argument('--sanitize',action='store_true');p.add_argument('--private',action='store_true');a=p.parse_args();s=a.source.resolve();o=a.output.resolve();assert not o.exists();o.mkdir(parents=True);sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest();git=lambda *v:subprocess.check_output(['git','-C',str(s),*v],text=True).strip();head,tree=git('rev-parse','HEAD'),git('rev-parse','HEAD^{tree}');assert not git('status','--porcelain')
cmd=[sys.executable,str(r/'fixture/run.py'),'--source',str(s),'--output',str(o/'raw'),'--cc',a.cc]+(['--sanitize']if a.sanitize else[])+(['--private']if a.private else[]);z=subprocess.run(cmd,text=True,capture_output=True,timeout=600);log=o/'producer.log';log.write_text(z.stdout+z.stderr);assert not z.returncode,z.stdout+z.stderr
rawp=o/'raw/receipt.json';v=json.loads(rawp.read_text());assert v['passed'] and v['source_commit']==head and v['tree']==tree and v['source_status']=='' and v['private']is a.private;assert v['sanitizers']==(['address','undefined']if a.sanitize else[]);assert [x['mode']for x in v['cases']]==['original','long','arena']and all(x['returncode']==0 and not x['sanitizer_error']for x in v['cases']);assert v['symbol_bytes']['regTexto']==2*1024*1024
sources={}
for f,h in v['dependency_sha256'].items():
 assert sha(f)==h
 if Path(f).is_relative_to(s):sources[str(Path(f).relative_to(s))]=h
commands=[c for c in v['commands']if '-c'in c];assert commands and all(('-DNV_TEST_LOCAL_LOGS=1'in c)is a.private and ('-fsanitize=address,undefined'in c)is a.sanitize for c in commands)
for f,h in v['logs_sha256'].items():assert sha(o/'raw'/f)==h
assert not git('status','--porcelain')and git('rev-parse','HEAD')==head
receipt={'passed':True,'source_commit':head,'source_tree':tree,'private':a.private,'sanitizers':v['sanitizers'],'cases':['original','long','arena'],'raw_receipt_relative':'raw/receipt.json','raw_receipt_sha256':sha(rawp),'source_inputs_sha256':sources,'logs_sha256':{str(q.relative_to(o)):sha(q)for q in o.rglob('*.log')},'symbol_bytes':v['symbol_bytes'],'scope':'Actual registry plus unchanged original assertions, maximum-length identities and bounded arena cases; typed offline service responses, no network/GPU/device claim.'};(o/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print('PASS artwork original+long+arena')
