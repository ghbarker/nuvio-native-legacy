#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
dir=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-player-touch-resize.XXXXXX")
trap 'rm -rf "$dir"' EXIT
case "$(uname -s)" in
  Darwin) stripflag=-Wl,-dead_strip ;;
  *) stripflag=-Wl,--gc-sections ;;
esac
flags=(-O1 -g -DNV_LINUX_DESKTOP -Isrc -I/opt/homebrew/include -I/opt/homebrew/include/SDL2)
if command -v sdl2-config >/dev/null; then
  read -r -a sdlflags <<< "$(sdl2-config --cflags)"
  flags+=("${sdlflags[@]}")
fi
for module in player trailer; do
  "${CC:-cc}" "${flags[@]}" -ffunction-sections -fdata-sections "$stripflag" \
    "tests/${module}_touch_resize.c" -lm -o "$dir/$module"
  "$dir/$module"
done
