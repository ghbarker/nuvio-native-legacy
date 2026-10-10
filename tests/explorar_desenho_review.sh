#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
dir=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-explorar-draw.XXXXXX")
trap 'rm -rf "$dir"' EXIT
flags=(-O1 -g -Isrc -DSDL_MAIN_HANDLED -DNV_LINUX_DESKTOP -ffunction-sections -fdata-sections)
if command -v pkg-config >/dev/null && pkg-config --exists sdl2; then
  read -r -a sdl_flags <<< "$(pkg-config --cflags sdl2)"; flags+=("${sdl_flags[@]}")
else flags+=(-I/opt/homebrew/include -I/opt/homebrew/include/SDL2); fi
case "$(uname -s)" in
  Darwin) flags+=(-Wl,-dead_strip) ;;
  MINGW*|MSYS*) flags+=(-fwhole-program -Wl,--gc-sections) ;;
  *) flags+=(-Wl,--gc-sections) ;;
esac
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
for fixture in explorar_desenho_review explorar_empty_review; do
  cc "${flags[@]}" "tests/$fixture.c" -lm -o "$dir/$fixture"
  "$dir/$fixture"
done
