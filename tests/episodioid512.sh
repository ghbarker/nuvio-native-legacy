#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
dir=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-episodioid512.XXXXXX")
trap 'rm -rf "$dir"' EXIT
flags=(-O1 -g -Wall -Wextra -Isrc)
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
cc "${flags[@]}" src/js.c src/traktscrobble.c tests/episodioid512.c -o "$dir/teste"
"$dir/teste"
