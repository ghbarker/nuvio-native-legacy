"""Bind historical helper contracts without relabeling them as fresh execution."""
from pathlib import Path
import argparse,json
from common import HERE,sha,source_binding
p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);p.add_argument('--proof',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
plan,proof,manifest=source_binding(a.source,a.proof)
r=HERE/'retained-jvm';k=json.loads((r/'kotlin-result.json').read_text())
assert k['status']=='passed' and k['passed_contracts']==26 and k['actual_product_unchanged_during_run']
assert all(x['exit_code']==0 for x in k['results'])
helper=a.source/'android/app/src/main/java/space/nuvio/nativelegacy/RegistroDownload.kt'
assert sha(helper)==k['actual_product_sha256']
for name,digest in k['fixture_sha256'].items():assert sha(r/name)==digest,name
assert len(k['tool_sha256'])==7
assert 'REGISTRO_DOWNLOAD_PREFLIGHT_OK 26' in (r/'kotlin-execute.log').read_text()
assert sha(r/'kotlin-execute.log')==next(x['log_sha256']for x in k['results']if x['step']=='execute')
v={'verified':True,'helper_sha256':sha(helper),'contracts':26,'historical_source_commit':'1cd707586a5bc94579904e5601de996e62326067','current_source_binding':proof,'historical_result_sha256':sha(r/'kotlin-result.json'),'fixture_sha256':k['fixture_sha256'],'historical_tool_sha256':k['tool_sha256'],'reexecuted':False,'scope':'Exact unchanged helper/fixture/tool graph; historical local JVM proof only. Fresh Activity/helper compilation and actual DEX/JNI are required separately in Android.'}
a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(v,indent=2)+'\n');print('Bound retained26 helper contracts; not a fresh runtime pass')
