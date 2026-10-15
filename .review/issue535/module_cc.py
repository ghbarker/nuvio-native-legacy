#!/usr/bin/env python3
"""Record exact repository-script compiler calls without changing their flags."""
from pathlib import Path
import hashlib,json,os,subprocess,sys,time
args=sys.argv[1:];cc=os.environ['REVIEW_REAL_CC'];directory=Path(os.environ['REVIEW_CC_OUTPUT']);directory.mkdir(parents=True,exist_ok=True)
added=['-fno-pie','-no-pie']if sys.platform.startswith('linux')and os.environ.get('SANITIZE')=='1'else[]
record={'compiler':cc,'original_arguments':args,'added_linux_flags':added,'effective_arguments':args+added}
p=directory/(str(time.time_ns())+'-'+str(os.getpid())+'.json');p.write_text(json.dumps(record,indent=2)+'\n')
r=subprocess.run([cc,*args,*added]);record['returncode']=r.returncode
if r.returncode==0 and '-o'in args:
 binary=Path(args[args.index('-o')+1]).resolve();record.update(binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest(),binary_bytes=binary.stat().st_size)
p.write_text(json.dumps(record,indent=2)+'\n');raise SystemExit(r.returncode)
