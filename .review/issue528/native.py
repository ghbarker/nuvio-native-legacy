"""Run the exact checked-in cateps suite against the complete included discovery source."""
from pathlib import Path
import argparse, hashlib, json, os, platform, re, signal, subprocess, sys, time
from common import HERE, sha, source_binding

p = argparse.ArgumentParser()
for key in ['source', 'proof', 'output']:
    p.add_argument('--' + key, type=Path, required=True)
a = p.parse_args()
s = a.source.resolve()
o = a.output.resolve()
o.mkdir(parents=True, exist_ok=False)
plan, proof, manifest = source_binding(s, a.proof)
assert platform.system() == 'Darwin'
cc = Path('/usr/bin/cc')
assert cc.is_file()
assert Path('/opt/homebrew/include/SDL2/SDL.h').is_file(), 'Exact upstream script requires its real Homebrew include path'
sdl3 = Path(subprocess.check_output(['brew', '--prefix', 'sdl3'], text=True).strip()) / 'lib'
assert sdl3.is_dir()
wrapper = HERE / 'cc_rpath.py'
assert sha(wrapper) == 'bdcf2d60839e9445da53f70778aac662601af031745ca28b2dc3da2ed7c012b9'
test_text = (s / 'tests/cateps.c').read_text()
markers = re.findall(r'puts\("(ok  [^"\n]*)"\);', test_text)
assert len(markers) >= 17 and sum('#528' in x for x in markers) == 8
assert any('worker publica retrato antes do TMDB' in x for x in markers)
expected_units = {'src/catalogo.c', 'src/cwordem.c', 'tests/cateps.c', 'src/cotacat.c',
                  'src/js.c', 'src/metaprov.c', 'src/colecoes.c', 'src/redeurl.c', 'src/catordem.c'}
assert '#include "../src/descoberta.c"' in test_text
rows = []
for mode in ['normal', 'asan']:
    d = o / mode
    d.mkdir()
    compile_out = d / 'compile'
    compile_out.mkdir()
    cc_bin = d / 'cc-bin'
    cc_bin.mkdir()
    executable = cc_bin / 'cc'
    executable.write_bytes(wrapper.read_bytes())
    executable.chmod(0o755)
    env = dict(os.environ, SANITIZE='1' if mode == 'asan' else '0',
               ASAN_OPTIONS='detect_leaks=0:halt_on_error=1',
               UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1',
               REVIEW_REAL_CC=str(cc), REVIEW_SDL3_LIBRARY_DIR=str(sdl3.resolve()),
               REVIEW_CC_OUTPUT=str(compile_out), PATH=str(cc_bin) + os.pathsep + os.environ['PATH'])
    log = d / 'cateps.log'
    started = time.monotonic()
    with log.open('w') as stream:
        child = subprocess.Popen(['/bin/bash', 'tests/cateps.sh'], cwd=s, env=env,
                                 stdout=stream, stderr=subprocess.STDOUT, start_new_session=True)
        try:
            code = child.wait(timeout=600)
        except subprocess.TimeoutExpired:
            os.killpg(child.pid, signal.SIGKILL)
            child.wait()
            (d / 'timeout.json').write_text(json.dumps({'outcome': 'failed', 'timeout_seconds': 600,
                'process_group_killed': True, 'log_sha256': sha(log)}) + '\n')
            raise RuntimeError('Actual cateps ' + mode + ' timed out; this is a failure')
    output = log.read_text(errors='replace')
    if code:
        print('\n'.join(output.splitlines()[-80:]), flush=True)
    assert code == 0, (mode, code)
    assert output.splitlines().count('cateps: tudo ok') == 1
    for marker in markers:
        assert output.splitlines().count(marker) == 1, (mode, marker)
    records = sorted(compile_out.glob('*-invocation.json'))
    assert len(records) == 1
    invocation = json.loads(records[0].read_text())
    args = invocation['arguments']
    assert invocation['returncode'] == 0 and invocation['compiler'] == str(cc)
    assert {x for x in args if x.endswith('.c')} == expected_units
    assert ('-fsanitize=address,undefined' in args) == (mode == 'asan')
    assert '-fno-omit-frame-pointer' in args if mode == 'asan' else True
    added = '-Wl,-rpath,' + str(sdl3.resolve())
    assert invocation['added_link_rpath'] == added
    assert invocation['effective_arguments'] == args + [added]
    binary = Path('/tmp/nuvio-cateps-tests')
    assert binary.is_file() and sha(binary) == invocation['sha256']
    dependencies = subprocess.check_output(['otool', '-L', str(binary)], text=True)
    commands = subprocess.check_output(['otool', '-l', str(binary)], text=True)
    (d / 'linked-libraries.log').write_text(dependencies)
    (d / 'load-commands.log').write_text(commands)
    assert str(sdl3.resolve()) in commands
    assert ('libclang_rt.asan' in dependencies) == (mode == 'asan')
    if mode == 'asan':
        assert not re.search(r'ERROR: AddressSanitizer|runtime error:|SUMMARY: (AddressSanitizer|UndefinedBehaviorSanitizer)', output)
    row = {'mode': mode, 'returncode': code, 'timed_out': False,
           'duration_seconds': round(time.monotonic() - started, 3),
           'source_binding': proof, 'assertion_markers': markers, 'marker_count': len(markers),
           'worker_integration_executed': True, 'original_test_script_unchanged': True,
           'binary_sha256': sha(binary), 'binary_bytes': binary.stat().st_size,
           'log_sha256': sha(log), 'compile_record_sha256': sha(records[0]),
           'linked_libraries_sha256': sha(d / 'linked-libraries.log'),
           'load_commands_sha256': sha(d / 'load-commands.log'),
           'sdl_linked': bool(re.search(r'libSDL[23]', dependencies)),
           'sanitizers': ['address', 'undefined'] if mode == 'asan' else [],
           'darwin_leak_detection': False, 'only_added_flag': added}
    (d / 'receipt.json').write_text(json.dumps(row, indent=2) + '\n')
    rows.append(row)
source_binding(s, a.proof)
result = {'outcome': 'passed', 'source_binding': proof, 'plan_sha256': sha(HERE / 'plan.json'),
          'suite': 'tests/cateps.sh', 'test_script_sha256': sha(s / 'tests/cateps.sh'),
          'test_source_sha256': sha(s / 'tests/cateps.c'), 'modes': rows,
          'compiler': subprocess.check_output([str(cc), '--version'], text=True).splitlines()[0],
          'scope': 'Exact checked-in complete discovery/catalog cateps suite, including actual cached-metadata worker publication and optional TMDB failure. No full app build or extracted helper substitute.'}
(o / 'native-verified.json').write_text(json.dumps(result, indent=2) + '\n')
print('PASS exact cateps normal + ASan/UBSan and actual worker regression', flush=True)
