#!/usr/bin/env bash
# Real Settings event routes and disposable on-disk preferences, without a window.
set -euo pipefail
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-settings-phone-list.XXXXXX")
trap 'rm -rf "$work"' EXIT
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/ajustes.c) continue;; esac
  sources+=("$source")
done
flags=(-O1 -g -Isrc -DNV_TOUCH_UI -DSDL_MAIN_HANDLED -ffunction-sections -fdata-sections)
libs=(-lm -pthread)
case "$(uname -s)" in
  Darwin)
    flags+=(-I/opt/homebrew/include -I/opt/homebrew/include/SDL2 -Wl,-dead_strip)
    libs+=(-L/opt/homebrew/lib -lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL)
    ;;
  *)
    cflags_text=$(pkg-config --cflags sdl2 SDL2_image SDL2_ttf glesv2 egl zlib)
    libs_text=$(pkg-config --libs sdl2 SDL2_image SDL2_ttf glesv2 egl zlib)
    read -r -a cflags <<< "$cflags_text"
    read -r -a system_libs <<< "$libs_text"
    flags+=(-DNV_LINUX_DESKTOP -Wl,--gc-sections "${cflags[@]}")
    libs+=("${system_libs[@]}" -ldl)
    ;;
esac
cc "${flags[@]}" "${sources[@]}" tests/ajustes_ux_interacao.c "${libs[@]}" -o "$work/routes"
"$work/routes"
