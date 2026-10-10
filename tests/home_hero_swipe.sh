#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
dir=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-home-hero-swipe.XXXXXX")
trap 'rm -rf "$dir"' EXIT
flags=(-O1 -g -Isrc -DSDL_MAIN_HANDLED -DNV_TOUCH_UI -ffunction-sections -fdata-sections)
if command -v pkg-config >/dev/null && pkg-config --exists sdl2; then
  read -r -a sdl_flags <<< "$(pkg-config --cflags sdl2)"; flags+=("${sdl_flags[@]}")
else flags+=(-I/opt/homebrew/include -I/opt/homebrew/include/SDL2); fi
case "$(uname -s)" in
  Darwin) flags+=(-Wl,-dead_strip) ;;
  MINGW*|MSYS*) flags+=(-DNV_LINUX_DESKTOP -fwhole-program -Wl,--gc-sections) ;;
  *) flags+=(-DNV_LINUX_DESKTOP -Wl,--gc-sections) ;;
esac
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
cc "${flags[@]}" tests/home_hero_swipe.c -lm -o "$dir/swipe"
"$dir/swipe"
