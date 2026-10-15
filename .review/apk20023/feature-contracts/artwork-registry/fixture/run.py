#!/usr/bin/env python3
"""Actual registry plus unchanged original cases; no network/media/GPU work."""
import argparse, hashlib, json, os, platform, re, shlex, subprocess
from pathlib import Path

p = argparse.ArgumentParser()
p.add_argument('--source', required=True, type=Path)
p.add_argument('--output', required=True, type=Path)
p.add_argument('--cc', default='/usr/bin/cc')
p.add_argument('--sanitize', action='store_true')
p.add_argument('--private', action='store_true')
a = p.parse_args()
s = a.source.resolve(); out = a.output.resolve(); f = Path(__file__).resolve().parent
assert not out.exists(), 'fresh output required'
out.mkdir(parents=True)
sha = lambda path: hashlib.sha256(Path(path).read_bytes()).hexdigest()
git = lambda *args: subprocess.check_output(['git', *args], cwd=s, text=True).strip()
initial = {str(q): sha(q) for root in [s/'src', s/'tests/fixtures/onepiece', f]
           for q in root.rglob('*') if q.is_file() and '__pycache__' not in q.parts}
assert sha(f/'original_artereserva.c') == sha(s/'tests/artereserva.c')
flags = ['-std=gnu11', '-O1', '-g', '-Wall', '-Wextra', '-Wno-deprecated-declarations',
         '-I'+str(s/'src'), '-I'+str(f/'sdk')]
if a.private: flags += ['-DNV_TEST_LOCAL_LOGS=1']
if a.sanitize:
    flags += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-fno-sanitize-recover=all']
    if platform.system() == 'Linux': flags += ['-fno-pie']
modules = [s/'src'/n for n in ['artereserva.c','artefontes.c','js.c','redeurl.c']]
modules += [f/'registry_long_url.c']
commands = []; objects = []; dependencies = {}
for i, module in enumerate(modules):
    obj = out/(str(i)+'.o'); dep = out/(str(i)+'.d')
    cmd = [a.cc, *flags, '-MMD', '-MF', str(dep), '-c', str(module), '-o', str(obj)]
    commands.append(cmd)
    z = subprocess.run(cmd, cwd=s, capture_output=True, text=True, timeout=120)
    (out/(str(i)+'.compile.log')).write_text(z.stdout+z.stderr)
    assert z.returncode == 0, z.stderr
    objects.append(str(obj))
    for raw in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
        q = Path(raw).resolve()
        assert str(q) in initial and initial[str(q)] == sha(q), str(q)
        dependencies[str(q)] = sha(q)
binary = out/'registry'
cmd = [a.cc, *flags, *objects, '-pthread', '-lm', '-o', str(binary)]
if a.sanitize and platform.system() == 'Linux': cmd += ['-no-pie']
commands.append(cmd)
z = subprocess.run(cmd, cwd=s, capture_output=True, text=True, timeout=120)
(out/'link.log').write_text(z.stdout+z.stderr)
assert z.returncode == 0, z.stderr
env = dict(os.environ, ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
           UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
cases = []
for mode in ['original','long','arena']:
    cmd = [str(binary), mode]; commands.append(cmd)
    z = subprocess.run(cmd, cwd=s, env=env, capture_output=True, text=True, timeout=120)
    (out/(mode+'.run.log')).write_text(z.stdout+z.stderr)
    cases.append({'mode':mode, 'returncode':z.returncode,
                  'fixture_failures':re.findall(r'^FALHOU: (.*)$',z.stdout,re.M),
                  'sanitizer_error':bool(re.search(r'ERROR: AddressSanitizer|runtime error:|LeakSanitizer|DEADLYSIGNAL',z.stdout+z.stderr)),
                  'summary':[v for v in z.stdout.splitlines() if v.startswith(('artereserva:','long-url checks=','arena checks='))],
                  'log_sha256':sha(out/(mode+'.run.log'))})
nm = subprocess.check_output(['nm','-S',str(binary)], text=True)
symbols = {}
for row in nm.splitlines():
    fields = row.split()
    if len(fields) >= 4 and fields[-1] in ['regTexto','reg']:
        symbols[fields[-1]] = int(fields[1],16)
assert symbols['regTexto'] == 2*1024*1024
unchanged = all(sha(q) == h for q,h in initial.items())
assert unchanged
receipt = {'source_commit':git('rev-parse','HEAD'), 'tree':git('rev-parse','HEAD^{tree}'),
           'source_status':git('status','--porcelain'), 'private':a.private, 'sanitizers':['address','undefined'] if a.sanitize else [],
           'passed':all(c['returncode']==0 and not c['sanitizer_error'] for c in cases),
           'cases':cases, 'unchanged_original_fixture_sha256':sha(f/'original_artereserva.c'),
           'production_modules':[str(q.relative_to(s)) for q in modules if q.is_relative_to(s)],
           'dependency_sha256':dependencies, 'fixture_sha256':{str(q.relative_to(f)):sha(q) for q in f.rglob('*') if q.is_file() and '__pycache__' not in q.parts},
           'symbol_bytes':symbols, 'source_inputs_unchanged':unchanged,
           'compiler_version':subprocess.check_output([a.cc,'--version'],text=True).splitlines()[0],
           'commands':commands, 'logs_sha256':{str(q.relative_to(out)):sha(q) for q in out.glob('*.log')}}
(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(json.dumps({k:receipt[k] for k in ['passed','source_commit','source_status','sanitizers','cases','symbol_bytes']}))
