#!/usr/bin/env python3
"""Record original flags and add the real SDL3 lookup directory only at link."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time

arguments = sys.argv[1:]
root = Path(os.environ['REVIEW_CC_OUTPUT'])
root.mkdir(parents=True, exist_ok=True)
index = str(time.time_ns())
effective_arguments = list(arguments)
link_rpath = None
if '-c' not in arguments:
    directory = Path(os.environ['REVIEW_SDL3_LIBRARY_DIR']).resolve()
    if not directory.is_dir():
        raise SystemExit('Real Homebrew SDL3 library directory is missing')
    link_rpath = '-Wl,-rpath,' + str(directory)
    effective_arguments.append(link_rpath)
record = {'compiler': os.environ['REVIEW_REAL_CC'], 'arguments': arguments,
          'effective_arguments': effective_arguments, 'added_link_rpath': link_rpath,
          'sanitizers': '-fsanitize=address,undefined' in arguments,
          'frame_pointers': '-fno-omit-frame-pointer' in arguments}
(root / (index + '-invocation.json')).write_text(json.dumps(record, indent=2) + '\n')
if not record['sanitizers'] or not record['frame_pointers']:
    raise SystemExit('Expected original address + undefined sanitizers and frame pointers')
result = subprocess.run([record['compiler']] + effective_arguments)
if result.returncode == 0 and '-c' not in arguments and '-o' in arguments:
    binary = Path(arguments[arguments.index('-o') + 1]).resolve()
    copy = root / ('linked-' + binary.name)
    shutil.copy2(binary, copy)
    record.update({'linked_binary': str(binary), 'saved_binary': str(copy),
                   'sha256': hashlib.sha256(copy.read_bytes()).hexdigest()})
    for label, command in [('libraries', ['otool', '-L', str(binary)]),
                           ('load-commands', ['otool', '-l', str(binary)]),
                           ('symbols', ['nm', '-m', str(binary)])]:
        try:
            evidence = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                      text=True, timeout=10)
            contents = evidence.stdout
            if label == 'symbols':
                contents = '\n'.join(line for line in contents.splitlines()
                                     if any(name in line for name in ('SDL', 'asan', 'dlopen', 'dllinit', 'error_dialog')))
            (root / (index + '-' + label + '.txt')).write_text(contents + '\n')
        except (OSError, subprocess.TimeoutExpired) as exc:
            (root / (index + '-' + label + '.txt')).write_text(str(exc) + '\n')
    (root / (index + '-invocation.json')).write_text(json.dumps(record, indent=2) + '\n')
raise SystemExit(result.returncode)
