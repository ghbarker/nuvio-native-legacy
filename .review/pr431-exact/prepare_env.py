#!/usr/bin/env python3
"""Prepare runner-only core configuration and CI debug signing without printing values."""
from pathlib import Path
import argparse, base64, hashlib, os, subprocess
from common import plan
p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);a=p.parse_args()
a.output.mkdir(parents=True,exist_ok=True);a.output.chmod(0o700)
body=os.environ.get('NUVIO_TOUCH_CORE_PROPERTIES','');assert body,'Missing core configuration secret'
first={}
for line in body.splitlines():
    if '=' in line and not line.lstrip().startswith('#'):
        k,v=line.split('=',1);first.setdefault(k,v)
if not first.get('NUVIO_REC_URL','').strip():
    lines=body.splitlines();replaced=False
    for n,line in enumerate(lines):
        if line.startswith('NUVIO_REC_URL=') and not replaced:
            lines[n]='NUVIO_REC_URL=https://nuvio-recomendacoes.henriquef29.workers.dev';replaced=True
    if not replaced:lines.append('NUVIO_REC_URL=https://nuvio-recomendacoes.henriquef29.workers.dev')
    body='\n'.join(lines)+'\n'
props=a.output/'product.properties';props.write_text(body);props.chmod(0o600)
encoded=os.environ.get('NUVIO_TOUCH_DEBUG_KEYSTORE_B64','');assert encoded,'Missing persistent CI signing secret'
key=a.output/'debug.keystore';key.write_bytes(base64.b64decode(''.join(encoded.split()),validate=True));key.chmod(0o600)
cert=subprocess.check_output(['keytool','-exportcert','-alias','androiddebugkey','-keystore',str(key),'-storepass','android'],stderr=subprocess.PIPE)
assert hashlib.sha256(cert).hexdigest()==plan()['ci_signing_certificate_sha256']
with open(os.environ['GITHUB_ENV'],'a') as f:
    f.write('NUVIO_PROPERTIES='+str(props.resolve())+'\nNUVIO_TEST_DEBUG_KEYSTORE='+str(key.resolve())+'\n')
print('Prepared core inputs and verified CI certificate; values withheld')
