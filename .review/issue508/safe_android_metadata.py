"""Safe build facts only; never store commands, raw cache text or key values."""
from pathlib import Path
import hashlib,json,re,shlex

def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main_entries(commands):
 out=[]
 for e in commands:
  argv=e.get('arguments') or shlex.split(e['command'])
  if any('CMakeFiles/main.dir/' in x for x in argv):out.append((e,argv))
 return out

def cache_for_commands(source, database, abi, commands):
 app=Path(source).resolve()/'android/app';rows=main_entries(commands);assert rows,'No actual main compilation entries'
 dirs={Path(e['directory']).resolve() for e,args in rows};assert len(dirs)==1,'Main compilation spans multiple build directories'
 directory=dirs.pop();assert directory.is_relative_to(app) and abi in directory.parts,'Build directory is not bound to source/ABI'
 cache=directory/'CMakeCache.txt';assert cache.is_file(),'Canonical CMakeCache missing for actual compile directory'
 text=cache.read_text()
 def value(name):
  found=re.findall(r'^'+re.escape(name)+r':[^=\n]+=(.*)$',text,re.M);assert len(found)==1,'Missing/ambiguous cache field '+name;return found[0].strip()
 assert Path(value('CMAKE_CACHEFILE_DIR')).resolve()==directory,'CMake cache belongs to another build directory'
 assert Path(value('CMAKE_HOME_DIRECTORY')).resolve()==app/'src/main/cpp','CMake cache belongs to another source tree'
 assert value('ANDROID_ABI')==abi,'CMake cache belongs to another ABI'
 return cache

def collect(source,layer):
 source=Path(source).resolve();app=source/'android/app';result={'scope':'Sanitized generated config and actual native compilation facts; no raw command arguments/cache/key values','layer':layer,'build_config':[],'native':[]}
 for f in sorted((app/'build/generated').rglob('BuildConfig.java')):
  fields=re.findall(r'public static final boolean NV_TEST_LOCAL_LOGS\s*=\s*(true|false)\s*;',f.read_text())
  result['build_config'].append({'path':str(f.relative_to(source)),'sha256':sha(f),'private_opt_in_fields':[v=='true' for v in fields]})
 for f in sorted(app.rglob('compile_commands.json')):
  abi=next((a for a in ('arm64-v8a','armeabi-v7a') if a in f.parts),None)
  if not abi:continue
  record={'abi':abi,'path':str(f.relative_to(source)),'sha256':sha(f)}
  try:
   commands=json.loads(f.read_text());rows=main_entries(commands);facts=[]
   for e,args in rows:
    report=[x.split('=',1)[1] for x in args if x.startswith('-DNV_REC_URL=')]
    facts.append({'file':Path(e['file']).name,'android':any(x in args for x in ('-DNV_ANDROID','-DNV_ANDROID=1')),'touch':any(x in args for x in ('-DNV_TOUCH_UI','-DNV_TOUCH_UI=1')),'private_opt_in':'-DNV_TEST_LOCAL_LOGS=1' in args,'report_macro_count':len(report),'report_endpoint_configured':len(report)==1 and bool(report[0].replace(chr(92),'').strip(chr(34)))})
   record['main_sources']=facts
   if rows:
    cache=cache_for_commands(source,f,abi,commands);text=cache.read_text();record['cache']={'path':str(cache.relative_to(source)),'sha256':sha(cache),'private_opt_in':re.findall(r'^NV_TEST_LOCAL_LOGS:[^=\n]+=(ON|OFF)\s*$',text,re.M),'sibling_of_database':cache.parent==f.parent,'source_abi_build_bound':True}
  except Exception as e:record['metadata_error_type']=type(e).__name__;record['metadata_error']=str(e) if isinstance(e,AssertionError) else 'Build metadata could not be parsed'
  result['native'].append(record)
 return result
