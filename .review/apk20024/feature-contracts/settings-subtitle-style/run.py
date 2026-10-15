#!/usr/bin/env python3
"""Normalize the complete frozen owner producer without changing its assertions."""
from pathlib import Path
import argparse,hashlib,json,subprocess,sys
HERE=Path(__file__).resolve().parent
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
p=argparse.ArgumentParser(description=__doc__)
for name in ['source','output']:p.add_argument('--'+name,type=Path,required=True)
p.add_argument('--cc',required=True);p.add_argument('--private',action='store_true');p.add_argument('--sanitize',action='store_true')
a=p.parse_args();source=a.source.resolve();out=a.output.resolve();assert not out.exists(),'Use fresh outputs'
git=lambda *args:subprocess.check_output(['git','-C',str(source),*args],text=True).strip()
commit,tree=git('rev-parse','HEAD'),git('rev-parse','HEAD^{tree}');assert not git('status','--porcelain')
freeze_path=HERE/'fixture-freeze.json';freeze=json.loads(freeze_path.read_text())
for name,digest in freeze['files_sha256'].items():assert sha(HERE/'fixture'/name)==digest,name
out.mkdir(parents=True)
command=[sys.executable,str(HERE/'fixture/run.py'),'--source',str(source),'--output',str(out/'raw'),'--cc',a.cc]+(['--sanitize']if a.sanitize else[])+(['--private']if a.private else[])
z=subprocess.run(command,text=True,capture_output=True,timeout=840);log=out/'producer.log';log.write_text(z.stdout+z.stderr);assert z.returncode==0,z.stdout+z.stderr
raw_path=out/'raw/receipt.json';raw=json.loads(raw_path.read_text());assert raw['passed']is True and len(raw['records'])==2
expected={('1080','2340','1','1'),('1080','2340','1.5','1'),('2340','1080','1','1'),('2340','1080','1.5','1'),('1920','1080','1','0'),('1920','1080','1.5','0')}
records=[]
for record,kind in zip(raw['records'],['menu','style']):
 path=Path(record['path']).resolve();assert path.is_relative_to((out/'raw').resolve()) and path==out/'raw'/kind/'receipt.json'
 assert sha(path)==record['sha256'];value=json.loads(path.read_text())
 assert value['kind']==kind and value['passed']is True and value['outcome']=='passed'
 assert value['source_commit']==commit and value['source_tree']==tree and value['private']is a.private
 assert value['sanitizers']==(['address','undefined']if a.sanitize else[]) and value['abort_traps_fired']is False
 assert len(value['cases'])==6 and {tuple(x['argv'])for x in value['cases']}==expected
 assert all(x['returncode']==0 and x['actual_returncode']==0 and x['expected_feature_absent']is False for x in value['cases'])
 for name,digest in value['dependency_sha256'].items():assert sha(name)==digest,name
 commands=[cmd for cmd in value['commands']if '-c'in cmd];assert commands
 assert all('-DNV_ANDROID=1'in cmd and '-DNV_TOUCH_UI=1'in cmd and ('-DNV_TEST_LOCAL_LOGS=1'in cmd)is a.private and ('-fsanitize=address,undefined'in cmd)is a.sanitize for cmd in commands)
 records.append({'kind':kind,'cases':6,'raw_receipt_relative':str(path.relative_to(out)),'raw_receipt_sha256':sha(path),'source_inputs_sha256':{str(Path(f).relative_to(source)):h for f,h in value['dependency_sha256'].items()if Path(f).is_relative_to(source)}})
for name,digest in raw['root_logs_sha256'].items():assert sha(out/'raw'/name)==digest,name
for name,digest in freeze['files_sha256'].items():assert sha(HERE/'fixture'/name)==digest,name
assert git('rev-parse','HEAD')==commit and git('rev-parse','HEAD^{tree}')==tree and not git('status','--porcelain')
result={'passed':True,'source_commit':commit,'source_tree':tree,'source_clean':True,'private':a.private,'sanitizers':['address','undefined']if a.sanitize else[],'fixture_freeze_sha256':sha(freeze_path),'producer_command':command,'producer_log_sha256':sha(log),'raw_receipt_relative':'raw/receipt.json','raw_receipt_sha256':sha(raw_path),'groups':records,'logs_sha256':{str(p.relative_to(out)):sha(p)for p in out.rglob('*.log')},'scope':raw['limits']+' Root normalizer adds only immutable source/fixture/flag/child receipt binding; owner assertions and seeded presentation boundary remain unchanged.'}
(out/'receipt.json').write_text(json.dumps(result,indent=2)+'\n');print('PASS complete Settings Style menu6 + editor6')
