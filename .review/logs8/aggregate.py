#!/usr/bin/env python3
"""Bind all independent build receipts to one frozen product/test plan; never publish."""
from pathlib import Path
import argparse,hashlib,json,os
p=argparse.ArgumentParser();p.add_argument('--results',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();here=Path(__file__).resolve().parent;plan=json.loads((here/'plan.json').read_text());assert plan['frozen'] and plan['format']==1;sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
def one(name):
 files=list(a.results.rglob(name));assert len(files)==1,(name,files);return files[0],json.loads(files[0].read_text())
def binding(record,layer):
 source=plan['sources'][layer];assert record['source_commit']==source['commit'] and record['tree']==source['tree'] and record['review_commit']==os.environ['GITHUB_SHA']
np,native=one('native-verified.json');assert native['outcome']=='passed' and native['sdk_sanity_passed'];assert {(x['layer'],x['ui']) for x in native['full_apps']}=={(l,u) for l in ['product','test'] for u in ['tv','touch']};assert all(x['bytes']>0 for x in native['full_apps'])
for layer,v in native['source_bindings'].items():binding(v,layer)
assert set(native['source_bindings'])=={'product','test'}
labels={r['label'] for r in native['records']};assert all('focused-'+g+'-'+m+'-run' in labels for g in ['original-parser','cache-metadata','exclusion-parser','preparation','profile-bank','profile-scope','worker','cache-indicator'] for m in ['normal','asan']);assert {'translations','actual513-dispatch','actual513-owner'}<=labels
for name,count in [('dispatch',50),('owner',18)]:
 files=list(a.results.rglob('report.json'));matched=[]
 for f in files:
  if ('actual513-'+name) in str(f):matched.append(json.loads(f.read_text()))
 assert len(matched)==1;report=matched[0];assert report['outcome']=='passed' and report['source_unchanged'];assert {(r['ui'],r['mode']) for r in report['records']}=={(u,m) for u in ['tv','mobile'] for m in ['normal','asan']};assert all(r['assertions']==count and r['returncode']==0 and not r['traps_fired'] for r in report['records'])
kp,kotlin=one('kotlin-verified.json');assert kotlin['outcome']=='passed' and kotlin['passed_contracts']==32;binding(kotlin['source_binding'],'test');assert [r['passed_contracts'] for r in kotlin['records']]==[0,26,6]
rp,reuse=one('player15-reuse-verified.json');assert reuse['outcome']=='passed' and reuse['online_original_job_verified'] and reuse['passed_selected_tests']==15 and reuse['excluded_old_private_log_tests']==5;assert reuse['source_commit']==plan['sources']['test']['commit'] and reuse['review_commit']==os.environ['GITHUB_SHA']
android=list(a.results.rglob('android-verified.json'));assert len(android)==2;layers={};apk=[]
for f in android:
 d=json.loads(f.read_text());assert d['outcome']=='passed';binding(d,d['layer']);layers[d['layer']]=d;assert d['native_16kb_pages']=='passed' and {r['abi'] for r in d['actual_arm_compiles']}=={'arm64-v8a','armeabi-v7a'};assert d['legacy_jni_overloads_preserved'] and d['file_provider']['nonexported'] and d['file_provider']['updater_path_only'];assert d['certificate_sha256']==plan['identity']['certificate_sha256'];files=list(f.parent.glob(d['apk_name']));assert len(files)==1 and sha(files[0])==d['apk_sha256'];apk.append({'layer':d['layer'],'name':d['apk_name'],'sha256':d['apk_sha256'],'bytes':d['apk_bytes']})
assert set(layers)=={'product','test'};assert layers['product']['package']=='space.nuvio.nativelegacy' and not layers['product']['private_log_opt_in'];test=layers['test'];assert test['package']==plan['identity']['package'] and test['version_code']==20018 and test['version_name']==plan['identity']['version_name'] and test['private_log_opt_in']
originals=list(a.results.rglob('original-asan-verified.json'));assert len(originals)==3
assert {json.loads(f.read_text())['suite'] for f in originals}=={'fontepref','fontepref_file','player_retido'}
for f in originals:
 d=json.loads(f.read_text());assert d['outcome']=='passed' and d['script_unchanged'] and d['original_flags_unchanged'];binding(d['source_binding'],'product');assert set(d['sanitizers'])=={'address','undefined'}
receipts=list(a.results.rglob('build-receipt.json'));assert len(receipts)==2
for f in receipts:
 d=json.loads(f.read_text());assert d['verified'] and d['source_restored_clean'];assert d['source_commit']==plan['sources'][d['layer']]['commit'] and d['review_commit']==os.environ['GITHUB_SHA']
a.output.parent.mkdir(parents=True,exist_ok=True);summary={'outcome':'passed','review_commit':os.environ['GITHUB_SHA'],'plan_sha256':sha(here/'plan.json'),'sources':plan['sources'],'apks':apk,'proof_hashes':{'native':sha(np),'kotlin':sha(kp),'historical_player15':sha(rp),'android':{json.loads(f.read_text())['layer']:sha(f) for f in android},'original3':{json.loads(f.read_text())['suite']:sha(f) for f in originals}},'scope':'New complete macOS TV/touch product+private links, affected actual C normal/sanitized, complete standard/test ARM32/64 APK/16KB/signature/DEX/JNI, new32Kotlin download contracts. Historical reuse applies only to15unchanged isolated player tests.','no_pr_release_tag_or_backend_action':True};a.output.write_text(json.dumps(summary,indent=2)+'\n');print('All frozen Logs8 source/build gates passed; artifacts are ready for root review')
