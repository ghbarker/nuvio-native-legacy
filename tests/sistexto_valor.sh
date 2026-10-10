#!/bin/bash
set -eu
cd "$(dirname "$0")/.."
tmp="$(mktemp -d "${TMPDIR:-/tmp}/nuvio-sistexto-valor-XXXXXX")"
trap 'rm -rf "$tmp"' EXIT
cc -std=c99 -O1 -DSDL_MAIN_HANDLED -Isrc $(pkg-config --cflags sdl2) \
  tests/sistexto_valor.c src/sistexto.c -o "$tmp/review"
"$tmp/review"
