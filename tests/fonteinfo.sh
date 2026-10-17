#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-fonteinfo.XXXXXX")
trap 'rm -rf "$work"' EXIT
flags=(-O1 -g -Isrc -ffunction-sections -fdata-sections -I/opt/homebrew/include -I/opt/homebrew/include/SDL2)
if [ "$(uname -s)" = Darwin ]; then flags+=(-Wl,-dead_strip); else flags+=(-Wl,--gc-sections); fi
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
cc "${flags[@]}" tests/fonteinfo.c src/fonteinfo.c src/fontefiltro.c src/badges.c src/linguas.c -lm -o "$work/test"
"$work/test"
