#!/bin/bash
# Capturas da Agenda em BMP, sem interacao e sem rede.
#
#   bash tests/agenda_shot.sh /tmp/nuvio-agenda
#
# Nao entra na suite (tools/testa-tudo.sh pula *_shot.sh): precisa de janela GL
# e de olho humano para julgar.
#
# NUVIO_DADOS aponta para uma pasta temporaria e o .c ABORTA se dados_dir() nao
# for exatamente ela — a captura escreve agenda-p1.txt e lembretes-p1.txt, e um
# teste deste repositorio ja escreveu por cima dos dados reais do dono.
set -eu
cd "$(dirname "$0")/.."
source tests/native_flags.inc

NUVIO_DADOS=$(mktemp -d /tmp/nuvio-agenda-shot.XXXXXX)
export NUVIO_DADOS
trap 'rm -rf "$NUVIO_DADOS"' EXIT
echo "dados da captura em $NUVIO_DADOS"

sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c) continue;; esac
  sources+=("$source")
done
cc "${nv_test_cflags[@]}" "${sources[@]}" tests/agenda_shot.c -o /tmp/nuvio-agenda-shot \
  "${nv_test_libs[@]}" -Wno-deprecated-declarations -Wno-macro-redefined
/tmp/nuvio-agenda-shot "$@"

