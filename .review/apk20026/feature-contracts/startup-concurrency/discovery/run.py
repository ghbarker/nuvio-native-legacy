#!/usr/bin/env python3
"""Actual discovery orchestration regression; fixed expectations never inferred."""
import argparse, hashlib, json, os, pathlib, platform, subprocess, time
p=argparse.ArgumentParser();p.add_argument('--source',required=True);p.add_argument('--out',required=True);p.add_argument('--fixed',choices=['0','1'],default='1');p.add_argument('--sanitize',action='store_true');a=p.parse_args()
src=pathlib.Path(a.source).resolve();out=pathlib.Path(a.out).resolve();out.mkdir(parents=True,exist_ok=True);here=pathlib.Path(__file__).resolve().parent
flags=['-O1','-g','-Isrc','-Itests','-I'+str(here/'sdk'),'-I'+str(here),'-pthread','-ffunction-sections','-fdata-sections']
flags+=['-Wl,-dead_strip'] if platform.system()=='Darwin' else ['-Wl,--gc-sections']
if a.sanitize:
 flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
 if platform.system()=='Linux':flags+=['-no-pie']
private=os.environ.get('NV_TEST_LOCAL_LOGS')=='1'
if private:flags+=['-DNV_TEST_LOCAL_LOGS=1']
catalog=(src/'src/catalogo.c').read_text();begin=catalog.index('void cat_republicar_fileiras(');end=catalog.index('\n}\n',begin)+3
assert (here/'catalog-republish-actual.inc').read_text()==catalog[begin:end], 'copied production catalogue helper drifted'
command=[os.environ.get('CC','cc'),*flags,'-DDISCOVERY_SOURCE="'+str(src/'src/descoberta.c')+'"',str(here/'latch.c'),str(here/'homejanelas-dependencies.c'),'src/perfis.c','src/cwordem.c','src/cotacat.c','src/js.c','src/metaprov.c','src/colecoes.c','src/redeurl.c','src/catordem.c','-o',str(out/'discovery-latch')]
env=os.environ.copy()
if a.sanitize:
 env['ASAN_OPTIONS']='detect_leaks='+('0' if platform.system()=='Darwin' else '1')+':halt_on_error=1';env['UBSAN_OPTIONS']='halt_on_error=1:print_stacktrace=1'
records=[]
def run(cmd,name):
 t=time.monotonic();r=subprocess.run(cmd,cwd=src,env=env,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=90);log=out/(name+'.log');log.write_text(r.stdout);records.append({'name':name,'command':cmd,'exit_code':r.returncode,'seconds':round(time.monotonic()-t,3),'log':log.name,'log_sha256':hashlib.sha256(log.read_bytes()).hexdigest()});
 if r.returncode:raise RuntimeError(name+' failed:\n'+r.stdout[-5000:])
try:
 run(command,'compile')
 for case in (['fresh'] if a.fixed=='0' else ['fresh','cached','clear','stale']):run([str(out/'discovery-latch'),a.fixed,case],case)
finally:
 hashes={str(f.relative_to(src)):hashlib.sha256(f.read_bytes()).hexdigest() for f in [src/'src/descoberta.c',src/'src/catalogo.c',src/'src/perfis.c']}
 receipt={'all_passed':bool(records) and all(r['exit_code']==0 for r in records),'fixed':a.fixed=='1','sanitized':a.sanitize,'source':str(src),'private':private,'source_sha256':hashes,'fixture_sha256':{f.name:hashlib.sha256(f.read_bytes()).hexdigest() for f in sorted(here.glob('*')) if f.is_file()},'catalog_boundary':'Existing Home catalogue storage double with faithful CW splice; production cat_republicar_fileiras helper is copied verbatim and bound only to fixture storage/mutex/no-op notifications. Actual discovery/perfis compiled unchanged. Full-source catalogue ownership suites remain separate required gates.','records':records}
 if (out/'discovery-latch').exists():receipt['binary_sha256']=hashlib.sha256((out/'discovery-latch').read_bytes()).hexdigest()
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(json.dumps({'all_passed':True,'cases':len(records)-1,'receipt':str(out/'receipt.json')}))
