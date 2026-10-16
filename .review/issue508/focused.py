"""Bind one accepted Linux fixture family to its exact matrix candidate."""
from pathlib import Path
import argparse,json,platform,sys
from common import HERE,source_binding,record,sha
p=argparse.ArgumentParser()
for name in ['source','proof','output']:p.add_argument('--'+name,type=Path,required=True)
a=p.parse_args();s=a.source.resolve();out=a.output.resolve();assert not out.exists();out.mkdir(parents=True)
plan,v,m=source_binding(s,a.proof);assert platform.system()=='Linux'and m['pr']==508
entry=HERE/'fixtures'/('pr'+str(m['pr']))/'entry.py';raw=out/'raw'
gate=record(out,'accepted-fixtures',[sys.executable,str(entry),'--source',str(s),'--proof',str(a.proof.resolve()),'--output',str(raw),'--cc','/usr/bin/cc'],timeout=1500)
receipt=raw/'receipt.json';assert receipt.is_file()
q=json.loads(receipt.read_text());assert q.get('passed',q.get('outcome')=='passed')
assert q['source_commit']==m['head']and q.get('source_tree',q.get('tree'))==m['tree']
source_binding(s,a.proof)
r={'outcome':'passed','source_binding':v,'plan_sha256':sha(HERE/'plan.json'),'candidate_pr':m['pr'],'gate':gate,'entry_sha256':sha(entry),'receipt_relative':str(receipt.relative_to(out)),'receipt_sha256':sha(receipt),'raw_json_sha256':{str(path.relative_to(out)):sha(path)for path in raw.rglob('*.json')},'raw_logs_sha256':{str(path.relative_to(out)):sha(path)for path in raw.rglob('*.log')},'scope':'Fresh relevant normal and ASan/UBSan actual-source assertions; typed SDK boundaries only. No production SDK output reuse.'}
(out/'focused-verified.json').write_text(json.dumps(r,indent=2)+'\n');print('PASS focused exact candidate',m['pr'],flush=True)
