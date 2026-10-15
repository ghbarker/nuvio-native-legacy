"""Fresh bounded actual-C subtitle contracts; GNU fixtures execute on Linux."""
from pathlib import Path
import argparse,json,os,subprocess
from common import HERE,source_binding,sha,record
p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);p.add_argument('--proof',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();S=a.source.resolve();O=a.output.resolve();O.mkdir(parents=True,exist_ok=False);plan,binding,manifest=source_binding(S,a.proof);groups=[]
invocations=[]
for mode in ['normal','asan']:
 invocations.append(('settings-'+mode,'settings',['--only','menu']+(['--sanitize']if mode=='asan'else[])))
invocations.append(('secondary','secondary',[]))
for ui in ['tv','mobile']:
 for mode in ['normal','asan']:invocations.append(('outline-'+ui+'-'+mode,'outline',['--platform',ui]+(['--sanitize']if mode=='asan'else[])))
for mode in ['normal','asan']:invocations.append(('style-'+mode,'style',['--platform','all']+(['--sanitize']if mode=='asan'else[])))
for label,kind,extra in invocations:
 dest=O/label;command=['python3','-B',str(HERE/'features'/kind/'run.py'),'--source',str(S),'--output',str(dest),'--cc','/usr/bin/cc',*extra]
 gate=record(O,label,command,timeout=1200)
 receipt=dest/('secondary-final-binding.json'if kind=='secondary'else'receipt.json');r=json.loads(receipt.read_text());assert r['passed'],label
 if kind=='settings':
  assert len(r['records'])==1
  q=json.loads((dest/'menu/receipt.json').read_text());assert q['source_commit']==plan['source_commit']and q['source_tree']==plan['source_tree']and not q['private'];assert len(q['cases'])==6 and all(c['returncode']==0 for c in q['cases'])and not q['abort_traps_fired']
 elif kind=='secondary':
  assert r['source_commit']==plan['source_commit']and r['source_tree']==plan['source_tree']and r['source_clean'];assert r['legacy_trace_cases']==16 and r['legacy_traces_equal']and r['independent_position_cases']==256 and r['bound_delay_normal_and_asan']and r['original_unbound_normal_and_asan'];assert all(c['returncode']==0 for c in r['commands'])
 else:
  assert r['source_commit']==plan['source_commit']and r['source_tree']==plan['source_tree']and not r['private'];assert r['sanitizers']==(['address','undefined']if label.endswith('asan')else[]);assert all(g['passed']for g in r['groups'])
  if kind=='outline':assert {g['group']for g in r['groups']}=={'player','secondary','ass','palette','grid-font'}and r['libass']['sdk_mode']=='Actual installed pkg-config SDK/library pair'
  if kind=='style':assert {g['platform']for g in r['groups']}=={'tv','mobile'}and all(g['returncode']==0 and not g['traps_fired']and g['stdout'].strip()=='PASS actual style controls 274 checks'for g in r['groups'])
 groups.append({'name':label,'kind':kind,'receipt':str(receipt.relative_to(O)),'receipt_sha256':sha(receipt),'execution':gate})
source_binding(S,a.proof)
files={str(f.relative_to(O)):sha(f)for f in sorted(O.rglob('*'))if f.is_file()and(f.suffix in ['.json','.log','.d']or f.name.endswith('.txt'))}
result={'outcome':'passed','source_binding':binding,'plan_sha256':sha(HERE/'plan.json'),'groups':groups,'group_count':9,'all_runtime_checks_fresh':True,'artifact_inputs_sha256':files,'limits':'Actual C Settings/persistence/pointer, secondary resolver/engine/Bidi, original standalone script and libass software rendering. Typed offline text/GPU companions; complete platform apps verified separately.'};(O/'features-verified.json').write_text(json.dumps(result,indent=2)+'\n');print('PASS all9 fresh subtitle contract invocations',flush=True)
