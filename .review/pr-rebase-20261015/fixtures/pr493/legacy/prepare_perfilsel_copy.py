#!/usr/bin/env python3
"""Preserve selector assertions and RPC fake; rename only its anonymous stub."""
import argparse,hashlib,json,os,re,tempfile
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('source',type=Path);p.add_argument('output',type=Path);a=p.parse_args()
assert a.source.absolute() != a.output.absolute(), 'External output must differ from original source'
original=a.source.read_bytes();s=original.decode()
old='char *nuvem_rpc_com(const char *funcao, const char *corpoJson,'
new='char *perfilsel_fixture_anon_rpc(const char *funcao, const char *corpoJson,'
assert s.count(old)==1;s=s.replace(old,new)
assert re.findall(r'\bassert\s*\((.*?)\)\s*;',original.decode(),re.S)==re.findall(r'\bassert\s*\((.*?)\)\s*;',s,re.S)
assert s.replace(new,old).encode()==original
# Replace the mirror directory entry atomically. It may initially be a symlink
# to the original fixture; write_text on that entry would follow the symlink.
def replace_entry(path,data):
 path.parent.mkdir(parents=True,exist_ok=True)
 fd,temp=tempfile.mkstemp(prefix='.'+path.name+'.',dir=path.parent)
 try:
  with os.fdopen(fd,'wb') as f:f.write(data)
  os.replace(temp,path)
 finally:
  if os.path.exists(temp):os.unlink(temp)
replace_entry(a.output,s.encode())
assert a.source.read_bytes()==original, 'Original fixture must remain byte-identical'
manifest={'original':str(a.source),'original_sha256':hashlib.sha256(original).hexdigest(),'external_copy':str(a.output),'companion_sha256':hashlib.sha256(s.encode()).hexdigest(),'all_assertions_unchanged':True,'only_change':'Rename anonymous avatar RPC stub; authenticated scoped RPCs call original sessao_rpc transport fake through external companion'}
replace_entry(a.output.with_suffix('.manifest.json'),json.dumps(manifest,indent=2).encode())
