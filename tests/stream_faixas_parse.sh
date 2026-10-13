#!/bin/bash
set -eu
cd "$(dirname "$0")/.."
dir=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-stream-faixas-parser.XXXXXX")
trap 'rm -rf "$dir"' EXIT
flags=(-std=gnu11 -O1 -g -Isrc -Wall -Wextra -ffunction-sections -fdata-sections)
if [ "$(uname -s)" = Darwin ]; then
  flags+=(-I/opt/homebrew/include -I/opt/homebrew/include/SDL2)
  linker=(-Wl,-dead_strip)
else
  linker=(-Wl,--gc-sections)
  if pkg-config --exists sdl2; then
    read -r -a sdk <<< "$(pkg-config --cflags sdl2)"
    flags+=("${sdk[@]}")
  fi
fi
if [ -n "${CFLAGS:-}" ]; then read -r -a extra <<< "$CFLAGS"; flags+=("${extra[@]}"); fi
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined,float-cast-overflow -fno-omit-frame-pointer -fno-sanitize-recover=all); fi
cc "${flags[@]}" tests/stream_faixas_parse.c src/stream_parse.c src/js.c src/badges.c \
  "${linker[@]}" -lm -o "$dir/test"
"$dir/test"
