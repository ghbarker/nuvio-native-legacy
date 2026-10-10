#!/usr/bin/env bash
set -eu
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-detail-arte-body.XXXXXX")
trap 'rm -rf "$work"' EXIT
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/detail.c) continue;; esac
  sources+=("$source")
done
flags=(-O1 -g -Isrc -DNV_TOUCH_UI -DSDL_MAIN_HANDLED -ffunction-sections -fdata-sections)
if command -v pkg-config >/dev/null && pkg-config --exists sdl2 SDL2_image SDL2_ttf; then
  read -r -a libs <<< "$(pkg-config --cflags --libs sdl2 SDL2_image SDL2_ttf)"
else libs=(-I/opt/homebrew/include -I/opt/homebrew/include/SDL2 -L/opt/homebrew/lib -lSDL2 -lSDL2_image -lSDL2_ttf); fi
case "$(uname -s)" in
  Darwin) libs+=(-framework OpenGL -Wl,-dead_strip);;
  *) flags+=(-DNV_LINUX_DESKTOP); libs+=(-lGLESv2 -Wl,--gc-sections);;
esac
cc "${flags[@]}" "${sources[@]}" tests/detail_arte_body.c "${libs[@]}" -lz -lm -pthread -o "$work/test"
mkdir "$work/dados"
NUVIO_DADOS="$work/dados" "$work/test"
