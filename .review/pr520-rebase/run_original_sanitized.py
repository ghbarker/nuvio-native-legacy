#!/usr/bin/env python3
"""Replay an unchanged original script with only additive compiler sanitizers.

Some originals do not read SANITIZE. Record both original and replay bytes so
the replay cannot be mistaken for support built into the original script.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess

p = argparse.ArgumentParser()
p.add_argument('--name', required=True)
p.add_argument('--output', type=Path, required=True)
a = p.parse_args()
root = Path.cwd().resolve()
original = root / 'tests' / (a.name + '.sh')
data = original.read_text()
workdir = 'cd "$(dirname "$0")/.."'
assert data.count(workdir) == 1
assert data.count('\ncc ') == 1
flags = '-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all'
# Do not replace any source list, flags, generated fixture or assertions.
replay = data.replace(workdir, 'cd "$REVIEW_PRODUCT_ROOT"', 1)
replay = replay.replace('\ncc ', '\ncc ' + flags + ' ', 1)
a.output.mkdir(parents=True, exist_ok=True)
script = a.output / (a.name + '-sanitized.sh')
script.write_text(replay)
env = dict(os.environ, REVIEW_PRODUCT_ROOT=str(root))
result = subprocess.run(['bash', str(script)], env=env, text=True)
proof = {'original_script': str(original.relative_to(root)),
         'original_sha256': hashlib.sha256(data.encode()).hexdigest(),
         'replay_sha256': hashlib.sha256(replay.encode()).hexdigest(),
         'source_root': str(root), 'sanitizer_flags': flags.split(),
         'only_changes': ['external script working-directory binding', 'additive compiler sanitizer flags'],
         'original_source_arguments_and_assertions_unchanged': True,
         'exit_code': result.returncode}
(a.output / (a.name + '-sanitized.json')).write_text(json.dumps(proof, indent=2) + '\n')
assert original.read_text() == data
raise SystemExit(result.returncode)
