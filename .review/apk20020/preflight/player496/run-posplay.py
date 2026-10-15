#!/usr/bin/env python3
from pathlib import Path
from portable import CC, SDK, SDK_EXTRA, SECTION_FLAGS, PLATFORM_FLAGS, DEAD_STRIP, sanitizer_flags, execution_env, unresolved_symbols, logical_symbol
import argparse,hashlib,json,subprocess
p=argparse.ArgumentParser();p.add_argument('--private-logs',action='store_true');p.add_argument('--source',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();s=a.source.resolve();o=a.output.resolve();o.mkdir(parents=True,exist_ok=True);here=Path(__file__).resolve().parent;sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest();before=sha(s/'src/posplay.c');records=[]
for san in [False,True]:
 flags=[*CC,*PLATFORM_FLAGS,'-std=gnu11','-O1','-g',*SECTION_FLAGS,'-Werror=implicit-function-declaration','-include',str(SDK_EXTRA),'-I'+str(SDK),'-I'+str(s/'src'),'-DPOSPLAY_SOURCE="'+str(s/'src/posplay.c')+'"']
 if a.private_logs:flags+=['-DNV_TEST_LOCAL_LOGS=1']
 if san:flags+=sanitizer_flags()
 producer_sources=[str(s/'src/sinopse.c')] if (s/'src/sinopse.c').is_file() else []
 binary=o/('posplay-san'if san else'posplay-normal')
 for label,cmd in [('build',flags+[str(here/'posplay-manual.c'),*producer_sources,DEAD_STRIP,'-lm','-pthread','-o',str(binary)]),('run',[str(binary)])]:
  z=subprocess.run(cmd,capture_output=True,text=True,env=execution_env());log=o/((binary.name)+'-'+label+'.log');log.write_text(z.stdout+z.stderr);records.append({'label':label,'sanitized':san,'command':cmd,'returncode':z.returncode,'log_sha256':sha(log)});assert not z.returncode,z.stdout+z.stderr
assert before==sha(s/'src/posplay.c');(o/'result.json').write_text(json.dumps({'outcome':'passed','source':str(s),'source_sha256':before,'fixture_sha256':sha(here/'posplay-manual.c'),'records':records,'scope':'Full actual posplay module manual reset preserves independent warm-next identity; typed cancellation/cache boundaries only.'},indent=2)+'\n');print('PASS actual posplay manual cancellation normal + ASan/UBSan')
