#!/usr/bin/env python3
"""Run the shipping FFmpeg helper with deterministic local build dependencies.

Use BASH32=/path/to/bash-3.2 to check that shell explicitly. On macOS the
default /bin/bash exercises the system Bash 3.2. No downloads or NDK required.
"""
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import zipfile


ROOT = Path(__file__).resolve().parents[1]
SHELL = os.environ.get('BASH32', '/bin/bash')
SOURCE = (ROOT / 'tools/android/ffmpeg-android.sh').read_text()
FFMPEG_PIN = re.search(r'^FFMPEG_PIN=(\w+)$', SOURCE, re.M).group(1)
FAKE_TOOL = r'''#!/usr/bin/env python3
import json, os
from pathlib import Path
import struct, sys
name = Path(sys.argv[0]).name
if name == 'uname':
    print('Darwin')
elif name == 'configure':
    log = Path(os.environ['FIXTURE_ARGV'])
    with log.open('a') as output:
        output.write(json.dumps(sys.argv[1:]) + '\n')
elif name == 'cmake':
    if sys.argv[1] == '--build':
        build = Path(sys.argv[2])
        info = json.loads((build / 'fixture.json').read_text())
        machine = 183 if info['abi'] == 'arm64-v8a' else 62
        data = bytearray(512)
        ident = b'\x7fELF' + bytes((2, 1, 1)) + bytes(9)
        struct.pack_into('<16sHHIQQQIHHHHHH', data, 0,
                         ident, 3, machine, 1, 0, 64, 0, 0, 64, 56, 2, 0, 0, 0)
        struct.pack_into('<IIQQQQQQ', data, 64,
                         1, 5, 0, 0, 0, 512, 16384, 16384)
        struct.pack_into('<IIQQQQQQ', data, 120,
                         0x6474e552, 4, 256, 256, 0, 256, 16128, 1)
        (Path(info['output']) / 'libffmpegJNI.so').write_bytes(data)
    else:
        build = Path(sys.argv[sys.argv.index('-B') + 1])
        build.mkdir(parents=True, exist_ok=True)
        def argument(key):
            return next(x[len(key):] for x in sys.argv if x.startswith(key))
        info = {'abi': argument('-DANDROID_ABI='),
                'output': argument('-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=')}
        Path(info['output']).mkdir(parents=True, exist_ok=True)
        (build / 'fixture.json').write_text(json.dumps(info))
elif name == 'llvm-nm':
    print('00000000 T Java_androidx_media3_decoder_ffmpeg_FfmpegLibrary_ffmpegGetVersion')
    print('00000001 T JNI_OnLoad')
elif name in ('make', 'llvm-strip', 'clang', 'ninja'):
    pass
else:
    raise SystemExit('unexpected external tool: ' + name)
'''


def executable(path, content=FAKE_TOOL):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content)
    path.chmod(0o755)


def run_case(label, abis, expected, source=SOURCE, succeeds=True, failure_text=None):
    with tempfile.TemporaryDirectory(prefix='nuvio-ffmpeg-') as directory:
        root = Path(directory)
        # Spaces catch accidental splitting in path and argument arrays.
        repo = root / 'repository with spaces'
        cache = root / 'cache with spaces'
        sdk = root / 'SDK with spaces'
        ndk = sdk / 'ndk/27.2.12479018'
        ndk.mkdir(parents=True)
        (ndk / 'source.properties').write_text('Pkg.Revision = 27.2.12479018\n')
        toolchain = ndk / 'toolchains/llvm/prebuilt/darwin-x86_64/bin'
        for tool in ('clang', 'llvm-nm', 'llvm-strip'):
            executable(toolchain / tool)
        for tool in ('cmake', 'ninja'):
            executable(sdk / 'cmake/3.22.1/bin' / tool)
        binary = root / 'bin'
        for tool in ('uname', 'make'):
            executable(binary / tool)
        executable(binary / 'curl', '#!/bin/sh\necho unexpected download >&2\nexit 99\n')
        source_directory = cache / 'src'
        source_directory.mkdir(parents=True)
        original = source_directory / 'media3-ffmpeg-decoder-1.8.0+1.aar'
        originals = {'classes.jar': b'published Java classes',
                     'META-INF/LICENSE': b'published license'}
        for abi in ('arm64-v8a', 'armeabi-v7a', 'x86_64', 'x86'):
            originals[f'jni/{abi}/libffmpegJNI.so'] = ('published ' + abi).encode()
        with zipfile.ZipFile(original, 'w') as output:
            for name, content in originals.items():
                output.writestr(name, content)
        # Only the trusted download digest changes for the generated AAR.
        digest = hashlib.sha256(original.read_bytes()).hexdigest()
        script = repo / 'tools/android/ffmpeg-android.sh'
        executable(script, re.sub(r'^AAR_HASH=\w+$', 'AAR_HASH=' + digest, source, flags=re.M))
        checker = repo / 'tools/android-apk-pages.py'
        checker.write_bytes((ROOT / 'tools/android-apk-pages.py').read_bytes())
        ffmpeg = source_directory / ('ffmpeg-' + FFMPEG_PIN)
        executable(ffmpeg / 'configure')
        checksums = b''.join(subprocess.check_output(['cksum'], input=p.read_bytes())
                             for p in (script, ndk / 'source.properties'))
        recipe = subprocess.check_output(['cksum'], input=checksums).split()[0].decode()
        stage = cache / 'media3-ffmpeg' / recipe
        jni = stage / 'jni'
        jni.mkdir(parents=True)
        for name in ('CMakeLists.txt', 'ffmpeg_jni.cc'):
            (jni / name).write_text('fixture input\n')
        argv_log = root / 'configure.jsonl'
        output = root / 'output with spaces/decoder.aar'
        environment = dict(os.environ, ANDROID_HOME=str(sdk), ANDROID_NDK_HOME=str(ndk),
                           NUVIO_ANDROID_CACHE=str(cache), NUVIO_ANDROID_JOBS='1',
                           FIXTURE_ARGV=str(argv_log), PYTHONDONTWRITEBYTECODE='1', LC_ALL='C',
                           PATH=str(binary) + os.pathsep + os.environ['PATH'])
        result = subprocess.run([SHELL, str(script), str(output), *abis],
                                env=environment, text=True, capture_output=True)
        if not succeeds:
            assert result.returncode != 0, label + ': expected failure'
            if failure_text is not None:
                assert failure_text in result.stdout + result.stderr, (label, result.stdout, result.stderr)
            print('PASS:', label, '(negative control)')
            return
        assert result.returncode == 0, label + ': ' + result.stdout + result.stderr
        calls = [json.loads(line) for line in argv_log.read_text().splitlines()] if argv_log.exists() else []
        assert len(calls) == len(expected), (label, calls)
        for args, abi in zip(calls, expected):
            assert all(args), (label, 'empty configure argument', args)
            assert '--libdir=android-libs/' + abi in args, (label, args)
            assert ('--disable-asm' in args) == (abi == 'x86_64'), (label, args)
            for decoder in ('flac', 'alac', 'pcm_mulaw', 'pcm_alaw', 'mp3', 'aac',
                            'ac3', 'eac3', 'dca', 'mlp', 'truehd'):
                assert '--enable-decoder=' + decoder in args, (label, decoder)
        with zipfile.ZipFile(output) as built:
            assert set(built.namelist()) == set(originals), label
            for name, content in originals.items():
                replacement = any(name == f'jni/{abi}/libffmpegJNI.so' for abi in expected)
                assert (built.read(name) != content) == replacement, (label, name)
        print('PASS:', label)


version = subprocess.check_output([SHELL, '--version'], text=True).splitlines()[0]
print('Shell:', version)
run_case('fresh ARM64', ['arm64-v8a'], ['arm64-v8a'])
run_case('default ABIs with no arguments', [], ['arm64-v8a'])
run_case('32-bit only with zero replacements', ['armeabi-v7a', 'x86'], [])
run_case('x86_64 disables assembly', ['x86_64'], ['x86_64'])
run_case('x86_64 flag does not leak to ARM64', ['x86_64', 'arm64-v8a'], ['x86_64', 'arm64-v8a'])
run_case('unknown ABI rejected', ['unknown-abi'], [], succeeds=False,
         failure_text='ABI desconhecida: unknown-abi')
if re.search(r'\bversion 3\.2\b', version):
    # Establish that this fixture enters the former failing fresh-build path.
    unsafe = SOURCE.replace('"${FF_OPTS[@]}"', '"${EMPTY_ASM[@]}" "${FF_OPTS[@]}"')
    unsafe = unsafe.replace('FF_OPTS=("${COMUM[@]}")', 'EMPTY_ASM=()\n  FF_OPTS=("${COMUM[@]}")')
    assert unsafe != SOURCE
    run_case('former empty-array expansion rejected', ['arm64-v8a'], ['arm64-v8a'],
             source=unsafe, succeeds=False, failure_text='unbound variable')
else:
    print('Bash 3.2 negative control skipped: use BASH32 or macOS /bin/bash')
