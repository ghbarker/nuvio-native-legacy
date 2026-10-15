"""Run the actual checked-in collection script against complete public source."""
from pathlib import Path
import argparse,json,os,platform,subprocess
from common import HERE,sha,source_binding
p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);p.add_argument('--proof',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--mode',choices=['normal','asan'],required=True);a=p.parse_args()
s=a.source.resolve();o=a.output.resolve();assert not o.exists();plan,binding,manifest=source_binding(s,a.proof)
assert platform.system()=='Darwin';script=s/'tests/colecao_exibicao.sh';test=s/'tests/colecao_exibicao.c'
assert script.read_bytes()==subprocess.check_output(['git','-C',str(s),'show',manifest['base']+':tests/colecao_exibicao.sh'])
assert sha(script)==plan['original_collection_script_sha256']
assert manifest['reviewed_test_compatibility']==plan['reviewed_test_compatibility']
assert len(manifest['reviewed_test_compatibility'])==1 and manifest['reviewed_test_compatibility'][0]['path']=='tests/colecao_exibicao.c'
assert sha(test)==manifest['reviewed_test_compatibility'][0]['sha256']
cc=Path(os.environ['NUVIO_REVIEW_REAL_CC']).resolve();assert cc==Path('/usr/bin/cc').resolve()
o.mkdir(parents=True);(o/'cc-bin').mkdir();(o/'compile').mkdir();shim=o/'cc-bin/cc';shim.write_bytes((HERE/'collection_cc.py').read_bytes());shim.chmod(0o755)
sdl3=(Path(subprocess.check_output(['brew','--prefix','sdl3'],text=True).strip())/'lib').resolve();assert sdl3.is_dir()
env=dict(os.environ,REVIEW_REAL_CC=str(cc),REVIEW_SDL3_LIBRARY_DIR=str(sdl3),REVIEW_CC_OUTPUT=str(o/'compile'),REVIEW_COLLECTION_SANITIZE='1'if a.mode=='asan'else'0',ASAN_OPTIONS='detect_leaks=0:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1',PATH=str(o/'cc-bin')+os.pathsep+os.environ['PATH'])
log=o/'colecao_exibicao.log'
with log.open('w')as stream:result=subprocess.run(['/bin/bash','tests/colecao_exibicao.sh'],cwd=s,env=env,stdout=stream,stderr=subprocess.STDOUT,timeout=1200)
assert result.returncode==0,log.read_text()[-8000:]
assert 'colecao_exibicao: ok'in log.read_text()
files=sorted((o/'compile').glob('*-invocation.json'));records=[json.loads(path.read_text())for path in files]
assert len(records)==1
record=records[0];san=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-sanitize-recover=all']if a.mode=='asan'else[]
assert record['returncode']==0 and record['compiler']==str(cc)
assert record['added_sanitizer_flags']==san and record['added_link_rpath']=='-Wl,-rpath,'+str(sdl3)
assert record['effective_arguments']==record['arguments']+san+[record['added_link_rpath']]
modules=sorted(str(path.relative_to(s))for path in list((s/'src').glob('*.c'))+list((s/'src/dts').glob('*.c')))
assert set(record['arguments'])&set(modules)==set(modules)-{'src/main.c','src/vertudo.c'}
assert 'tests/colecao_exibicao.c'in record['arguments']and not any('NV_TEST_LOCAL_LOGS'in flag for flag in record['effective_arguments'])
source_binding(s,a.proof)
value={'outcome':'passed','source_binding':binding,'plan_sha256':sha(HERE/'plan.json'),'mode':a.mode,'script_sha256':sha(script),'test_sha256':sha(test),'script_unchanged':True,'test_transport_compatibility_reviewed':True,'full_source_files_compiled':len(modules)-2,'actual_repository_script':True,'sanitizers':['address','undefined']if a.mode=='asan'else[],'added_sanitizer_flags':san,'compiler_shim_sha256':sha(HERE/'collection_cc.py'),'log_sha256':sha(log),'compile_proofs':{str(path.relative_to(o)):sha(path)for path in files},'scope':'Actual unchanged repository Bash command with real complete-source Mac SDK compilation; only reviewed existing test transport/orchestration update, preserving behavior assertions.'}
(o/'original-collection-verified.json').write_text(json.dumps(value,indent=2)+'\n');print('PASS actual repository full-source collection',a.mode)
