#!/bin/bash
# Source-list cache chip: All -> Cached -> Uncached; local fixtures, no network.
set -euo pipefail
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-source-cache.XXXXXX")
trap 'rm -rf "$work"' EXIT
sources=()
for source in src/*.c src/dts/*.c; do
  [[ "$source" != src/main.c && "$source" != src/streams.c ]] && sources+=("$source")
done
cflags=(-O1 -g -Isrc -Wno-deprecated-declarations -Wno-macro-redefined)
ldflags=(-lz -lm)
if [[ "${NV_TEST_SANITIZE:-0}" == 1 ]]; then
  cflags+=(-fsanitize=address,undefined -fno-omit-frame-pointer)
  ldflags+=(-fsanitize=address,undefined)
fi
if [[ "$(uname -s)" == Darwin ]]; then
  cflags+=(-I/opt/homebrew/include -I/opt/homebrew/include/SDL2)
  ldflags+=(-L/opt/homebrew/lib -lSDL2 -lSDL2_image -lSDL2_ttf -framework OpenGL)
else
  read -r -a sdlflags <<< "$(pkg-config --cflags --libs sdl2 SDL2_image SDL2_ttf)"
  ldflags+=("${sdlflags[@]}" -lGL)
fi
cc "${sources[@]}" tests/source_cache_filter.c "${cflags[@]}" \
  -o "$work/test" "${ldflags[@]}"
ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}" NUVIO_DADOS="$work" "$work/test"
