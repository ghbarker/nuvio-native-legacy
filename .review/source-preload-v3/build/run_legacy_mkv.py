#!/usr/bin/env python3
"""Retain the original pool test; guard its empty Bash 3.2 array only."""
import argparse
import hashlib
from pathlib import Path
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('--source', required=True)
args = parser.parse_args()
script = Path(args.source).resolve() / 'tests/mkvass_pool_obsoleto.sh'
original = script.read_text()
old = '"${flags[@]}"'
new = '${flags[@]+"${flags[@]}"}'
assert original.count(old) == 1
adapted = original.replace(old, new)
assert adapted.replace(new, old) == original
print('Original pool script SHA256:', hashlib.sha256(script.read_bytes()).hexdigest(), flush=True)
print('Review-only adaptation: one empty array guard; C source arguments and assertions unchanged', flush=True)
# $0 remains the original path, retaining the script's exact cd/relative paths.
raise SystemExit(subprocess.run(['bash', '-c', adapted, str(script)]).returncode)
