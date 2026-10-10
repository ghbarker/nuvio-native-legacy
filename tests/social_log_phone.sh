#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
dir=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-social-log-phone.XXXXXX")
trap 'rm -rf "$dir"' EXIT
flags=(-O2 -g -Isrc -DSDL_MAIN_HANDLED -DNV_LINUX_DESKTOP -ffunction-sections -fdata-sections)
if command -v pkg-config >/dev/null && pkg-config --exists sdl2; then
  read -r -a sdl_flags <<< "$(pkg-config --cflags sdl2)"; flags+=("${sdl_flags[@]}")
else flags+=(-I/opt/homebrew/include -I/opt/homebrew/include/SDL2); fi
case "$(uname -s)" in
  Darwin) flags+=(-Wl,-dead_strip) ;;
  MINGW*|MSYS*) flags+=(-fwhole-program -Wl,--gc-sections) ;;
  *) flags+=(-Wl,--gc-sections) ;;
esac
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
for mode in REACAO AMIGO REGISTRO; do
  "${CC:-cc}" "${flags[@]}" "-DTESTE_$mode" tests/social_log_phone.c -lm -o "$dir/$mode"
  "$dir/$mode"
done
