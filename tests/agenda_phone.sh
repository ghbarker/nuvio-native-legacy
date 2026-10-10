#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
dir=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-agenda-phone.XXXXXX")
trap 'rm -rf "$dir"' EXIT
flags=(-O1 -g -Isrc -ffunction-sections -fdata-sections)
if command -v pkg-config >/dev/null && pkg-config --exists sdl2; then
  read -r -a sdl_flags <<< "$(pkg-config --cflags sdl2)"; flags+=("${sdl_flags[@]}")
else flags+=(-I/opt/homebrew/include -I/opt/homebrew/include/SDL2); fi
case "$(uname -s)" in
  Darwin) flags+=(-Wl,-dead_strip);;
  *) flags+=(-DNV_LINUX_DESKTOP -Wl,--gc-sections);;
esac
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
for variant in phone tv; do
  extra=(); if [ "$variant" = tv ]; then extra=(-DAGENDA_TV_REVIEW); fi
  cc "${flags[@]}" "${extra[@]}" tests/agenda_phone.c -lm -o "$dir/$variant"
  "$dir/$variant"
done
