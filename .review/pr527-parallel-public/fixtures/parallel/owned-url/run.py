#!/usr/bin/env python3
"""Exercise the actual manifest reader's retained URL lifetime."""
import argparse, hashlib, json, os, pathlib, platform, subprocess
p=argparse.ArgumentParser();p.add_argument('--source',required=True);p.add_argument('--support',required=True);p.add_argument('--out',required=True);p.add_argument('--sanitize',action='store_true');p.add_argument('--expect-bug',action='store_true');a=p.parse_args()
src=pathlib.Path(a.source).resolve();support=pathlib.Path(a.support).resolve();out=pathlib.Path(a.out).resolve();out.mkdir(parents=True,exist_ok=True);here=pathlib.Path(__file__).resolve().parent
source_before=hashlib.sha256((src/'src/descoberta.c').read_bytes()).hexdigest()
flags=['-O1','-g','-Isrc','-Itests','-I'+str(support/'sdk'),'-I'+str(support),'-pthread','-ffunction-sections','-fdata-sections']
flags += ['-Wl,-dead_strip'] if platform.system()=='Darwin' else ['-Wl,--gc-sections']
if a.sanitize:
 flags += ['-fsanitize=address,undefined','-fsanitize-address-use-after-scope','-fno-omit-frame-pointer']
 if platform.system()=='Linux': flags+=['-no-pie']
command=[os.environ.get('CC','cc'),*flags,'-DDISCOVERY_SOURCE="'+str(src/'src/descoberta.c')+'"',str(here/'retained.c'),str(support/'homejanelas-dependencies.c'),'src/perfis.c','src/cwordem.c','src/cotacat.c','src/js.c','src/metaprov.c','src/colecoes.c','src/redeurl.c','src/catordem.c','-o',str(out/'retained')]
env=os.environ.copy();env['ASAN_OPTIONS']='detect_leaks=0:detect_stack_use_after_return=1:halt_on_error=1';env['UBSAN_OPTIONS']='halt_on_error=1:print_stacktrace=1'
r=subprocess.run(command,cwd=src,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=90);(out/'compile.log').write_text(r.stdout);assert r.returncode==0,r.stdout
r=subprocess.run([str(out/'retained')],cwd=src,env=env,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30);(out/'run.log').write_text(r.stdout)
if a.expect_bug: assert r.returncode and ('stack-use-after-return' in r.stdout or 'stack-use-after-scope' in r.stdout) and 'lerManifesto' in r.stdout,r.stdout
else: assert r.returncode==0 and 'PASS: actual manifest declaration owns URL after reader return and compaction' in r.stdout,r.stdout
h=lambda f:hashlib.sha256(f.read_bytes()).hexdigest()
source_after=h(src/'src/descoberta.c')
assert source_before==source_after, 'source changed during validation'
receipt={'passed':True,'source_unchanged':True,'source_sha256_before':source_before,'expected_bug':a.expect_bug,'sanitize':a.sanitize,'source':str(src),'head':subprocess.check_output(['git','rev-parse','HEAD'],cwd=src,text=True).strip(),'source_sha256':h(src/'src/descoberta.c'),'fixture_sha256':h(here/'retained.c'),'support_sha256':{f.name:h(f) for f in sorted(support.glob('*')) if f.is_file()},'compile':command,'run_exit':r.returncode,'log_sha256':h(out/'run.log'),'binary_sha256':h(out/'retained')}
(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt,sort_keys=True))
