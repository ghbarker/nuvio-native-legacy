#!/usr/bin/env python3
"""External legacy vectors updated to upstream #205; every assert stays intact."""
import argparse,hashlib,json,re,os,tempfile
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('source',type=Path);p.add_argument('output',type=Path);a=p.parse_args()
raw=a.source.read_bytes();old=raw.decode();text=old
changes=[
 ('const int mistaQ[] = {2,3,1,0}, mistaA[] = {1,2,3,0};','const int mistaQ[] = {0,2,3,1}, mistaA[] = {1,2,3,0};'),
 ('const int hintQ[] = {2,1,0}, hintA[] = {1,2,0};','const int hintQ[] = {0,2,1}, hintA[] = {1,2,0};'),
 ('const int consultada[] = {1,0,2};\n  modo = 0; conferir(consultada, 3); modo = 1; conferir(consultada, 3);','const int consultadaQ[] = {0,2,1}, consultadaA[] = {1,0,2};\n  modo = 0; conferir(consultadaQ, 3); modo = 1; conferir(consultadaA, 3);'),
]
for before,after in changes:
 assert text.count(before)==1,before
 text=text.replace(before,after)
assert re.findall(r'\bassert\s*\((.*?)\)\s*;',old,re.S)==re.findall(r'\bassert\s*\((.*?)\)\s*;',text,re.S)
assert a.source.absolute()!=a.output.absolute()
a.output.parent.mkdir(parents=True,exist_ok=True)
fd,name=tempfile.mkstemp(dir=a.output.parent,prefix='.order-fixture-')
with os.fdopen(fd,'w') as f:f.write(text)
os.replace(name,a.output)
assert a.source.read_bytes()==raw
a.output.with_suffix('.manifest.json').write_text(json.dumps({'label':'Legacy addon-order assertions with expected quality vectors updated to upstream #205',
 'original_sha256':hashlib.sha256(raw).hexdigest(),'companion_sha256':hashlib.sha256(text.encode()).hexdigest(),
 'all_assertions_unchanged':True,'product_source_or_original_tests_modified':False,'replacements':[{'before':b,'after':c} for b,c in changes],
 'baseline_evidence':'Exact upstream3c28 original fixture plus real linguas.c fails at mistaQ: index0 versus stale expected2; source already puts uncached items last within each quality group'},indent=2)+'\n')
