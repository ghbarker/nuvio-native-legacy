#!/usr/bin/env python3
"""Compile all application C sources in TV and Mobile modes on the exact PR tree."""
import argparse
from concurrent.futures import ThreadPoolExecutor
import json
import os
from pathlib import Path
import subprocess
from verify_source import plan, verify

p = argparse.ArgumentParser()
p.add_argument('--case', required=True)
p.add_argument('--output', type=Path, required=True)
a = p.parse_args()
root = Path.cwd().resolve()
value, case = plan(a.case)
proof = verify(root, value, case)
out = a.output.resolve()
out.mkdir(parents=True, exist_ok=True)
modules = sorted(str(p) for p in Path('src').glob('*.c')) + sorted(str(p) for p in Path('src/dts').glob('*.c'))
flags = ['-O1', '-g', '-DNV_SUPABASE_URL=""', '-DNV_SUPABASE_ANON_KEY=""', '-DNV_TV_LOGIN_BASE=""',
         '-DNV_TRAKT_CLIENT_ID=""', '-DNV_TRAKT_CLIENT_SECRET=""', '-DNV_SIMKL_CLIENT_ID=""',
         '-DNV_SIMKL_APP=""', '-DNV_TMDB_API_KEY=""', '-I/opt/homebrew/include',
         '-I/opt/homebrew/include/SDL2', '-L/opt/homebrew/lib', '-lSDL2', '-lSDL2_image',
         '-lSDL2_ttf', '-lz', '-framework', 'OpenGL', '-Wno-deprecated-declarations']
def build(mode):
    command = ['cc', *modules, *(['-DNV_TOUCH_UI=1'] if mode == 'mobile' else ['-UNV_TOUCH_UI']),
               '-o', str(out / ('app-' + mode)), *flags]
    with (out / (mode + '.log')).open('w') as log:
        result = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, timeout=900)
    if result.returncode:
        print('\n'.join((out / (mode + '.log')).read_text(errors='replace').splitlines()[-70:]), flush=True)
        raise RuntimeError(mode + ': build failed')
    return {'mode': mode, 'status': 'passed', 'source_files': modules, 'command': command,
            'binary_sha256': __import__('hashlib').sha256((out / ('app-' + mode)).read_bytes()).hexdigest()}
with ThreadPoolExecutor(max_workers=2) as pool:
    runs = list(pool.map(build, ('tv', 'mobile')))
with (out / 'translations.log').open('w') as log:
    subprocess.run([__import__('sys').executable, 'tools/idiomas.py'], stdout=log, stderr=subprocess.STDOUT, check=True)
verify(root, value, case)
proof.update({'outcome': 'passed', 'gates': runs, 'parallel_independent_builds': True, 'all_translation_tables_and_font_glyphs': 'passed'})
(out / 'verified.json').write_text(json.dumps(proof, indent=2) + '\n')
print('Both full application builds passed:', a.case, flush=True)
