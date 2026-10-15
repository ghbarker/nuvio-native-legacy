"""Only the two affected public Android regressions for PR431."""
from pathlib import Path
import argparse,ast,json,os,shutil,subprocess,sys
from common import HERE,source_binding,record,sha
p=argparse.ArgumentParser()
for key in ['source','proof','output']:p.add_argument('--'+key,type=Path,required=True)
a=p.parse_args();s=a.source.resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
plan,v,m=source_binding(s,a.proof);assert m['pr']==431
kotlinc=shutil.which('kotlinc');java=shutil.which('java');assert kotlinc and java
rows=[record(out,'kotlin-version',[kotlinc,'-version'],timeout=60)]
env=dict(os.environ,NUVIO_REVIEW_SOURCE=str(s),KOTLINC=kotlinc,JAVA=java)
rows.append(record(out,'interface-mode',[sys.executable,str(HERE/'fixtures/pr431/android_interface_mode.py')],env=env,timeout=240))
# Adapt only the upstream harness tool discovery. All production inputs, embedded
# Kotlin test sources and Python assertions remain byte-for-byte/AST identical.
original=(s/'tests/registro_download.py').read_text()
discovery="""gradle = next((Path.home() / '.gradle/wrapper/dists/gradle-8.9-bin').glob('*/gradle-8.9/lib'))
stdlib = gradle / 'kotlin-stdlib-1.9.23.jar'
java_home = subprocess.check_output(['/usr/libexec/java_home', '-v', '17'], text=True).strip()
java = str(Path(java_home) / 'bin/java')"""
replacement="""gradle = Path(os.environ['NUVIO_REVIEW_KOTLIN_LIB'])
stdlib = gradle / 'kotlin-stdlib.jar'
java = os.environ['JAVA']"""
assert original.count(discovery)==1
adapted=original.replace(discovery,replacement)
def assertions(text):return [ast.dump(n,include_attributes=False)for n in ast.walk(ast.parse(text))if isinstance(n,ast.Assert)]
assert assertions(original)==assertions(adapted)
def embedded(text):
 t=ast.parse(text)
 return [ast.get_source_segment(text,n.value)for n in ast.walk(t)if isinstance(n,ast.Assign)and any(isinstance(v,ast.Name)and v.id=='sources'for v in n.targets)]
assert len(embedded(original))==1 and embedded(original)==embedded(adapted)
kotlin_lib=Path(kotlinc).resolve().parent.parent/'lib'
assert (kotlin_lib/'kotlin-compiler.jar').is_file()and (kotlin_lib/'kotlin-stdlib.jar').is_file(),kotlin_lib
env['NUVIO_REVIEW_KOTLIN_LIB']=str(kotlin_lib)
adapter=out/'registro-download-adapted.py';adapter.write_text(adapted)
# Preserve __file__ so the upstream root assignment still selects the exact
# candidate checkout, although the generated adapter is kept outside it.
launcher=out/'registro-download-launch.py'
launcher.write_text("import runpy\nfrom pathlib import Path\nsrc="+repr(str(s/'tests/registro_download.py'))+"\nadapter="+repr(str(adapter))+"\nexec(compile(Path(adapter).read_text(),src,'exec'),{'__file__':src,'__name__':'__main__'})\n")
rows.append(record(out,'registro-download',[sys.executable,str(launcher)],env=env,timeout=240))
adapter_proof={'boundary':'Kotlin compiler/std-library and Java discovery only','original_sha256':sha(s/'tests/registro_download.py'),'generated_sha256':sha(adapter),'launcher_sha256':sha(launcher),'python_assertions_unchanged':True,'embedded_kotlin_sources_unchanged':True,'assertion_count':len(assertions(original)),'kotlin_lib':str(kotlin_lib),'compiler_jar_sha256':sha(kotlin_lib/'kotlin-compiler.jar'),'stdlib_sha256':sha(kotlin_lib/'kotlin-stdlib.jar')}

source_binding(s,a.proof)
r={'outcome':'passed','source_binding':v,'plan_sha256':sha(HERE/'plan.json'),'groups':['interface-mode','registro-download'],'gates':rows,'compiler_sha256':sha(kotlinc),'java_sha256':sha(java),'assertion_source_sha256':{'interface-mode':sha(HERE/'fixtures/pr431/android_interface_mode.py'),'registro-download':sha(s/'tests/registro_download.py')},'registro_adapter':adapter_proof,'scope':'Fresh production Kotlin classifier/viewport and upstream download/process-recreation regressions only; no broad JVM suite.'}
(out/'kotlin-verified.json').write_text(json.dumps(r,indent=2)+'\n');print('PASS affected Android Kotlin regressions',flush=True)
