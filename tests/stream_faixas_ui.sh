#!/bin/bash
# Metadata-only source previews: real parser, row renderer, layout and cache.
set -euo pipefail
cd "$(dirname "$0")/.."
task_dir=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-stream-faixas-ui.XXXXXX")
trap 'rm -rf "$task_dir"' EXIT
flags=(-std=gnu11 -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L -O1 -g -Isrc -pthread
       -Wall -Wextra -Wno-unused-function -ffunction-sections -fdata-sections)
if [ "$(uname -s)" = Darwin ]; then
  flags+=(-D_DARWIN_C_SOURCE -I/opt/homebrew/include -I/opt/homebrew/include/SDL2)
  linker=(-Wl,-dead_strip)
else
  flags+=(-DNV_LINUX_DESKTOP)
  linker=(-Wl,--gc-sections)
  if pkg-config --exists sdl2; then
    read -r -a sdk <<< "$(pkg-config --cflags sdl2)"
    flags+=("${sdk[@]}")
  fi
fi
# CPU-only SDK declaration shims can be supplied without SDL/GL libraries.
if [ -n "${CFLAGS:-}" ]; then read -r -a extra <<< "$CFLAGS"; flags+=("${extra[@]}"); fi
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
sources=(tests/stream_faixas_ui.c src/fontesmortas.c src/stream_parse.c
         src/js.c src/badges.c
         src/streamfit.c src/naovideo.c src/limpa.c src/dobra.c src/linguas.c src/fontecache.c)
for modo in tv touch; do
  touch_flags=(-UNV_TOUCH_UI)
  if [ "$modo" = touch ]; then touch_flags=(-DNV_TOUCH_UI); fi
  cc "${flags[@]}" "${touch_flags[@]}" "${sources[@]}" "${linker[@]}" -lm -o "$task_dir/test-$modo"
  "$task_dir/test-$modo"
done
