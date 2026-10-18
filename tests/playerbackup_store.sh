#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-player-backup-store.XXXXXX")
trap 'rm -rf "$work"' EXIT
flags=(-std=gnu11 -O1 -g -Isrc -ffunction-sections -fdata-sections -Wno-deprecated-declarations)
if [ "$(uname -s)" = Darwin ]; then flags+=(-I/opt/homebrew/include -I/opt/homebrew/include/SDL2 -L/opt/homebrew/lib -Wl,-dead_strip);libs=(-lSDL2);else read -r -a sdk_flags <<< "$(pkg-config --cflags sdl2)";read -r -a libs <<< "$(pkg-config --libs sdl2)";flags+=("${sdk_flags[@]}" -Wl,--gc-sections);fi
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer);fi
"${CC:-cc}" "${flags[@]}" src/playerbackup.c tests/playerbackup_store.c "${libs[@]}" -lm -o "$work/test"
"$work/test" "$work"
