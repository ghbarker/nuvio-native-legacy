#!/usr/bin/env python3
"""Focused actual request-owned collection reader contracts. No product edits."""
import argparse,hashlib,json,os,pathlib,platform,subprocess,time
p=argparse.ArgumentParser();p.add_argument('--source',required=True);p.add_argument('--out',required=True);p.add_argument('--sanitize',action='store_true');p.add_argument('--private',action='store_true');p.add_argument('--sdk',type=pathlib.Path,required=True);a=p.parse_args()
src=pathlib.Path(a.source).resolve();out=pathlib.Path(a.out).resolve();out.mkdir(parents=True,exist_ok=True);here=pathlib.Path(__file__).resolve().parent
sdk=a.sdk.resolve();assert sdk.is_dir()
flags=['-O1','-g','-Isrc','-I'+str(sdk),'-pthread','-ffunction-sections','-fdata-sections']
if a.private:flags+=['-DNV_TEST_LOCAL_LOGS=1']
flags+=['-Wl,-dead_strip'] if platform.system()=='Darwin' else ['-Wl,--gc-sections']
if a.sanitize:
 flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
 if platform.system()=='Linux':flags+=['-no-pie']
cmd=[os.environ.get('CC','cc'),*flags,'-DDISCOVERY_SOURCE="'+str(src/'src/descoberta.c')+'"',str(here/'reader.c'),'src/js.c','src/perfis.c','src/redeurl.c','src/metaprov.c','-o',str(out/'backend-reader')]
env=os.environ.copy()
if a.sanitize:env.update(ASAN_OPTIONS='detect_leaks='+('0' if platform.system()=='Darwin' else '1')+':halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
records=[]
def run(c,n):
 t=time.monotonic();r=subprocess.run(c,cwd=src,env=env,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=60);f=out/(n+'.log');f.write_text(r.stdout);records.append(dict(name=n,command=c,exit_code=r.returncode,seconds=round(time.monotonic()-t,3),log=f.name,log_sha256=hashlib.sha256(f.read_bytes()).hexdigest()))
 if r.returncode:raise RuntimeError(n+' failed:\n'+r.stdout[-8000:])
try:
 run(cmd,'compile')
 for case in ['addon','genre','received-supplement','late-supplement','reserve-hidden-off','watched-pages','watched-bounded','raw-skip','duplicate-pages','top100','top250','tmdb-discover','tmdb-list','tmdb-collection','tmdb-person','tmdb-director','tmdb-company','tmdb-network','trakt','trakt-stale','trakt-compact-first','trakt-compact-last','trakt-compact-both','empty-spaces','trakt-empty-spaces','null-array','object-array','string-array','number-array','truncated-array','trakt-truncated-array','empty','failure','invalid','disabled','stale-before','stale-during','overlap','overlap-mixed']:
  run([str(out/'backend-reader'),case],case)
finally:
 receipt={'all_passed':len(records)>1 and all(r['exit_code']==0 for r in records),'sanitized':a.sanitize,'private':a.private,'source':str(src),'source_sha256':{f:hashlib.sha256((src/f).read_bytes()).hexdigest() for f in ['src/descoberta.c','src/descoberta.h','src/perfis.c','src/js.c','src/redeurl.c','src/metaprov.c']},'fixture_sha256':{f.name:hashlib.sha256(f.read_bytes()).hexdigest() for f in here.glob('*') if f.is_file()},'boundary':'Actual descoberta.c included unchanged, actual JSON parser, profile session implementation, addon URL and metadata helpers. Only external network, read-only settings/addon/catalog lookup, i18n and Trakt artwork boundaries are controlled doubles. No real network or fixture mutation of product reader logic.','records':records}
 if (out/'backend-reader').exists():receipt['binary_sha256']=hashlib.sha256((out/'backend-reader').read_bytes()).hexdigest()
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(json.dumps({'all_passed':True,'cases':len(records)-1,'receipt':str(out/'receipt.json')}))
