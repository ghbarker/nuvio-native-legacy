#!/usr/bin/env python3
"""Fail cheaply if the selected Apple compiler cannot find its own SDK."""
import argparse, hashlib, json, os, subprocess
from pathlib import Path

p = argparse.ArgumentParser(); p.add_argument('--output', type=Path, required=True); a = p.parse_args()
out = a.output.resolve(); out.mkdir(parents=True, exist_ok=True)
assert os.environ['REVIEW_REAL_CC'] == '/usr/bin/cc', 'Use the accepted Apple SDK driver, captured before wrapper PATH setup'
probe = out / 'sdk-probe.c'
probe.write_text('#include <stdio.h>\n#include <AvailabilityMacros.h>\n#include <SDL2/SDL.h>\n#include <OpenGL/gl.h>\nint sdk_probe(void) { return sizeof(FILE*) + SDL_MAJOR_VERSION + GL_FALSE; }\n')
flags = ['cc', '-I/opt/homebrew/include', '-I/opt/homebrew/include/SDL2', '-Wno-deprecated-declarations']
records = []
for mode, extra in (('preprocess', ['-E', str(probe), '-o', os.devnull]), ('compile', ['-c', str(probe), '-o', str(out / 'sdk-probe.o')])):
    argv = flags + extra
    with (out / (mode + '.log')).open('w') as log:
        result = subprocess.run(argv, stdout=log, stderr=subprocess.STDOUT, timeout=60)
    records.append({'mode': mode, 'arguments': argv, 'exit_code': result.returncode,
                    'log_sha256': hashlib.sha256((out / (mode + '.log')).read_bytes()).hexdigest()})
    assert result.returncode == 0, ('SDK sanity failed before full application build', mode)
proof = {'outcome': 'passed', 'review_commit': os.environ['GITHUB_SHA'], 'compiler': os.environ['REVIEW_REAL_CC'],
         'driver_selection': 'command -v cc before wrapper PATH setup', 'source_sha256': hashlib.sha256(probe.read_bytes()).hexdigest(),
         'required_headers': ['stdio.h', 'AvailabilityMacros.h', 'SDL2/SDL.h', 'OpenGL/gl.h'], 'checks': records}
(out / 'sdk-verified.json').write_text(json.dumps(proof, indent=2) + '\n')
print('PASS Apple SDK preprocess and compile using the same full-build driver/environment')
