#!/usr/bin/env python3
"""Preserve each review test's compiler flags; supply SDL3's link-time rpath."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import time


arguments = sys.argv[1:]
effective = list(arguments)
record_root = Path(os.environ['REVIEW_CC_OUTPUT'])
record_root.mkdir(parents=True, exist_ok=True)
index = str(time.time_ns()) + '-' + str(os.getpid())
link_rpath = None
compile_only = any(flag in arguments for flag in ('-c', '-E', '-S', '-fsyntax-only', '-M', '-MM'))
driver_query = any(flag in arguments for flag in ('--version', '-v', '-dumpmachine', '-dumpversion', '-print-search-dirs')) and not any(
    Path(argument).suffix in ('.c', '.cc', '.cpp', '.m', '.mm', '.o', '.a') for argument in arguments)
if arguments and not compile_only and not driver_query:
    directory = Path(os.environ['REVIEW_SDL3_LIBRARY_DIR']).resolve()
    if not directory.is_dir():
        raise SystemExit('Real Homebrew SDL3 library directory is missing')
    link_rpath = '-Wl,-rpath,' + str(directory)
    effective.append(link_rpath)
record = {'compiler': os.environ['REVIEW_REAL_CC'], 'cwd': str(Path.cwd()), 'arguments': arguments,
          'effective_arguments': effective, 'added_link_rpath': link_rpath,
          'compile_only': compile_only, 'driver_query': driver_query}
record_path = record_root / (index + '-invocation.json')
record_path.write_text(json.dumps(record, indent=2) + '\n')
result = subprocess.run([record['compiler']] + effective)
record['returncode'] = result.returncode
if result.returncode == 0 and link_rpath is not None and '-o' in arguments:
    binary = Path(arguments[arguments.index('-o') + 1]).resolve()
    if binary.is_file():
        record['linked_binary'] = str(binary)
        record['sha256'] = hashlib.sha256(binary.read_bytes()).hexdigest()
        record['bytes'] = binary.stat().st_size
        commands = subprocess.run(['otool', '-l', str(binary)], text=True,
                                  stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=15)
        (record_root / (index + '-load-commands.txt')).write_text(commands.stdout)
        record['load_commands_returncode'] = commands.returncode
        libraries = subprocess.run(['otool', '-L', str(binary)], text=True,
                                   stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=15)
        (record_root / (index + '-linked-libraries.txt')).write_text(libraries.stdout)
        record['linked_libraries_returncode'] = libraries.returncode
record_path.write_text(json.dumps(record, indent=2) + '\n')
raise SystemExit(result.returncode)
