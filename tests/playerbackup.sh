#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-player-backup.XXXXXX")
trap 'rm -rf "$work"' EXIT
flags=(-std=c11 -O1 -g -Wall -Wextra -Werror -Isrc)
if [ "$(uname -s)" = Darwin ]; then flags+=(-I/opt/homebrew/include -I/opt/homebrew/include/SDL2); else read -r -a sdk_flags <<< "$(pkg-config --cflags sdl2)"; flags+=("${sdk_flags[@]}"); fi
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
"${CC:-cc}" "${flags[@]}" src/playerbackup.c tests/playerbackup.c -o "$work/test"
"$work/test"
