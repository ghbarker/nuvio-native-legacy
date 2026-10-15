"""Strict same-job object-reuse evidence for the one explicit private macro delta."""
from pathlib import Path
import hashlib,shlex
PRIVATE_MACRO=b'NV_TEST_LOCAL_LOGS'
PRIVATE_DEFINE='-DNV_TEST_LOCAL_LOGS=1'
def translation_units(source):
 source=Path(source).resolve()
 return sorted([p.relative_to(source) for p in (source/'src').glob('*.c')]+[p.relative_to(source) for p in (source/'src/dts').glob('*.c')])
def assert_translation_sets(product,test):
 a,b=translation_units(product),translation_units(test)
 assert a and a==b,'Product/test translation unit sets differ; private extras must never be ignored'
 return a

def dependency_snapshot(depfile,obj,source,product):
 product=Path(product).resolve();source=Path(source).resolve();obj=Path(obj).resolve();raw=Path(depfile).read_text();tokens=shlex.split(raw.replace('\\\n',' '))
 assert len(tokens)>1 and tokens[0]==str(obj)+':','Missing or malformed dependency target'
 paths=[]
 for token in tokens[1:]:
  path=Path(token);assert path.is_absolute() and path.is_file(),f'Unverifiable dependency: {token}'
  path=path.resolve()
  if path not in paths:paths.append(path)
 assert paths and source in paths,'Dependency list must include its actual source translation unit'
 entries=[]
 for path in paths:
  data=path.read_bytes()
  try:local=str(path.relative_to(product))
  except ValueError:local=None
  entries.append({'path':str(path),'local':local,'sha256':hashlib.sha256(data).hexdigest(),'private_macro_sensitive':PRIVATE_MACRO in data})
 return entries

def reuse_evidence(snapshot,product,test):
 assert snapshot,'No object may be reused without a nonempty parsed dependency closure'
 product=Path(product).resolve();test=Path(test).resolve();differences=[];sensitive=[];checked=[]
 for e in snapshot:
  original=Path(e['path']);assert original.is_file() and hashlib.sha256(original.read_bytes()).hexdigest()==e['sha256'],'A compiled dependency changed after object production'
  candidate=test/e['local'] if e['local'] is not None else original
  if not candidate.is_file():differences.append(str(candidate));continue
  data=candidate.read_bytes();digest=hashlib.sha256(data).hexdigest()
  if digest!=e['sha256']:differences.append(str(candidate))
  if e['private_macro_sensitive'] or PRIVATE_MACRO in data:sensitive.append(str(candidate))
  checked.append({'path':e['local'] or e['path'],'sha256':digest})
 return {'eligible':not differences and not sensitive,'changed_dependencies':differences,'private_macro_sensitive_dependencies':sensitive,'checked_dependencies':checked,'compiler_macro_delta':[PRIVATE_DEFINE],'macro_compatibility':'Recompile every dependency closure containing the differing macro; reuse only if no dependency can observe it.'}
