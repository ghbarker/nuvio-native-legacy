#!/usr/bin/env python3
import argparse
import os
from pathlib import Path
import platform
import subprocess

parser=argparse.ArgumentParser()
parser.add_argument('--root',required=True,type=Path)
parser.add_argument('--output',required=True,type=Path)
parser.add_argument('--sdk',type=Path)
args=parser.parse_args()
fixtures=Path(__file__).resolve().parent
source=args.root.resolve()
output=args.output.resolve()
output.mkdir(parents=True,exist_ok=True)
sdk=args.sdk or fixtures.parent/'controller/sdk'
if not sdk.exists():
    sdk=fixtures.parent/'next/sdk'
flags=['-std=gnu11','-O1','-g','-ffunction-sections','-fdata-sections',
       '-Werror=implicit-function-declaration','-I'+str(sdk),'-I'+str(source/'src'),'-pthread']
if os.environ.get('SANITIZE','0')=='1':
    flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    if platform.system()=='Linux':
        flags+=['-fno-pie','-no-pie']
link=['-Wl,-dead_strip' if platform.system()=='Darwin' else '-Wl,--gc-sections','-lm']
cases=[('next_preload_controller',['fontefalha.c']),
       ('next_preload_posplay',['proximo.c']),
       ('next_ready_posplay_joined',['fontefalha.c','proximo.c']),
       ('next_ready_source_only',['fontefalha.c'])]
for name,modules in cases:
    binary=output/name
    command=[os.environ.get('CC','cc')]+flags+[str(fixtures/'tests'/(name+'.c'))]
    command += [str(source/'src'/module) for module in modules]+link+['-o',str(binary)]
    build=subprocess.run(command,capture_output=True,text=True)
    (output/(name+'.build.log')).write_text(build.stdout+build.stderr)
    if build.returncode:
        print(build.stdout+build.stderr)
        raise SystemExit(build.returncode)
    env=os.environ.copy()
    env.setdefault('ASAN_OPTIONS','detect_leaks=0:halt_on_error=1')
    env.setdefault('UBSAN_OPTIONS','halt_on_error=1:print_stacktrace=1')
    checked=subprocess.run([str(binary)],capture_output=True,text=True,env=env,timeout=25)
    (output/(name+'.run.log')).write_text(checked.stdout+checked.stderr)
    print(checked.stdout+checked.stderr,end='',flush=True)
    if checked.returncode:
        raise SystemExit(checked.returncode)
