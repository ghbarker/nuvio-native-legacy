#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
dir=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-playback-actions-review.XXXXXX")
trap 'rm -rf "$dir"' EXIT
flags=(-O2 -g -Isrc -DSDL_MAIN_HANDLED -DNV_LINUX_DESKTOP -ffunction-sections -fdata-sections)
if command -v pkg-config >/dev/null && pkg-config --exists sdl2; then
  read -r -a inc <<< "$(pkg-config --cflags sdl2)"; flags+=("${inc[@]}")
else flags+=(-I/opt/homebrew/include -I/opt/homebrew/include/SDL2); fi
case "$(uname -s)" in
  Darwin) flags+=(-Wl,-dead_strip) ;;
  MINGW*|MSYS*) flags+=(-fwhole-program -Wl,--gc-sections) ;;
  *) flags+=(-Wl,--gc-sections) ;;
esac
for module in ILHA DIAG; do
  cc "${flags[@]}" -DTESTE_"$module" tests/playback_actions_review.c -lm -lpthread -o "$dir/$module"
  "$dir/$module"
done
"$dir/DIAG" intro
