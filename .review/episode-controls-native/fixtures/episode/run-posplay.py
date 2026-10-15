#!/usr/bin/env python3
from pathlib import Path
import argparse,hashlib,json,subprocess
p=argparse.ArgumentParser();p.add_argument('--cc',default='cc');p.add_argument('--source',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--san',action='store_true');a=p.parse_args();s=a.source.resolve();o=a.output.resolve();o.mkdir(parents=True,exist_ok=True);here=Path(__file__).resolve().parent;sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest();before=sha(s/'src/posplay.c');records=[]
for san in [a.san]:
 flags=[a.cc,'-D_DARWIN_C_SOURCE','-std=gnu11','-O1','-g','-ffunction-sections','-fdata-sections','-Werror=implicit-function-declaration','-include',str(here/'sdk-extra.h'),'-I'+str(here/'sdk'),'-I'+str(s/'src'),'-DPOSPLAY_SOURCE="'+str(s/'src/posplay.c')+'"']
 if san:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer',*([]if __import__('sys').platform=='darwin'else['-fno-pie','-no-pie'])]
 binary=o/('posplay-san'if san else'posplay-normal')
 for label,cmd in [('build',flags+[str(here/'posplay-manual.c'),*(['-Wl,-dead_strip']if __import__('sys').platform=='darwin'else['-Wl,--gc-sections']),'-lm','-pthread','-o',str(binary)]),('run',[str(binary)])]:
  z=subprocess.run(cmd,capture_output=True,text=True);log=o/((binary.name)+'-'+label+'.log');log.write_text(z.stdout+z.stderr);records.append({'label':label,'sanitized':san,'command':cmd,'returncode':z.returncode,'log_sha256':sha(log)});assert not z.returncode,z.stdout+z.stderr
assert before==sha(s/'src/posplay.c');(o/'result.json').write_text(json.dumps({'outcome':'passed','source':str(s),'source_sha256':before,'fixture_sha256':sha(here/'posplay-manual.c'),'records':records,'scope':'Full actual posplay module manual reset preserves independent warm-next identity; typed cancellation/cache boundaries only.'},indent=2)+'\n');print('PASS actual posplay manual cancellation normal + ASan/UBSan')
