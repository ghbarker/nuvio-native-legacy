"""Original reporting/Settings scripts, with an explicit stale enum-tail adapter."""
from pathlib import Path
import argparse,json,os,subprocess,sys
from common import HERE,sha,source_binding,record
p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);p.add_argument('--proof',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--contracts',type=Path,required=True);a=p.parse_args();s=a.source.resolve();o=a.output.resolve();o.mkdir(parents=True,exist_ok=False)
plan,v,m=source_binding(s,a.proof);contracts=json.loads((a.contracts/'report.json').read_text());assert contracts['outcome']=='passed' and contracts['baseline']==plan['base']
last=(a.contracts/'baseline-desktop-tv-execute.log').read_text().splitlines()[-1].split('\t');assert last[0:2]==['290','-provedoresEscopo']
ccbin=o/'cc-bin';ccbin.mkdir();wrapper=ccbin/'cc';wrapper.write_text('#!/bin/sh\nexec python3 "'+str(HERE/'cc_rpath.py')+'" "$@"\n');wrapper.chmod(0o755)
sdl3=Path(subprocess.check_output(['brew','--prefix','sdl3'],text=True).strip())/'lib';assert sdl3.is_dir()
env=dict(os.environ,PATH=str(ccbin)+os.pathsep+os.environ['PATH'],REVIEW_REAL_CC='/usr/bin/cc',REVIEW_SDL3_LIBRARY_DIR=str(sdl3),REVIEW_CC_OUTPUT=str(o/'compile'))
rows=[];original={};adapter=None
for suite in ['ajustes_envio_auto','ajustes_padroes','ajustes_ux_toque_modo','regcodigo']:
 for ext in ['c','sh']:
  rel='tests/'+suite+'.'+ext;data=(s/rel).read_bytes();assert data==subprocess.check_output(['git','show',plan['base']+':'+rel],cwd=s);original[rel]=sha(s/rel)
 script=s/'tests'/(suite+'.sh')
 if suite=='ajustes_padroes':
  old='assert(AJ_N == AJ_PROXIMO_CONTAGEM + 1);';new='assert(AJ_N == AJ_PROVEDORES_ESCOPO + 1);';include='#include "../src/ajustes.c"';text=(s/'tests/ajustes_padroes.c').read_text();assert text.count(old)==text.count(include)==1
  adapted=text.replace(old,new,1).replace(include,'#include "'+str(s/'src/ajustes.c')+'"',1);assert adapted.replace(new,old,1).replace('#include "'+str(s/'src/ajustes.c')+'"',include,1)==text
  fixture=o/'defaults.c';fixture.write_text(adapted);shell=script.read_text();assert shell.count('cd "$(dirname "$0")/.."')==1 and shell.count('tests/ajustes_padroes.c')==1
  replay=shell.replace('cd "$(dirname "$0")/.."','cd "$REVIEW_PRODUCT_ROOT"',1).replace('tests/ajustes_padroes.c','"$REVIEW_DEFAULTS_FIXTURE"',1);script=o/'defaults.sh';script.write_text(replay)
  env.update(REVIEW_PRODUCT_ROOT=str(s),REVIEW_DEFAULTS_FIXTURE=str(fixture));adapter={'only_assertion_change':'Stale final-enum metadata names current upstream AJ_PROVEDORES_ESCOPO; every behavioral/default assertion unchanged','original_sha256':original['tests/ajustes_padroes.c'],'adapted_sha256':sha(fixture),'original_unadapted_counted_as_pass':False,'baseline_table_tail':last}
 rows.append(record(o,suite,['bash',str(script)],cwd=s,env=env))
rows.append(record(o,'envio-auto-asan',[sys.executable,str(HERE/'run_original_sanitized.py'),'--name','ajustes_envio_auto','--output',str(o/'sanitized')],cwd=s,env=env))
san=json.loads((o/'sanitized/ajustes_envio_auto-sanitized.json').read_text());assert san['exit_code']==0 and san['original_source_arguments_and_assertions_unchanged']
compiles=[json.loads(x.read_text())for x in (o/'compile').glob('*-invocation.json')];assert compiles and all(x['returncode']==0 and x['effective_arguments']==x['arguments']+([x['added_link_rpath']]if x['added_link_rpath']else[])for x in compiles)
source_binding(s,a.proof);result={'outcome':'passed','source_binding':v,'records':rows,'original_test_sha256':original,'defaults_metadata_adapter':adapter,'sanitized_original_replay':san,'compiler_invocations':len(compiles),'compiler_proofs':{str(x.relative_to(o)):sha(x)for x in (o/'compile').glob('*-invocation.json')}}
(o/'reporting-verified.json').write_text(json.dumps(result,indent=2)+'\n');print('PASS original Send/View/preferences/reporting integration')
