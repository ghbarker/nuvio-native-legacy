"""Run the actual committed public test scripts without rewriting their sources."""
from pathlib import Path
import argparse, json, os, platform, re, signal, subprocess, sys, time
from common import HERE, sha, source_binding

p = argparse.ArgumentParser()
for key in ['source', 'proof', 'output']:
    p.add_argument('--' + key, type=Path, required=True)
a = p.parse_args()
s = a.source.resolve()
o = a.output.resolve()
o.mkdir(parents=True, exist_ok=False)
plan, binding, manifest = source_binding(s, a.proof)
assert platform.system() == 'Darwin'
cc = Path('/usr/bin/cc')
sdl3 = Path(subprocess.check_output(['brew', '--prefix', 'sdl3'], text=True).strip()) / 'lib'
assert cc.is_file() and sdl3.is_dir()
wrapper = HERE / 'cc_rpath.py'
groups = []
for suite in manifest['public_suites']:
    script = s / suite['script']
    assert script.is_file() and suite['script'].startswith('tests/')
    assert sha(script) == suite['script_sha256']
    for source, digest in suite['inputs'].items():
        assert source.startswith(('tests/', 'src/')) and sha(s / source) == digest
    for variant in suite.get('variants', [{'name': 'default', 'env': {}}]):
        rules = {**suite, **variant}
        for mode in suite.get('modes', ['normal', 'asan']):
            name = suite['name'] + '-' + variant['name'] + '-' + mode
            d = o / name
            d.mkdir()
            captures = d / 'compiler'
            captures.mkdir()
            cc_bin = d / 'cc-bin'
            cc_bin.mkdir()
            for executable in ['cc', 'clang', 'gcc']:
                target = cc_bin / executable
                target.write_bytes(wrapper.read_bytes())
                target.chmod(0o755)
            env = dict(os.environ, **variant.get('env', {}))
            env.update(SANITIZE='1' if mode == 'asan' else '0', CC=str(cc_bin / 'cc'),
                       ASAN_OPTIONS='detect_leaks=0:halt_on_error=1',
                       UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1',
                       REVIEW_REAL_CC=str(cc), REVIEW_SDL3_LIBRARY_DIR=str(sdl3.resolve()),
                       REVIEW_CC_OUTPUT=str(captures),
                       PATH=str(cc_bin) + os.pathsep + os.environ['PATH'])
            command = ['/bin/bash', suite['script']]
            log = d / 'test.log'
            started = time.monotonic()
            with log.open('w') as stream:
                child = subprocess.Popen(command, cwd=s, env=env, stdout=stream,
                                         stderr=subprocess.STDOUT, start_new_session=True)
                try:
                    code = child.wait(timeout=suite.get('timeout_seconds', 900))
                except subprocess.TimeoutExpired:
                    os.killpg(child.pid, signal.SIGKILL)
                    child.wait()
                    raise RuntimeError('Actual public test timed out: ' + name)
            output = log.read_text(errors='replace')
            if code:
                print('\n'.join(output.splitlines()[-80:]), flush=True)
            assert code == 0, (name, code)
            assert not re.search(r'ERROR: AddressSanitizer|runtime error:|SUMMARY: (AddressSanitizer|UndefinedBehaviorSanitizer)', output)
            for marker in rules.get('required_markers', []):
                assert output.splitlines().count(marker) == 1, (name, marker)
            for marker, count in rules.get('marker_counts', {}).items():
                assert output.splitlines().count(marker) == count, (name, marker, count)
            if 'ok_marker_count' in rules:
                assert sum(line.startswith('ok  ') for line in output.splitlines()) == rules['ok_marker_count'], name
            if 'line_count' in rules:
                rule = rules['line_count']
                assert sum(line.startswith(rule['prefix']) for line in output.splitlines()) == rule['count'], name
            records = []
            for path in sorted(captures.glob('*-invocation.json')):
                record = json.loads(path.read_text())
                assert record['compiler'] == str(cc) and record['returncode'] == 0
                assert record['cwd'] == str(s)
                assert record['effective_arguments'] == record['arguments'] + ([record['added_link_rpath']] if record['added_link_rpath'] else [])
                records.append({'path': str(path.relative_to(o)), 'sha256': sha(path), 'record': record})
            compiled = [row['record'] for row in records if any(Path(x).suffix == '.c' for x in row['record']['arguments'])]
            linked = [row['record'] for row in records if row['record'].get('linked_binary')]
            assert compiled and linked, 'Actual compiler/link invocations are required'
            if 'compile_invocations' in rules: assert len(compiled)==rules['compile_invocations'], (name, 'actual compiler count', len(compiled))
            if 'link_invocations' in rules: assert len(linked)==rules['link_invocations'], (name, 'actual link count', len(linked))
            for record in compiled:
                sanitizers = set()
                for flag in record['arguments']:
                    if flag.startswith('-fsanitize='):
                        sanitizers.update(flag.split('=', 1)[1].split(','))
                assert {'address', 'undefined'} <= sanitizers if mode == 'asan' else not sanitizers
                assert '-DNDEBUG' not in record['arguments']
            for record in linked:
                assert record['bytes'] > 0 and re.fullmatch('[0-9a-f]{64}', record['sha256'])
                assert record['load_commands_returncode'] == record['linked_libraries_returncode'] == 0
            groups.append({'name': name, 'suite': suite['script'], 'mode': mode,
                           'variant': variant, 'returncode': code, 'timed_out': False,
                           'seconds': round(time.monotonic() - started, 3),
                           'command': command, 'source_binding': binding,
                           'required_markers': rules.get('required_markers', []),
                           'marker_counts': rules.get('marker_counts', {}),
                           'ok_marker_count': rules.get('ok_marker_count'),
                           'line_count': rules.get('line_count'),
                           'log': str(log.relative_to(o)), 'log_sha256': sha(log),
                           'compiler_records': records, 'linked_binary_count': len(linked),
                           'test_script_sha256': sha(script), 'input_sha256': suite['inputs'],
                           'sanitizers': ['address', 'undefined'] if mode == 'asan' else []})
source_binding(s, a.proof)
raw = {str(path.relative_to(o)): sha(path) for path in sorted(o.rglob('*')) if path.is_file()}
result = {'passed': True, 'outcome': 'passed', 'source_binding': binding,
          'plan_sha256': sha(HERE / 'plan.json'), 'groups': groups, 'group_count': len(groups),
          'raw_files_sha256': raw,
          'scope': 'Exact committed public scripts and C assertions run normal/ASan on actual macOS. Compiler wrapper preserves original arguments and only appends the real SDL3 link rpath; no test rewriting, fixture extraction, stub substitution or relabelled prior pass. Binary digests and Mach-O facts are captured at link time; whole test/app binaries are not downloaded.'}
(o / 'receipt.json').write_text(json.dumps(result, indent=2) + '\n')
print('PASS exact committed public tests:', len(groups), 'fresh mode/variant groups', flush=True)
