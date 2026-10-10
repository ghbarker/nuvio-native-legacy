#!/usr/bin/env bash
set -eu
cd "$(dirname "$0")/.."
dir=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-telefone-folhas.XXXXXX")
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
for module in STREAMS SALVOS AVISOS; do
  "${CC:-cc}" "${flags[@]}" -DTESTE_"$module" tests/telefone_folhas.c -lm -lpthread -o "$dir/teste"
  "$dir/teste"
done
