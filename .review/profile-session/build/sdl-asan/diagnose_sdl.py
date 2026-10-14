#!/usr/bin/env python3
"""Probe real Homebrew SDL2 compatibility loading with unchanged ASan enabled."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

from bounded_run import capture, run

PROBE = r'''#include <SDL.h>
#include <stdio.h>
int main(void) {
  fprintf(stderr, "REAL SDL PROBE ENTERED MAIN\n");
  if (SDL_Init(SDL_INIT_TIMER) != 0) {
    fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
    return 1;
  }
  SDL_version version;
  SDL_GetVersion(&version);
  fprintf(stderr, "SDL version: %u.%u.%u\n", version.major, version.minor, version.patch);
  SDL_Quit();
  return 0;
}
'''


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--candidate-only', action='store_true')
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    commands = [('os', ['sw_vers']), ('architecture', ['uname', '-a']),
                ('compiler', ['cc', '--version']), ('xcode', ['xcodebuild', '-version']),
                ('developer-directory', ['xcode-select', '-p']),
                ('sdk', ['xcrun', '--show-sdk-path']), ('sdk-version', ['xcrun', '--show-sdk-version']),
                ('compiler-resource', ['cc', '-print-resource-dir']), ('brew-config', ['brew', 'config']),
                ('brew-packages', ['brew', 'info', '--json=v2', 'sdl2', 'sdl3', 'sdl2_image', 'sdl2_ttf'])]
    if not args.candidate_only:
        for name, command in commands:
            capture(command, output / (name + '.txt'), 20)
        for package in ('sdl2', 'sdl3', 'sdl2_image', 'sdl2_ttf'):
            prefix = Path(subprocess.check_output(['brew', '--prefix', package], text=True).strip())
            evidence = output / package
            evidence.mkdir(exist_ok=True)
            receipt = prefix / 'INSTALL_RECEIPT.json'
            if receipt.exists():
                (evidence / 'INSTALL_RECEIPT.json').write_bytes(receipt.read_bytes())
            for formula in (prefix / '.brew').glob('*.rb'):
                (evidence / formula.name).write_bytes(formula.read_bytes())
            for lib in (prefix / 'lib').glob('libSDL*.dylib'):
                if lib.is_symlink():
                    continue
                label = lib.name
                (evidence / (label + '.identity.json')).write_text(json.dumps({
                    'path': str(lib), 'realpath': str(lib.resolve()),
                    'sha256': hashlib.sha256(lib.read_bytes()).hexdigest()}, indent=2) + '\n')
                capture(['otool', '-L', str(lib)], evidence / (label + '.libraries.txt'))
                capture(['otool', '-l', str(lib)], evidence / (label + '.load-commands.txt'))
    source = output / 'real-sdl-probe.c'
    source.write_text(PROBE)
    binary = output / 'real-sdl-probe-asan'
    compile_command = ['cc', '-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-O1', '-g',
                       '-I/opt/homebrew/include', '-I/opt/homebrew/include/SDL2', str(source),
                       '-o', str(binary), '-L/opt/homebrew/lib', '-lSDL2', '-lSDL2_image', '-lSDL2_ttf',
                       '-lz', '-framework', 'OpenGL']
    candidate = os.environ.copy()
    sdl3lib = subprocess.check_output(['brew', '--prefix', 'sdl3'], text=True).strip() + '/lib'
    previous = candidate.get('DYLD_LIBRARY_PATH', '')
    candidate['DYLD_LIBRARY_PATH'] = sdl3lib + (':' + previous if previous else '')
    (output / 'loader-path.json').write_text(json.dumps({'candidate_sdl3_library_directory': sdl3lib,
        'candidate_DYLD_LIBRARY_PATH': candidate['DYLD_LIBRARY_PATH'],
        'inherited_DYLD_LIBRARY_PATH': previous}, indent=2) + '\n')
    baseline_rc = None
    lookup_rc = None
    if not args.candidate_only:
        (output / 'compile-command.json').write_text(json.dumps(compile_command, indent=2) + '\n')
        if capture(compile_command, output / 'compile.log', 60) != 0:
            raise SystemExit('Real SDL + ASan baseline probe failed to compile')
        capture(['otool', '-L', str(binary)], output / 'probe-libraries.txt')
        capture(['otool', '-l', str(binary)], output / 'probe-load-commands.txt')
        baseline_rc = run([str(binary)], output / 'without-explicit-sdl3-path', 10)
        if baseline_rc != 0:
            # Diagnostic launch only: stop before NSAlert, read its C-string argument,
            # and terminate this separate launch. No interposition or SDL stub.
            commands_file = output / 'error-dialog.lldb'
            commands_file.write_text('breakpoint set --name error_dialog\nrun\n'
                'thread backtrace all\nimage list -o -f\n'
                'script f=lldb.debugger.GetSelectedTarget().GetProcess().GetSelectedThread().GetSelectedFrame(); '
                'r=f.FindRegister("x0" if "arm64" in f.GetThread().GetProcess().GetTarget().GetTriple() else "rdi"); '
                'e=lldb.SBError(); print("SDL2 error_dialog: " + '
                'f.GetThread().GetProcess().ReadCStringFromMemory(r.GetValueAsUnsigned(), 4096, e)); print(e)\n'
                'process kill\nquit\n')
            run(['lldb', '--batch', '-s', str(commands_file), '--', str(binary)],
                output / 'error-dialog-debugger', 10)
        lookup_rc = run([str(binary)], output / 'with-explicit-sdl3-path', 10, env=candidate)
    rpath_binary = output / 'real-sdl-probe-asan-rpath'
    rpath_command = list(compile_command)
    rpath_command[rpath_command.index('-o') + 1] = str(rpath_binary)
    rpath_command.append('-Wl,-rpath,' + sdl3lib)
    (output / 'rpath-compile-command.json').write_text(json.dumps(rpath_command, indent=2) + '\n')
    if capture(rpath_command, output / 'rpath-compile.log', 60) != 0:
        raise SystemExit('Real SDL + ASan rpath probe failed to compile')
    capture(['otool', '-L', str(rpath_binary)], output / 'rpath-probe-libraries.txt')
    capture(['otool', '-l', str(rpath_binary)], output / 'rpath-probe-load-commands.txt')
    candidate_rc = run([str(rpath_binary)], output / 'with-linked-sdl3-rpath-original-environment', 10)
    report = {'baseline_returncode': baseline_rc, 'temporary_loader_path_returncode': lookup_rc,
              'linked_rpath_returncode': candidate_rc, 'candidate_passed': candidate_rc == 0,
              'linked_rpath': sdl3lib, 'durable_probe_uses_original_environment': True,
              'full_source_tests_validated': False}
    (output / 'probe-result.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))
    raise SystemExit(0 if candidate_rc == 0 else 1)


if __name__ == '__main__':
    main()
