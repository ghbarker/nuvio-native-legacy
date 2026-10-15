#!/usr/bin/env python3
"""Apply only explicit CI signing/compatible-app transforms, recording exact hashes."""
from pathlib import Path
import argparse,json,os,subprocess
from common import binding,file_sha,sha,transformed,DEBUG_SIGNER
p=argparse.ArgumentParser()
for key in ('source','proof','output'):p.add_argument('--'+key,type=Path,required=True)
p.add_argument('--layer',choices=['product','test'],required=True);p.add_argument('--restore',action='store_true');a=p.parse_args();s=a.source.resolve()
if a.restore:
 _,_,_,r=transformed(s,a.proof,a.layer,a.output)
 for e in r['changes']:(s/e['path']).write_bytes(subprocess.check_output(['git','-C',str(s),'show','HEAD:'+e['path']]))
 assert not subprocess.check_output(['git','-C',str(s),'diff','--name-only','HEAD'],text=True).strip()
 print('Restored exact tracked source after completed build verification');raise SystemExit(0)
plan,v,m,git=binding(s,a.proof,a.layer);assert not git('status','--porcelain')
key=os.environ.get('NUVIO_TEST_DEBUG_KEYSTORE','');assert key and Path(key).is_file()
changes={};gradle='android/app/build.gradle.kts';manifest='android/app/src/main/AndroidManifest.xml'
body=(s/gradle).read_text();before=body
signer=DEBUG_SIGNER
marker='    signingConfigs {\n';assert body.count(marker)==1
if a.layer=='product':
 assert 'getByName("debug")' not in body and 'NUVIO_TEST_DEBUG_KEYSTORE' not in body
 assert body.count('applicationId = "space.nuvio.nativelegacy"')==1
 assert body.count('versionCode = vx * 10000 + vy * 100 + vz')==1 and body.count('versionName = versao')==1
 body=body.replace(marker,marker+signer,1);changes[gradle]=(before,body)
else:
 # The private overlay already pins its compatible identity and signing hook.
 # Verify it byte-for-byte and retain a clean no-op transform receipt.
 ident=plan['identity'];assert body.count(signer)==1 and body.count('getByName("debug")')==1
 for token in ['applicationId = '+json.dumps(ident['package']),'versionCode = '+str(ident['version_code']),'versionName = '+json.dumps(ident['version_name']),'namespace = "space.nuvio.nativelegacy"']:
  assert body.count(token)==1,token
 text=(s/manifest).read_text()
 for token in ['android:label='+json.dumps(ident['label']),'android:authorities="'+ident['package']+'.atualizacao"']:
  assert text.count(token)==1,token
r={'source_commit':v['source_commit'],'tree':v['tree'],'layer':a.layer,'source_proof_sha256':file_sha(a.proof),'changes':[],'scope':'CI-only persistent debug signer insertion on product; test overlay identity/signing verified without edits. Product C/Kotlin code unchanged.'}
for rel,(before,after) in changes.items():
 assert before!=after;(s/rel).write_text(after);r['changes'].append({'path':rel,'before_sha256':sha(before.encode()),'after_sha256':sha(after.encode())})
a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(r,indent=2)+'\n')
transformed(s,a.proof,a.layer,a.output)
print('Verified narrow CI-only transforms:',a.layer,len(changes),'files')
