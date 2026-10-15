"""Plumbing self-checks only; no product source suites execute."""
import importlib.util,json
from pathlib import Path
p=Path(__file__).parent/'player496/portable.py'
s=importlib.util.spec_from_file_location('portable_preflight',p);m=importlib.util.module_from_spec(s);s.loader.exec_module(m)
assert '-fno-pie' not in m.sanitizer_flags(True) and '-no-pie' not in m.sanitizer_flags(True)
assert '-fno-pie' in m.sanitizer_flags(False) and '-no-pie' in m.sanitizer_flags(False)
assert m.unresolved_symbols('Undefined symbols for architecture arm64:\n  "_fixture_target", referenced from:\n      _main in fixture.o\n  "_nv_layout_w", referenced from:\n      _main in fixture.o\nld: symbol(s) not found for architecture arm64\n',True)==['_fixture_target','_nv_layout_w']
assert m.logical_symbol('_nv_layout_w',True)=='nv_layout_w'
assert m.unresolved_symbols("ld: fixture.o: undefined reference to `fixture_target'\n",False)==['fixture_target']
assert m.unresolved_symbols('compiler syntax error',True)==[]
assert m.DEAD_STRIP in ['-Wl,-dead_strip','-Wl,--gc-sections']
assert m.SDK.is_dir() and m.SDK_EXTRA.is_file()
for path in (Path(__file__).parent/'player496').glob('*'):
 if path.suffix in {'.py','.c','.inc'}:assert '/workspace/' not in path.read_text(),str(path)
print(json.dumps({'outcome':'passed','scope':'Portable platform flag/unresolved-symbol/path plumbing self-checks; product suites not executed.'}))
