"""Platform plumbing only; behavioral C fixtures remain byte-identical."""
from pathlib import Path
import os, platform, re, shlex

ROOT = Path(__file__).resolve().parents[1]
SDK = ROOT / 'sdk'
SDK_EXTRA = ROOT / 'sdk-extra.h'
IS_DARWIN = platform.system() == 'Darwin'
CC = shlex.split(os.environ.get('CC', '/usr/bin/cc' if IS_DARWIN else 'cc'))
SECTION_FLAGS = [] if IS_DARWIN else ['-ffunction-sections', '-fdata-sections']
PLATFORM_FLAGS = ['-D_DARWIN_C_SOURCE=1'] if IS_DARWIN else []
DEAD_STRIP = '-Wl,-dead_strip' if IS_DARWIN else '-Wl,--gc-sections'

def sanitizer_flags(darwin=IS_DARWIN):
    flags = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-fno-sanitize-recover=all']
    return flags if darwin else flags + ['-fno-pie', '-no-pie']

def execution_env():
    return dict(os.environ,
        ASAN_OPTIONS=('detect_leaks=0' if IS_DARWIN else 'detect_leaks=1') + ':halt_on_error=1',
        UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')

def unresolved_symbols(stderr, darwin=IS_DARWIN):
    if darwin:
        symbols = re.findall(r'^\s*"([^"]+)", referenced from:\s*$', stderr, re.M)
    else:
        symbols = re.findall(r"undefined reference to [`']([^'`]+)['`]", stderr)
    symbols = sorted(set(symbols))
    assert all(re.fullmatch(r'[A-Za-z_][A-Za-z_0-9]*', s) for s in symbols), symbols
    return symbols

def logical_symbol(symbol, darwin=IS_DARWIN):
    return symbol[1:] if darwin and symbol.startswith('_') else symbol
