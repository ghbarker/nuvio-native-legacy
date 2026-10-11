#!/bin/bash
# Rascunho, cancelamento e persistencia, sem abrir janela ou fazer requisicoes.
#   bash tests/ajustes_ux_interacao.sh
set -eu
cd "$(dirname "$0")/.."
source tests/native_flags.inc
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/ajustes.c) continue;; esac
  sources+=("$source")
done
binary="$(mktemp /tmp/nuvio-ajustes-ux-interacao.XXXXXX)"
trap 'rm -f "$binary"' EXIT
cc "${nv_test_cflags[@]}" "${sources[@]}" tests/ajustes_ux_interacao.c -o "$binary" \
  "${nv_test_libs[@]}" -Wno-deprecated-declarations -Wno-macro-redefined
"$binary"

