#!/usr/bin/env python3
"""Replay the retained135 plus combined30-check cache fixture against current production sources."""
from pathlib import Path
import argparse,hashlib,json,os,platform,re,shlex,subprocess
p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--cc',default='/usr/bin/cc');p.add_argument('--sanitize',action='store_true');a=p.parse_args();s=a.source.resolve();o=a.output.resolve();f=Path(__file__).resolve().parent;o.mkdir(parents=True,exist_ok=False)
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def git(*args):return subprocess.check_output(['git','-C',str(s),*args],text=True).strip()
head,tree=git('rev-parse','HEAD'),git('rev-parse','HEAD^{tree}');assert not git('status','--porcelain')
assert sha(f/'cache_refresh.c')=='75faa0b4d9600965a1beb28ced44e57225b2ce4fe5f4775632ea5b808d125a63'
records=[];deps={};fixture={str(x.relative_to(f)):sha(x)for x in f.rglob('*')if x.is_file()and x.suffix in ['.c','.h','.py']}
def run(label,command,env=None):
 with (o/(label+'.log')).open('w')as log:r=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,env=env,timeout=180)
 row={'label':label,'command':command,'returncode':r.returncode,'log':label+'.log','log_sha256':sha(o/(label+'.log'))};records.append(row)
 if r.returncode:print((o/(label+'.log')).read_text()[-6000:]);raise SystemExit(r.returncode)
 return (o/(label+'.log')).read_text()
for mode,defs in [('tv',[]),('mobile',['-DNV_ANDROID=1','-DNV_TOUCH_UI=1'])]:
 flags=['-std=gnu11','-O1','-g','-ffunction-sections','-fdata-sections','-I'+str(f/'sdk'),'-I'+str(s/'src'),'-DNV_STREAMS_SOURCE="'+str(s/'src/streams.c')+'"','-DNV_CACHE_FIX=1','-DNV_CAPTION_NEGATIVE=1','-DNV_NATIVE_REFRESH=1',*defs]
 if a.sanitize:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer']+(['-fno-pie']if platform.system()=='Linux'else[])
 objects=[]
 units=[f/'cache_refresh.c',*[s/'src'/x for x in ['fontefalha.c','badges.c','naovideo.c','linguas.c','stream_parse.c','js.c','fontefiltro.c','fontenota.c']]]
 for index,unit in enumerate(units):
  name=mode+'-'+str(index)+'-'+unit.stem;obj=o/(name+'.o');dep=o/(name+'.d');run(name,[a.cc,*flags,'-MMD','-MF',str(dep),'-c',str(unit),'-o',str(obj)]);objects.append(str(obj))
  for token in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
   path=Path(token).resolve()
   try:rel=str(path.relative_to(s))
   except ValueError:continue
   digest=sha(path);assert rel not in deps or deps[rel]==digest;deps[rel]=digest
 binary=o/('cache-'+mode);link=['-Wl,-dead_strip']if platform.system()=='Darwin'else['-Wl,--gc-sections']
 if a.sanitize and platform.system()=='Linux':link+=['-no-pie']
 run(mode+'-link',[a.cc,*flags,*objects,*link,'-pthread','-lm','-o',str(binary)])
 text=run(mode+'-execute',[str(binary)],dict(os.environ,ASAN_OPTIONS=('detect_leaks=0:abort_on_error=1'if platform.system()=='Darwin'else'detect_leaks=1:abort_on_error=1'),UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1'))
 assert text.splitlines().count('refresh checks=135 failures=0 existingResolverCalls=13')==1
 assert text.splitlines().count('combined cache/audio checks=30 failures=0 existingResolverCalls=13')==1
 assert not re.search(r'ERROR: AddressSanitizer|runtime error:|SUMMARY: (AddressSanitizer|UndefinedBehaviorSanitizer)',text)
 records[-1].update(binary_sha256=sha(binary),binary_bytes=binary.stat().st_size)
assert git('rev-parse','HEAD')==head and git('rev-parse','HEAD^{tree}')==tree and not git('status','--porcelain')
for rel,digest in deps.items():assert sha(s/rel)==digest
receipt={'outcome':'passed','source_commit':head,'source_tree':tree,'source_clean':True,'modes':['tv','mobile'],'checks_per_mode':135,'combined_cache_audio_checks_per_mode':30,'existing_offline_resolver_calls_per_mode':13,'sanitizers':['address','undefined']if a.sanitize else[],'product_dependency_sha256':deps,'fixture_sha256':fixture,'records':records,'scope':'Complete production streams.c parser/manual-native/cache-negative/icon/caption/Cached-chip paths with typed offline media/provider/GPU boundaries. No real network, provider quota, decoder or device execution.'}
(o/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print('PASS cache truth135 + combined audio30 checks per TV/Mobile mode',head)
