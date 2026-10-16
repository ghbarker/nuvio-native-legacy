#!/bin/bash
# #512: numeric TVDB/TMDB/Trakt episode ids stay in the scrobble body.
set -euo pipefail
cd "$(dirname "$0")/.."
flags=()
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
dir=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-trakt512ids.XXXXXX")
trap 'rm -rf "$dir"' EXIT
cc ${flags[@]+"${flags[@]}"} src/traktscrobble.c src/metaep.c src/js.c tests/trakt_scrobble_ids.c \
  -Isrc -o "$dir/teste" -O1 -g -Wall -Wextra
"$dir/teste"
