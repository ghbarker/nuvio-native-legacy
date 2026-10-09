#!/usr/bin/env bash
set -eu
cd "$(dirname "$0")/.."
dir=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-gpun-touch.XXXXXX")
trap 'rm -rf "$dir"' EXIT
flags=(-O1 -g -Isrc -DNV_TOUCH_PREVIEW -ffunction-sections -fdata-sections)
if command -v pkg-config >/dev/null && pkg-config --exists sdl2; then
  read -r -a includes <<< "$(pkg-config --cflags sdl2)"
  flags+=("${includes[@]}")
else flags+=(-I/opt/homebrew/include); fi
case "$(uname -s)" in
  Darwin) flags+=(-Wl,-dead_strip);;
  *) flags+=(-Wl,--gc-sections);;
esac
cc "${flags[@]}" tests/gpun_touch.c src/layout.c -lm -o "$dir/teste"
"$dir/teste"
