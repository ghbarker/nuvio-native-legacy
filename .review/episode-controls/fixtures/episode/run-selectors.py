#!/usr/bin/env python3
"""Run actual episode selectors and unchanged original assertions; never edit source tests."""
from pathlib import Path
import argparse, hashlib, json, os, subprocess

p=argparse.ArgumentParser()
p.add_argument('--cc',default='cc');p.add_argument('--source',type=Path,required=True)
p.add_argument('--output',type=Path,required=True)
p.add_argument('--previous',action='store_true');p.add_argument('--san',action='store_true')
a=p.parse_args(); source=a.source.resolve(); out=a.output.resolve(); out.mkdir(parents=True,exist_ok=True)
here=Path(__file__).resolve().parent
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
watched=[source/'src/proximo.c',source/'src/proximo.h',source/'src/catalogo.h',source/'tests/proximo.c']
before={str(p.relative_to(source)):sha(p)for p in watched};records=[]
for fixture in ['original','selectors']:
 for san in [a.san]:
  label=fixture+('-asan'if san else '-normal');binary=out/label
  flags=[a.cc,'-D_DARWIN_C_SOURCE','-std=gnu11','-O1','-g','-Wall','-Wextra','-Werror','-ffunction-sections','-fdata-sections','-I'+str(source/'src')]
  if san:flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-sanitize-recover=all',*([]if __import__('sys').platform=='darwin'else['-fno-pie','-no-pie'])]
  inputs=[source/'tests/proximo.c']if fixture=='original'else[here/'selector-cases.c',source/'src/proximo.c']
  if fixture=='selectors':flags+=['-DHAVE_PREVIOUS='+str(int(a.previous))]
  command=flags+list(map(str,inputs))+[*(['-Wl,-dead_strip']if __import__('sys').platform=='darwin'else['-Wl,--gc-sections']),'-lm','-o',str(binary)]
  z=subprocess.run(command,capture_output=True,text=True);(out/(label+'.compile.log')).write_text(z.stdout+z.stderr)
  assert not z.returncode,z.stderr
  run=subprocess.run([str(binary)],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS=('detect_leaks=0:halt_on_error=1'if __import__('sys').platform=='darwin'else'detect_leaks=1:halt_on_error=1'),UBSAN_OPTIONS='halt_on_error=1'))
  (out/(label+'.run.log')).write_text(run.stdout+run.stderr);assert not run.returncode,run.stdout+run.stderr
  records.append({'label':label,'command':command,'compile_returncode':z.returncode,'run_returncode':run.returncode,'output':run.stdout.strip(),'compile_log_sha256':sha(out/(label+'.compile.log')),'run_log_sha256':sha(out/(label+'.run.log'))})
  print(label,run.stdout.strip().splitlines()[-1],flush=True)
assert before=={str(p.relative_to(source)):sha(p)for p in watched}
(out/'result.json').write_text(json.dumps({'outcome':'passed','source':str(source),'source_head':subprocess.check_output(['git','-C',str(source),'rev-parse','HEAD'],text=True).strip(),'source_hashes':before,'source_unchanged':True,'previous_included':a.previous,'fixture_sha256':sha(here/'selector-cases.c'),'records':records,'scope':'Actual production pure selectors; original proximo C assertions unchanged. Linux host compilation, no UI or playback claim.'},indent=2)+'\n')
