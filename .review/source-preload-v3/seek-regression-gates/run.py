#!/usr/bin/env python3
"""Review-only actual-C seek regressions; never injects test code into product.

report.json: runs[] records mode, case, source_root, source_hashes, exact
build_command/sanitizer_flags, build_exit_code, exit_code, expected_exit_code,
and passed. The top-level source_hashes must still match after every run.
The optional --expect-jitter-bug uses the identical actual-player fixture on
the unchanged Logs3 tree, requiring its functional failure return code 42.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import subprocess
import sys

parser=argparse.ArgumentParser()
parser.add_argument('--root',required=True,type=Path)
parser.add_argument('--output',required=True,type=Path)
parser.add_argument('--mode',choices=['normal','asan','both'],default='both')
parser.add_argument('--helpers',type=Path,help='Sibling reviewed controller/next-ready helper directory')
parser.add_argument('--expect-jitter-bug',action='store_true')
args=parser.parse_args()
base=Path(__file__).resolve().parent
fixtures=base/'tests'
helpers=(args.helpers or base.parent).resolve()
root=args.root.resolve()
output=args.output.resolve()
output.mkdir(parents=True,exist_ok=True)
paths=['src/player.c','src/player.h','src/posplay.c','src/posplay.h',
       'src/creditosauto.h','src/proximo.c','src/proximo.h',
       'src/proximopreparo.c','src/proximopreparo.h',
       'src/fontefalha.c','src/velocidade.c','src/relogio.c']
def hashes():
    return {path:hashlib.sha256((root/path).read_bytes()).hexdigest() for path in paths}
source_hashes=hashes()
host=platform.system()
flags=['-std=gnu11','-O1','-g','-pthread','-DNV_ANDROID',
       '-Werror=implicit-function-declaration','-ffunction-sections','-fdata-sections',
       '-I'+str(helpers/'controller/sdk'),'-I'+str(root/'src'),
       '-I'+str(helpers/'next-ready/tests'),'-include',str(fixtures/'sdk-extra.h')]
link=['-Wl,-dead_strip' if host=='Darwin' else '-Wl,--gc-sections','-lm']
cases=[('seek_gestures',['tests/seek_gestures.c','tests/posplay_probe.c'],
        ['proximo.c','velocidade.c','relogio.c']),
       ('seek_controller',['tests/seek_controller.c'],['proximo.c','fontefalha.c'])]
if not args.expect_jitter_bug:
    cases.append(('window_reentry',['tests/window_reentry.c'],['proximo.c']))
if args.expect_jitter_bug:
    cases=cases[:1]
report={'schema':1,'host':host,'source_root':str(root),
        'source_hashes':source_hashes,'baseline_reproduction':args.expect_jitter_bug,'runs':[]}
def save():
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
save()
for mode in (['normal','asan'] if args.mode=='both' else [args.mode]):
    san=[]
    if mode=='asan':
        san=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-sanitize-recover=all']
        if host=='Linux':san+=['-fno-pie','-no-pie']
    for case,testfiles,modules in cases:
        label=case+'-'+mode
        binary=output/label
        command=[os.environ.get('CC','cc')]+flags+san
        command += [str(base/file) for file in testfiles]
        command += [str(root/'src'/file) for file in modules]+link+['-o',str(binary)]
        run={'mode':mode,'case':case,'source_root':str(root),
             'source_hashes':source_hashes,'sanitizer_flags':san,
             'build_command':command,'expected_exit_code':42 if args.expect_jitter_bug else 0,
             'passed':False}
        report['runs'].append(run)
        try:
            build=subprocess.run(command,capture_output=True,text=True,timeout=60)
            run['build_exit_code']=build.returncode
            (output/(label+'.build.log')).write_text(build.stdout+build.stderr)
            if build.returncode:
                print(build.stdout+build.stderr,file=sys.stderr)
                raise RuntimeError(f'{label}: compilation failed')
            env=os.environ.copy()
            env.setdefault('ASAN_OPTIONS','detect_leaks=0:halt_on_error=1')
            env.setdefault('UBSAN_OPTIONS','halt_on_error=1:print_stacktrace=1')
            invocation=[str(binary)]+(['--jitter-only'] if args.expect_jitter_bug else [])
            run['run_command']=invocation
            checked=subprocess.run(invocation,capture_output=True,text=True,env=env,timeout=25)
            run['exit_code']=checked.returncode
            (output/(label+'.run.log')).write_text(checked.stdout+checked.stderr)
            print(checked.stdout+checked.stderr,end='',flush=True)
            if checked.returncode!=run['expected_exit_code']:
                raise RuntimeError(f'{label}: got {checked.returncode}, expected {run["expected_exit_code"]}')
            if hashes()!=source_hashes:
                raise RuntimeError('Product sources changed during the regression run')
            run['passed']=True
            print(f'{label}: PASS',flush=True)
        except Exception as exc:
            run['error']=str(exc)
            save()
            raise
        save()
report['passed']=all(run['passed'] for run in report['runs'])
save()
