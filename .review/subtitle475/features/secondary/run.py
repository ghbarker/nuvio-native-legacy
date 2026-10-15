#!/usr/bin/env python3
"""Replay the approved secondary draw/delay/standalone fixtures on a clean source."""
from pathlib import Path
import argparse, difflib, hashlib, json, os, subprocess, shutil, shlex

parser=argparse.ArgumentParser()
parser.add_argument('--source',type=Path,required=True)
parser.add_argument('--output',type=Path,required=True)
parser.add_argument('--cc',default='cc')
args=parser.parse_args()
s=args.source.resolve(); p=args.output.resolve(); fixture=Path(__file__).resolve().parent
sdk=fixture/'sdk'
cc=shutil.which(args.cc)
assert cc,'Requested compiler is not available'
cc=str(Path(cc).resolve())
sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
def git(*argv):return subprocess.check_output(['git','-C',str(s),*argv],text=True,stderr=subprocess.DEVNULL).strip()
assert not git('status','--porcelain'),'Source must be clean before this replay'
head=git('rev-parse','HEAD'); tree=git('rev-parse','HEAD^{tree}')
files=['render-boundaries.c','settings-companion.c','baseline-legendasui.c','delay-boundaries.c']
approved_fixture_sha256={'render-boundaries.c': '9cf7bb35a3ab52195725dfce1e4cb30810c4f2e8f8165c0554e2a1db73d3d75d', 'settings-companion.c': 'b2430904fb9b5744d3a2aa15cbfb258a1b6d79952d0b4e759d349330741cff9b', 'baseline-legendasui.c': 'f2a2c1adffeeddf0da257b010ef698fa3bba844751a4b4d1ef0fa489df935738', 'delay-boundaries.c': 'e254e0f858bf25e3286939f540f7d3dcf59d132023bfa81eeebffca4a37155a8'}
for name in files:assert sha(fixture/name)==approved_fixture_sha256[name],name
assert sha(s/'tests/legenda2.c')=='1b9caf0d9c13ea6baa01115251c27c8768c49f74c85e5b57e37b00af3a9c82d3'
assert sha(s/'tests/legenda2.sh')=='72f6c23ca20586d4fe7c4acfdd82387df28713753463162a1edeeb7d999331d4'
tracked=[s/'src'/name for name in ['ajustes.c','ajustes.h','legendasui.c','legendasui.h','bidi.c','layout.c','linguas.c','player.c','player.h','legenda2.c','legenda2.h','legenda.c','assrender.c','video.h','plainass.h','app.c','episodios.c','detail.c','main.c']]
tracked+=list((s/'src').glob('ajustes_ux_*.inc'))+[s/'tests/legenda2.c',s/'tests/legenda2.sh']
before={str(path.relative_to(s)):sha(path) for path in tracked}
p.mkdir(parents=True,exist_ok=False); (p/'temporaries').mkdir()
compiler_bin=p/'compiler-bin'; compiler_bin.mkdir()
compiler_wrapper=compiler_bin/'cc'
compiler_wrapper.write_text('#!/bin/sh\nexec '+shlex.quote(cc)+' "$@"\n'); compiler_wrapper.chmod(0o700)
env={**os.environ,'PATH':str(compiler_bin)+os.pathsep+os.environ.get('PATH',''),'TMPDIR':str(p/'temporaries'),'ASAN_OPTIONS':'detect_leaks=1:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1:print_stacktrace=1'}
commands=[]
def run(cmd,log,extra=None,cwd=None):
 record={'command':cmd,'log':log,'cwd':str(cwd) if cwd else None}
 with (p/log).open('w') as out:
  z=subprocess.run(cmd,cwd=cwd,text=True,stdout=out,stderr=subprocess.STDOUT,env={**env,**(extra or {})},timeout=180)
 record.update(returncode=z.returncode,log_sha256=sha(p/log));commands.append(record)
 if z.returncode:print((p/log).read_text(errors='replace')[-6000:],flush=True)
 assert z.returncode==0,(log,z.returncode)
 return (p/log).read_text(errors='replace')
common=['-std=gnu11','-O1','-g','-ffunction-sections','-fdata-sections','-DNV_ANDROID=1','-DNV_TOUCH_UI=1','-I'+str(sdk),'-I'+str(s/'src'),'-DSDL_TICKS_PASSED(A,B)=((Sint32)((B)-(A))<=0)','-DSETTINGS_SOURCE='+json.dumps(str(s/'src/ajustes.c'))]
for sanitize in [False,True]:
 suffix='asan' if sanitize else 'normal'
 sanitizer=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-sanitize-recover=all','-fno-pie'] if sanitize else []
 flags=common+sanitizer
 obj=p/('settings-'+suffix+'.o')
 run([cc,*flags,'-c',str(fixture/'settings-companion.c'),'-o',str(obj)],'settings-'+suffix+'.compile.log')
 for baseline in ([False] if sanitize else [False,True]):
  name='render-baseline' if baseline else 'render-'+suffix
  producer=fixture/'baseline-legendasui.c' if baseline else s/'src/legendasui.c'
  cmd=[cc,*flags,'-DRENDER_SOURCE='+json.dumps(str(producer)),*(['-DBASELINE=1','-include',str(fixture/'baseline-api-compat.h')] if baseline else []),str(fixture/'render-boundaries.c'),str(obj),str(s/'src/bidi.c'),str(s/'src/layout.c'),str(s/'src/linguas.c'),'-Wl,--gc-sections','-lm','-o',str(p/name)]
  if sanitize:cmd+=['-no-pie']
  run(cmd,name+'.compile.log'); output=run([str(p/name)],name+'.log')
  print(name,'PASS',output.splitlines()[-1],flush=True)
 cmd=[cc,'-Isrc','-O1','-g','-Wall','-Wextra',*sanitizer,str(s/'src/legenda2.c'),str(s/'src/legenda.c'),str(s/'src/assrender.c'),str(fixture/'delay-boundaries.c'),'-pthread','-lm','-o',str(p/('delay-'+suffix))]
 if sanitize:cmd+=['-no-pie']
 run(cmd,'delay-'+suffix+'.compile.log',cwd=s)
 output=run([str(p/('delay-'+suffix))],'delay-'+suffix+'.log')
 assert 'secondary persisted delay, AutoSync, reset, map and +/-60s actual cue boundaries: PASS' in output
 print('delay-'+suffix,'PASS',flush=True)
 output=run(['bash','tests/legenda2.sh'],'unbound-original-'+suffix+'.log',{'SANITIZE':'1' if sanitize else '0'},cwd=s)
 assert 'legenda2: states, caps, cancel, stale session/selection, independent offset, primary untouched ok' in output
 print('unbound-original-'+suffix,'PASS',flush=True)
before_traces=[l for l in (p/'render-baseline.log').read_text().splitlines() if l.startswith('LEGACY')]
after_traces=[l for l in (p/'render-normal.log').read_text().splitlines() if l.startswith('LEGACY')]
assert len(before_traces)==16 and before_traces==after_traces
assert head==git('rev-parse','HEAD') and tree==git('rev-parse','HEAD^{tree}') and not git('status','--porcelain')
assert before=={str(path.relative_to(s)):sha(path) for path in tracked},'Actual source changed during replay'
proof={'passed':True,'source_commit':head,'source_tree':tree,'source_clean':True,'source_sha256':before,
 'original_approved_commit':'77051883c3fa4798d032d568975d2aaec618c3ab',
 'fixture_bytes_unchanged':True,'fixture_sha256':{name:sha(fixture/name) for name in files},
 'baseline_only_api_adapter':{'file':'baseline-api-compat.h','sha256':sha(fixture/'baseline-api-compat.h'),'origin':'Exact toquerol_livre body from approved77051883 rolagemtoque.h; comparison renderer only; unrelated input body is not executed'},
 'runner_sha256':sha(Path(__file__)),'compiler':cc,'sdk_sha256':{str(x.relative_to(sdk)):sha(x) for x in sdk.rglob('*') if x.is_file()},'commands':commands,'legacy_trace_cases':16,'legacy_traces_equal':True,
 'independent_position_cases':256,'bound_delay_normal_and_asan':True,'original_unbound_normal_and_asan':True,
 'original_standalone_test_and_script_unchanged':True,'sanitizers':['address','undefined'],
 'baseline_scope':'Frozen approved outline-prefix renderer for the 16 legacy Top/Stacked/OSD/letterbox/island traces; actual candidate renderer and Settings resolver for all new position/style cases.',
 'limits':'Actual Settings resolver and secondary draw/wrap/Bidi code with deterministic typed font metrics/textures and GPU boundaries. Actual subtitle loader/parser/cues execute for delay and standalone suites; local persistence callback store in delay fixture. No physical pixel/font-raster/decoder claim.'}
(p/'secondary-final-binding.json').write_text(json.dumps(proof,indent=2)+'\n')
print('PASS unchanged approved secondary fixtures, actual latest-source replay, normal + ASan/UBSan',flush=True)
