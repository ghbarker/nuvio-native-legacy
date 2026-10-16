#!/bin/bash
# Leitores reais de progresso/identidade + mapa real; sem rede, SDL ou conta.
set -eu
cd "$(dirname "$0")/.."
tmp=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-extras-identidade.XXXXXX")
trap 'rm -rf "$tmp"' EXIT
python3 - "$tmp" <<'PY'
from pathlib import Path
import sys
out = Path(sys.argv[1])
extras = Path('src/extras.c').read_text()
detail = Path('src/detail.c').read_text()
app = Path('src/app.c').read_text()
trakt = Path('src/trakt.h').read_text()
def block(text, start, end):
    assert text.count(start) == text.count(end) == 1
    return text[text.index(start):text.index(end)]
(out/'extras-state.inc').write_text(block(extras, '#define EX_VIS_T ', 'static int  nRel;'))
(out/'extras-import.inc').write_text(block(extras, '// Progress has Trakt numbering;', '// O Trakt devolve a nota como fracao'))
(out/'extras-read.inc').write_text(block(extras, 'int extras_ep_visto(', '#ifdef NV_SHOT_HOOKS\n// Capturas: relacionados fixos'))
(out/'detail-target.inc').write_text(block(detail, 'static void episodioRetomarFonte(', '// Episodio que os COMENTARIOS seguem:'))
(out/'detail-play.inc').write_text(block(detail, 'int detail_pediu_reproduzir(', '// Pedido de reproducao vindo de FORA'))
(out/'detail-start.inc').write_text(block(detail, 'int detail_pediu_do_inicio(', '// Uma vez por pedido: a pagina saiu por ESQUERDA'))
(out/'detail-ready.inc').write_text(block(detail, 'int detail_ep_foco(', 'int detail_assentado('))
(out/'app-target.inc').write_text(block(app, 'static void idDoAlvo(', '// A tela de Diagnostico foi aberta'))
(out/'app-play.inc').write_text(block(app, 'static int gravarCwExplicito(', '// Canal de portal Stalker:'))
(out/'trakt-lease-decl.inc').write_text(block(trakt, 'int trakt_historico_aplicar_se_atual(', '// 1 quando a ultima resposta do Trakt'))
(out/'extras-reset.inc').write_text(block(extras, 'static void zerarPublicado(', '// Titulo sem id que estas fontes conhecam:'))
(out/'extras-request.inc').write_text(block(extras, 'static void esquecerPedido(', 'void extras_pedir(const char *imdb'))
# O corpo usado no join e o MESMO que o leitor de notas consome depois.
assert extras.count('https://api.trakt.tv/shows/%s/seasons?extended=episodes,full') == 1
assert extras.index('temporadasCorpo = rede_baixar_com') < extras.index('// --- nota ---')
assert 'corpo = temporadasCorpo; temporadasCorpo = NULL;' in extras
assert 'extras_pedir_fonte' in detail and 'extras_atualizar_episodios();' in detail
assert 'ci && ci->imdb[0] && alvo[0] &&' in app
assert 'if (!alvo[0] || !stream_lista_do_alvo(alvo)) return;' in app
assert 'if (!alvo || !alvo[0] || !preparo.tem' in app
PY
flags=()
if [ "${SANITIZE:-0}" = 1 ]; then
  flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer)
  if [ "$(uname -s)" = Linux ]; then flags+=(-no-pie); fi
fi
${CC:-cc} ${flags[@]+"${flags[@]}"} tests/extras_episode_identity_512.c src/js.c src/vistoep.c src/proximo.c \
  -I"$tmp" -Isrc -std=c11 -D_DEFAULT_SOURCE -D_DARWIN_C_SOURCE -O1 -g -Wall -Wextra \
  -Wno-address -Wno-unused-function -lpthread -lm -o "$tmp/test"
"$tmp/test"
