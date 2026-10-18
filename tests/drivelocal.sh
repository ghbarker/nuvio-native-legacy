#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-drive-local-test.XXXXXX")
trap 'rm -rf "$work"' EXIT
flags=(-std=gnu11 -O1 -g -Wall -Wextra -Werror -Isrc -pthread)
if [ "$(uname -s)" != Darwin ]; then flags+=(-ldl);fi
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer);fi
# Keep warnings fatal for the new protocol code. Existing transport sources
# have GCC 14 format-truncation/indentation warnings; retain those diagnostics
# without changing unrelated network code in a backup feature.
"${CC:-cc}" "${flags[@]}" -Wno-error -c src/rede.c -o "$work/rede.o"
"${CC:-cc}" "${flags[@]}" -Wno-error -c src/redeurl.c -o "$work/redeurl.o"
"${CC:-cc}" "${flags[@]}" src/backuplocal.c src/drivelocal.c src/js.c src/jsw.c tests/drivelocal.c "$work/rede.o" "$work/redeurl.o" -lz -lm -o "$work/test"
"$work/test"
