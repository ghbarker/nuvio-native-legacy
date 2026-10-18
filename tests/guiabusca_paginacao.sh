#!/bin/bash
# #344: buscaFazer REAL (src/guia.c incluido) pagina a grade por posicao, sem
# pular nem repetir programa com sobreposicao. Ver tests/guiabusca_paginacao.c.
#
#   bash tests/guiabusca_paginacao.sh
#   SANITIZE=1 bash tests/guiabusca_paginacao.sh
set -euo pipefail
cd "$(dirname "$0")/.."
flags=(-std=gnu11 -O1 -g -Isrc)
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
if [ "$(uname -s)" = Darwin ]; then
  flags+=(-I/opt/homebrew/include -I/opt/homebrew/include/SDL2 -L/opt/homebrew/lib)
  libs=(-lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL)
else
  read -r -a sdk_flags <<< "$(pkg-config --cflags sdl2 SDL2_image SDL2_ttf)"
  read -r -a sdk_libs <<< "$(pkg-config --libs sdl2 SDL2_image SDL2_ttf)"
  flags+=(-DNV_LINUX_DESKTOP "${sdk_flags[@]}")
  libs=("${sdk_libs[@]}" -lz -lGLESv2 -pthread -lm -ldl)
fi
tmp="$(mktemp -d "${TMPDIR:-/tmp}/nuvio-guiabusca-pag-XXXXXX")"
trap 'rm -rf "$tmp"' EXIT
sources=()
for s in src/*.c src/dts/*.c; do case "$s" in src/main.c|src/guia.c) continue;; esac; sources+=("$s"); done
cc "${flags[@]}" "${sources[@]}" tests/guiabusca_paginacao.c -o "$tmp/t" \
  "${libs[@]}" -Wno-deprecated-declarations
NUVIO_DADOS="$tmp" "$tmp/t"
