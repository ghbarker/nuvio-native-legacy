#!/usr/bin/env python3
"""Exercise whole production watchdog/player functions without an SDL/JNI SDK."""
import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile


def function(source, name, optional=False):
    match = re.search(r'(?m)^(?:static\s+)?(?:int|void)\s+' + re.escape(name) + r'\s*\([^;]*?\)\s*\{', source)
    if not match:
        if optional:
            return ''
        raise AssertionError('production function missing: ' + name)
    # Tokenize strings/comments so braces in diagnostics do not affect the span.
    depth = 0
    tokens = re.compile(r'/\*[\s\S]*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[{}]')
    for token in tokens.finditer(source, source.index('{', match.start())):
        if token.group() == '{':
            depth += 1
        elif token.group() == '}':
            depth -= 1
            if depth == 0:
                return source[match.start():token.end()] + '\n'
    raise AssertionError('unterminated production function: ' + name)


parser = argparse.ArgumentParser()
parser.add_argument('--source-root', type=Path, default=Path(__file__).resolve().parents[1])
parser.add_argument('--app-root', type=Path)
args = parser.parse_args()
root = args.source_root.resolve()
app = ((args.app_root or root) / 'src/app.c').read_text()
player = (root / 'src/player.c').read_text()
backend = (root / 'src/video_android.c').read_text()
helper = function(app, 'fonteAtualFalhou', optional=True)
if helper:
    # The four opening consumers must all use the same current-session signal.
    # Only the VOD watchdog's complete closure is executed in this small fixture.
    for name in ('vigiarFonteGuardada', 'vigiarAntecipada',
                 'vigiarAberturaManual', 'tentarProximaFonteVOD'):
        body = function(app, name)
        assert 'fonteAtualFalhou()' in body, name + ' bypasses source ownership'
        assert 'video_falhou() || player_fonte_falhou()' not in body
with tempfile.TemporaryDirectory(prefix='nuvio-failover-prefetch-') as directory:
    tmp = Path(directory)
    (tmp / 'watchdog.inc').write_text(
        helper +
        function(app, 'aberturaVencida') + function(app, 'tentarProximaFonteVOD'))
    declaration = player.index('static struct {\n  unsigned token;')
    declaration_end = player.index('} preTitulo;', declaration) + len('} preTitulo;')
    (tmp / 'prepared.inc').write_text(player[declaration:declaration_end] + '\n' +
        function(player, 'player_precarregar_cancelar') +
        function(player, 'preTituloLegendaUsar'))
    (tmp / 'player.inc').write_text(''.join(function(player, name) for name in
        ('player_fonte_falhou', 'player_tem_video', 'player_erro_fonte',
         'player_erro_fonte_motivo', 'erroSemVideo', 'temMkv', 'prebuscaCabe',
         'tocarFonte', 'player_voltar_a_esperar', 'player_definir_fonte')))
    # Keep the production closed-player guard and complete subtitle-release
    # block. The rest of player_atualizar is drawing/input, outside this test.
    update = player.index('  if (!aberto) {\n    prebuscaUrl[0] = 0;')
    update_end = player.index('  entrada = anim_mola(', update)
    (tmp / 'prefetch-update.inc').write_text(
        'static void atualizarPrebusca(Uint32 agora) {\n' +
        player[update:update_end] + '}\n')
    (tmp / 'backend.inc').write_text(function(backend, 'video_parar') + function(backend, 'video_falhou'))
    test = Path(__file__).with_suffix('.c')
    flags = ['-std=gnu11', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-DNV_ANDROID',
             '-ffunction-sections', '-fdata-sections', '-I' + str(root / 'src'), '-I' + str(tmp)]
    flags += shlex.split(os.environ.get('CFLAGS', ''))
    if os.environ.get('SANITIZE') == '1':
        flags += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
    link = ['-Wl,-dead_strip'] if os.uname().sysname == 'Darwin' else ['-Wl,--gc-sections']
    output = tmp / 'test'
    subprocess.run([os.environ.get('CC', 'cc'), *flags, str(test),
        str(root / 'src/fontefalha.c'), str(root / 'src/fontefiltro.c'),
        str(root / 'src/inicio.c'), str(root / 'src/linguas.c'),
        '-pthread', *link, '-lm', '-o', str(output)], check=True)
    subprocess.run([str(output)], check=True)
