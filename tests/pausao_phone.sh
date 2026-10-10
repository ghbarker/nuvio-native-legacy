#!/usr/bin/env bash
set -eu
cd "$(dirname "$0")/.."
dir=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-pausao-phone.XXXXXX")
trap 'rm -rf "$dir"' EXIT
flags=(-O1 -g -DNV_LINUX_DESKTOP -Isrc -ffunction-sections -fdata-sections)
if command -v pkg-config >/dev/null && pkg-config --exists sdl2; then
  read -r -a includes <<< "$(pkg-config --cflags sdl2)"
  flags+=("${includes[@]}")
else flags+=(-I/opt/homebrew/include); fi
case "$(uname -s)" in
  Darwin) flags+=(-Wl,-dead_strip);;
  MINGW*|MSYS*) flags+=(-fwhole-program -Wl,--gc-sections);;
  *) flags+=(-Wl,--gc-sections);;
esac
"${CC:-cc}" "${flags[@]}" tests/pausao_phone.c -lm -o "$dir/teste"
"$dir/teste"
