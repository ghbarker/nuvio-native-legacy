#!/bin/bash
# #216: regressao de toque puro, sem inicializador Cocoa/SDL no host.
set -eu
cd "$(dirname "$0")/.."
dir=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-ponteiro-toque.XXXXXX")
trap 'rm -rf "$dir"' EXIT
flags=(-O1 -g -Wall -Wextra -Isrc -DSDL_MAIN_HANDLED -ffunction-sections -fdata-sections)
if command -v pkg-config >/dev/null && pkg-config --exists sdl2; then
  read -r -a sdl_flags <<< "$(pkg-config --cflags sdl2)"
  flags+=("${sdl_flags[@]}")
else flags+=(-I/opt/homebrew/include -I/opt/homebrew/include/SDL2); fi
case "$(uname -s)" in
  Darwin) flags+=(-Wl,-dead_strip);;
  *) flags+=(-DNV_LINUX_DESKTOP -Wl,--gc-sections);;
esac
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
sources=(tests/ponteiro_toque.c src/ponteiro.c src/ctxlista.c src/layout.c)
cc "${flags[@]}" "${sources[@]}" -lm -o "$dir/teste"
"$dir/teste"
cc "${flags[@]}" -DNV_TOUCH_UI "${sources[@]}" -lm -o "$dir/preview"
"$dir/preview"
