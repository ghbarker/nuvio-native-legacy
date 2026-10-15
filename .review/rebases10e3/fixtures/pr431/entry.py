"""Run the accepted Linux TV/Mobile UI contracts once, with fresh source binding.
The producer itself runs normal and ASan/UBSan; this wrapper records the raw
receipt and compiler without changing any existing runtime assertion.
"""
from pathlib import Path
import argparse, hashlib, json, os, shlex, subprocess, sys
HERE=Path(__file__).resolve().parent
SHARED=HERE.parents[1]
sys.path.insert(0,str(SHARED))
from common import source_binding, sha

def main():
 p=argparse.ArgumentParser()
 for key in ['source','proof','output','cc']: p.add_argument('--'+key,type=Path,required=True)
 a=p.parse_args(); source=a.source.resolve(); proof_path=a.proof.resolve(); out=a.output.resolve(); cc=a.cc.resolve()
 assert cc.is_file() and not out.exists()
 plan,proof,manifest=source_binding(source,proof_path)
 commit=subprocess.check_output(['git','-C',str(source),'rev-parse','HEAD'],text=True).strip()
 tree=subprocess.check_output(['git','-C',str(source),'rev-parse','HEAD^{tree}'],text=True).strip()
 out.mkdir(parents=True)
 bindir=out/'compiler-bin'; bindir.mkdir(); wrapper=bindir/'cc'
 wrapper.write_text('#!/bin/sh\nexec '+shlex.quote(str(cc))+' "$@"\n'); wrapper.chmod(0o755)
 env=dict(os.environ,PATH=str(bindir)+os.pathsep+os.environ['PATH'])
 env['PYTHONPATH']=str(SHARED)+(os.pathsep+env['PYTHONPATH'] if env.get('PYTHONPATH') else '')
 command=[sys.executable,str(HERE/'runtime.py'),'--source',str(source),'--proof',str(proof_path),'--output',str(out/'runtime')]
 result=subprocess.run(command,env=env,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=900)
 log=out/'runtime-producer.log'; log.write_text(result.stdout)
 assert result.returncode==0,(str(log),result.stdout[-5000:])
 path=out/'runtime/runtime-verified.json'; raw=json.loads(path.read_text())
 assert raw['outcome']=='passed' and raw['source_binding']==proof
 assert raw['normal_and_sanitizer_modes']==['normal','address+undefined']
 assert raw['settings_apply_cancel_persistence_cases']==48
 assert raw['storage_error_geometry_cases']==12 and raw['tv_notice_observations']==12
 assert raw['mode_leaf_executions']==18 and raw['reused_test_passes'] is False
 assert len(raw['compiled'])==22 and len(raw['runs'])==70
 assert all(row['returncode']==0 for row in raw['runs'])
 source_binding(source,proof_path)
 assert subprocess.check_output(['git','-C',str(source),'rev-parse','HEAD'],text=True).strip()==commit
 assert subprocess.check_output(['git','-C',str(source),'rev-parse','HEAD^{tree}'],text=True).strip()==tree
 receipt={'outcome':'passed','source_binding':proof,'source_commit':commit,'source_tree':tree,
          'runtime_receipt':'runtime/runtime-verified.json','runtime_receipt_sha256':sha(path),
          'runtime_log':'runtime-producer.log','runtime_log_sha256':sha(log),
          'compiler':str(cc),'compiler_sha256':sha(cc),'compiler_wrapper_sha256':sha(wrapper),
          'producer_sha256':sha(HERE/'runtime.py'),'entry_sha256':sha(Path(__file__)),
          'fresh_normal_and_sanitizers':True,'compiled_fixtures':len(raw['compiled']),
          'executed_cases':len(raw['runs']),'scope':raw['scope']}
 (out/'pr431-targeted-verified.json').write_text(json.dumps(receipt,indent=2)+'\n')
 print('PASS PR431: fresh accepted TV/Mobile runtime normal+ASan/UBSan, source-bound raw receipts',flush=True)

if __name__=='__main__': main()
