#!/usr/bin/env python3
"""Require every declared focused gate, source binding and real full build."""
import hashlib
import json
import os
from pathlib import Path

out=Path(os.environ['NUVIO_SOURCE_PRELOAD_RESULTS'])
case=json.loads((out/'case.json').read_text())
assert case['phase']=='final'
required=['subtitle-title-fixtures','linguas-normal','linguas-asan']
for name in ('legendasui','legendasui_425'):
 required += [name+'-original',name+'-original-asan']
for name in ('legmemoria','faixasmkv','faixasmkv_casar'):
 required += [name+'-original',name+'-sanitized-replay']
if case['case']['delivery']:
 required += ['original-log-source','original-log-and-download-guards','original-log-and-download-guards-asan']
else:
 required += ['subtitle-visual-tv','subtitle-visual-touch']
assert sorted((out/'results.txt').read_text().splitlines())==sorted(n+' PASS' for n in required)
assert (out/'app-tv').is_file() and (out/'app-mobile').is_file()
source=json.loads((out.parent/'source-verified.json').read_text())
assert source['verified'] and source['case']==case['case']['id'] and source['base']==case['case']['base']
originals=json.loads((out/'original-tests-verified.json').read_text())
assert len(originals)==11
replays=[]
for name in ('legmemoria','faixasmkv','faixasmkv_casar'):
 proof=json.loads((out/'sanitized-originals'/(name+'-sanitized.json')).read_text())
 assert proof['exit_code']==0 and proof['original_source_arguments_and_assertions_unchanged']
 assert proof['original_sha256']==originals['tests/'+name+'.sh']
 replays.append(proof)
titles=json.loads((out/'subtitle-titles/report.json').read_text())
assert titles['passed'] and titles['source_unchanged']
assert {(r['case'],r['mode']) for r in titles['records']}=={(c,m) for c in ('title_rows','android_names','upstream_425') for m in ('normal','asan')}
for rel,sha in titles['source_sha256'].items():
 assert hashlib.sha256(Path(rel).read_bytes()).hexdigest()==sha,rel
for record in titles['records']:
 assert record['passed'] and record['compile']['exit_code']==0
 assert record['runs'] and all(r['exit_code']==0 for r in record['runs'])
 assert ('-fsanitize=address,undefined' in record['compile']['command'])==(record['mode']=='asan')
visuals=[]
if not case['case']['delivery']:
 for mode in ('tv','touch'):
  visual=json.loads((out/('subtitle-visual-'+mode)/'visual-report.json').read_text())
  assert visual['passed'] and visual['source_unchanged']
  assert len(visual['captures'])==3
  assert visual['touch']==(mode=='touch')
  visuals.append(visual)
records=[json.loads(p.read_text()) for p in (out.parent/'compile').glob('*-invocation.json')]
assert records
for record in records:
 assert record['returncode']==0
 assert record['effective_arguments']==record['arguments']+([record['added_link_rpath']] if record['added_link_rpath'] else [])
proof={'outcome':'passed','source_commit':source['source_commit'],'review_commit':source['build_commit'],
       'source_scope':source['scope'],'case':source['case'],'gates':required,
       'full_app_compile_modes':['TV support off','Mobile support on'],
       'original_test_bytes':originals,'sanitized_original_replays':replays,
       'subtitle_fixture_records':titles['records'],'subtitle_source_sha256':titles['source_sha256'],
       'subtitle_visual_captures':visuals,
       'compiler_invocations_verified':len(records),'same_product_source_reverification_required':True,
       'only_added_compiler_fix':'real SDL3 link-time rpath'}
if case['case']['delivery']:
 logproof=json.loads((out/'original-log-source.json').read_text())
 assert logproof['verified'] and logproof['original_reporting_preserved'] and logproof['original_live_viewer_preserved'] and logproof['download_test_only']
 proof['original_log_behavior']=logproof
(out/'native-gates-verified.json').write_text(json.dumps(proof,indent=2)+'\n')
print('Every focused native gate passed:',source['case'],len(required))
