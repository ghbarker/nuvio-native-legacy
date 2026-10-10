#!/usr/bin/env python3
"""Run existing local-data GL snapshots at phone dimensions, without an account."""
from pathlib import Path
import argparse
import json
import os
import re
import shlex
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

ROOT = Path(__file__).resolve().parents[1]

def run(command, **kwargs):
    return subprocess.run(command, cwd=ROOT, check=True, **kwargs)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', default='build/phone-review')
    parser.add_argument('--fixtures', nargs='+', default=[
        'ajustes', 'busca', 'vertudo', 'biblioteca', 'explorar', 'agenda',
        'detail_secoes', 'detalhe_glass', 'pipintro', 'novidades20', 'novidades_cartao',
        'spotlight', 'perfil', 'pessoas', 'recenviar', 'central', 'registro',
        'diagnostico', 'livetvdiag', 'trocaarte', 'ilha3', 'player_glass'])
    parser.add_argument('--cases', nargs='+', default=['1080x2340@1', '2340x1080@1.5'])
    parser.add_argument('--all-settings', action='store_true')
    parser.add_argument('--settings-layout', nargs='+', type=int, choices=[0, 1], default=[0, 1])
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    if ROOT not in output.parents:
        raise SystemExit('Output must remain inside the workspace')
    output.mkdir(parents=True, exist_ok=True)
    objects = output / 'objects'
    objects.mkdir(exist_ok=True)
    flags = ['-O1', '-g', '-DNV_LINUX_DESKTOP', '-DNV_TOUCH_PREVIEW',
             '-DAJUSTES_TESTE', '-DNV_SHOT_HOOKS', '-DCENTRAL_TESTE',
             '-DREGISTRO_TESTE', '-DAVISOS_TESTE_ENVIO', '-DDESEMPENHO_TESTE',
             '-DTELEMETRIA_TESTE', '-DSDL_MAIN_HANDLED', '-ffunction-sections',
             '-fdata-sections', '-Isrc', '-Itests']
    cflags = shlex.split(subprocess.check_output(
        ['pkg-config', '--cflags', 'sdl2', 'SDL2_image', 'SDL2_ttf', 'glesv2', 'egl', 'zlib'], text=True))
    libraries = shlex.split(subprocess.check_output(
        ['pkg-config', '--libs', 'sdl2', 'SDL2_image', 'SDL2_ttf', 'glesv2', 'egl', 'zlib'], text=True))
    sources = sorted((ROOT / 'src').glob('*.c')) + sorted((ROOT / 'src/dts').glob('*.c'))
    def compile_source(source):
        relative = source.relative_to(ROOT)
        target = objects / ('_'.join(relative.parts) + '.o')
        # Use the Android option catalog with the desktop drawing backend.
        # The other modules keep their Linux video/network stubs.
        catalog_flags = ['-DNV_ANDROID'] if source.name == 'ajustes.c' else []
        with target.with_suffix('.log').open('w') as stream:
            run(['cc', *flags, *catalog_flags, *cflags, '-c', str(relative), '-o', str(target)],
                stdout=stream, stderr=subprocess.STDOUT)
        return target
    sources = [source for source in sources if source.name != 'main.c']
    with ThreadPoolExecutor(max_workers=2) as pool:
        source_objects = list(pool.map(compile_source, sources))
    print(f'Built {len(source_objects)} drawing modules', flush=True)
    archive = output / 'libphone-review.a'
    run(['ar', 'rcs', str(archive), *map(str, source_objects)])
    results = []
    for name in args.fixtures:
        print(f'Capturing {name}', flush=True)
        if not re.fullmatch(r'[a-z0-9_]+', name):
            raise SystemExit('Invalid fixture name')
        fixture = ROOT / 'tests' / f'{name}_shot.c'
        binary = output / f'{name}-shot'
        log = output / f'{name}-build.log'
        try:
            with log.open('w') as stream:
                run(['cc', *flags, *cflags, '-include', 'tests/phone_shot.h',
                     str(fixture), str(archive), *libraries, '-ldl', '-pthread', '-lm',
                     '-Wl,--gc-sections', '-Wl,--wrap=st_ime_disponivel', '-o', str(binary)],
                    stdout=stream, stderr=subprocess.STDOUT)
        except subprocess.CalledProcessError:
            results.append({'fixture': name, 'status': 'build failed', 'log': log.name})
            continue
        for case in args.cases:
            match = re.fullmatch(r'(\d+)x(\d+)@([\d.]+)', case)
            if not match:
                raise SystemExit('Invalid capture case')
            width, height, scale = match.groups()
            folder = output / name / case.replace('@', '-ui')
            folder.mkdir(parents=True, exist_ok=True)
            # Some existing fixtures take a directory, others a file prefix.
            # Provide both without changing their individual test semantics.
            (folder / name).mkdir(exist_ok=True)
            data_name = {'detail_secoes': 'nuvio-detsec-dados',
                         'detalhe_glass': 'nuvio-detglass-dados',
                         'trocaarte': 'nuvio-trocaarte-dados',
                         'spainel_abas': 'nuvio-abas-shot'}.get(name, 'data')
            data = folder / data_name
            data.mkdir(exist_ok=True)
            environment = dict(os.environ, NUVIO_DADOS=str(data), NUVIO_TESTE_DIR=str(data),
                               NUVIO_PHONE_SHOT_W=width, NUVIO_PHONE_SHOT_H=height,
                               NUVIO_TAMANHO_UI=scale, LIBGL_ALWAYS_SOFTWARE='1')
            log = folder / 'capture.log'
            try:
                with log.open('w') as stream:
                    run([str(binary), str(folder / name)], env=environment,
                        stdout=stream, stderr=subprocess.STDOUT, timeout=600)
                pngs = sorted(folder.rglob('*.png'))
                if not pngs:
                    raise RuntimeError('Fixture produced no captures')
                from PIL import Image
                for png in pngs:
                    with Image.open(png) as image:
                        if image.size != (int(width), int(height)):
                            raise RuntimeError(f'Unexpected framebuffer size: {png.name}')
                results.append({'fixture': name, 'case': case, 'status': 'captured',
                                'images': [str(p.relative_to(output)) for p in pngs]})
            except (subprocess.CalledProcessError, subprocess.TimeoutExpired, RuntimeError) as error:
                results.append({'fixture': name, 'case': case, 'status': 'capture failed',
                                'error': str(error), 'log': str(log.relative_to(output))})
    if args.all_settings and 'ajustes' in args.fixtures:
        binary = output / 'ajustes-shot'
        if binary.exists():
            for layout in args.settings_layout:
                folder = output / 'settings-catalog' / ('list' if layout else 'panel')
                folder.mkdir(parents=True, exist_ok=True)
                data = folder / 'data'
                data.mkdir(exist_ok=True)
                environment = dict(os.environ, NUVIO_DADOS=str(data),
                                   NUVIO_PHONE_SHOT_W='1080', NUVIO_PHONE_SHOT_H='2340',
                                   NUVIO_TAMANHO_UI='1', LIBGL_ALWAYS_SOFTWARE='1',
                                   NUVIO_PHONE_SETTINGS_ALL='1', NUVIO_SHOT_AVANCADAS='1',
                                   NUVIO_SHOT_AJUSTES_ESCALA='100',
                                   NUVIO_SHOT_LAYOUT=str(layout))
                log = folder / 'capture.log'
                try:
                    with log.open('w') as stream:
                        run([str(binary), str(folder / 'settings')], env=environment,
                            stdout=stream, stderr=subprocess.STDOUT, timeout=1200)
                    pngs = sorted(folder.glob('*.png'))
                    if len(pngs) < 200:
                        raise RuntimeError(f'Only {len(pngs)} catalog images were produced')
                    from PIL import Image
                    for png in pngs:
                        with Image.open(png) as image:
                            if image.size != (1080, 2340):
                                raise RuntimeError(f'Unexpected framebuffer size: {png.name}')
                    results.append({'fixture': 'settings-catalog', 'case': folder.name,
                                    'status': 'captured', 'count': len(pngs),
                                    'images': [str(p.relative_to(output)) for p in pngs]})
                except (subprocess.CalledProcessError, subprocess.TimeoutExpired, RuntimeError) as error:
                    results.append({'fixture': 'settings-catalog', 'case': folder.name,
                                    'status': 'capture failed', 'error': str(error),
                                    'log': str(log.relative_to(output))})
    (output / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
    for result in results:
        print(result['fixture'], result.get('case', ''), result['status'])
    return 0 if all(r['status'] == 'captured' for r in results) else 1

if __name__ == '__main__':
    sys.exit(main())
