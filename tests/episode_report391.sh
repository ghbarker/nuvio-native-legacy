#!/bin/bash
# Verify #391's photographed title through the actual current metadata worker.
set -eu
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-episode-report391.XXXXXX")
trap 'rm -rf "$work"' EXIT
flags=(-std=gnu11 -O1 -g -Isrc -pthread -Wall -Wno-deprecated-declarations)
if [ "${SANITIZE:-0}" = 1 ]; then
  flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer)
fi
if command -v pkg-config >/dev/null && pkg-config --exists sdl2; then
  read -r -a includes <<< "$(pkg-config --cflags sdl2)"
else
  includes=(-I/opt/homebrew/include -I/opt/homebrew/include/SDL2)
fi
"${CC:-cc}" "${flags[@]}" "${includes[@]}" \
  src/catalogo.c src/cwordem.c tests/episode_report391.c src/cotacat.c \
  src/js.c src/metaprov.c src/colecoes.c src/redeurl.c src/catordem.c src/artefontes.c \
  -o "$work/test"
for scenario in live-nuvio live-cinemeta live-nuvio-rebuild live-cinemeta-rebuild; do
  "$work/test" "$scenario"
done
