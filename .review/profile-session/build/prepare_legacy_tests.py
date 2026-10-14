#!/usr/bin/env python3
"""Link omitted epoch infrastructure only in four retained reduced fixtures.

Changes are confined to the CI working copy of the old test runners. Existing
assertions and their mock transports remain intact. New ownership/race suites
and full-app links never receive these epoch-zero compatibility definitions.
"""
import argparse
from pathlib import Path
import platform
import shlex
import subprocess
import tempfile


def probe_optional_import(log_path):
    if platform.system() != 'Darwin':
        return
    log_path.parent.mkdir(parents=True, exist_ok=True)
    lines = []
    body = '''int main(void) {
  uint64_t g = nv_perfil_sessao_ler();
  if (!nv_perfil_sessao_atual(g) || !nv_perfil_sessao_travar(g)) return 1;
  nv_perfil_sessao_destravar();
  return 0;
}
'''
    variants = {
        'header-only': '#include "perfisessao.h"\n',
        'strong-before': '#include "perfis.h"\n#include "perfisessao.h"\n',
        'strong-after': '#include "perfisessao.h"\n#include "perfis.h"\n',
    }
    modes = {
        'plain': [],
        'asan-ubsan': ['-fsanitize=address,undefined', '-fno-omit-frame-pointer'],
    }
    with tempfile.TemporaryDirectory(prefix='nuvio-optional-import-') as folder:
        folder = Path(folder)
        for variant, headers in variants.items():
            for mode, extra in modes.items():
                name = variant+'-'+mode
                source, obj, exe = (folder/(name+suffix) for suffix in ('.c', '.o', ''))
                source.write_text(headers+body)
                compile_result = subprocess.run(['cc', '-O1', '-g', '-ffunction-sections',
                                                 '-fdata-sections', '-Isrc', *extra, '-c',
                                                 str(source), '-o', str(obj)],
                                                text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
                lines.append(f'=== {name}: compiler status {compile_result.returncode} ===\n'+compile_result.stdout)
                log_path.write_text('\n'.join(lines))
                assert compile_result.returncode == 0, compile_result.stdout
                symbols = subprocess.run(['nm', '-m', str(obj)], text=True,
                                         stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=True)
                lines.append(symbols.stdout)
                log_path.write_text('\n'.join(lines))
                link_result = subprocess.run(['cc', str(obj), *extra, '-Wl,-dead_strip', '-o', str(exe)],
                                             text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
                lines.append(f'link status {link_result.returncode}\n'+link_result.stdout)
                log_path.write_text('\n'.join(lines))
                if link_result.returncode == 0:
                    subprocess.run([str(exe)], check=True)
    log_path.write_text('\n'.join(lines))
    print('\n'.join(lines))
    print('Apple optional-import probe recorded; actual profile suites still link strong producers')


def replace_once(path, old, new):
    if not path.exists():
        return False
    text = path.read_text()
    if new in text:
        return True
    assert text.count(old) == 1, (path, old)
    path.write_text(text.replace(old, new))
    return True


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--probe-log', type=Path)
    args = parser.parse_args()
    if args.probe_log:
        probe_optional_import(args.probe_log)
    compat = shlex.quote(str(Path(__file__).resolve().with_name('legacy-epoch.c')))
    changed = []
    for name in ('fontecache_vod', 'fonte_ordem_addon'):
        old = f'cc "${{flags[@]}}" tests/{name}.c'
        if replace_once(Path('tests')/(name+'.sh'), old, old+' '+compat):
            changed.append(name)
    old = 'sources=(tests/stream_faixas_ui.c'
    if replace_once(Path('tests/stream_faixas_ui.sh'), old, old+' '+compat):
        changed.append('stream_faixas_ui')
    old = '"$task_review/tests/watched_discovery.c" src/js.c src/metaprov.c'
    if replace_once(Path('.review/hide-watched/build/run_regressions.sh'), old, old+' '+compat):
        changed.append('watched_discovery')
    assert 'fontecache_vod' in changed, 'Expected the original reduced VOD-cache fixture'
    print('Legacy epoch-zero compatibility linked only into: '+', '.join(changed))


if __name__ == '__main__':
    main()
