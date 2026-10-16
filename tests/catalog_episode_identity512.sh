#!/bin/bash
# Real catalog storage and real profile epoch; no network or GUI initialization.
set -eu
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-catalog-episode-identity512.XXXXXX")
trap 'rm -rf "$work"' EXIT
flags=(-std=gnu11 -O1 -g -Isrc -pthread -Wall -Wno-deprecated-declarations
       -DNV_PERFIL_SESSAO_FORTE -ffunction-sections -fdata-sections)
if [ "${SANITIZE:-0}" = 1 ]; then
  flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer)
elif [ "${SANITIZE:-0}" = thread ]; then
  flags+=(-fsanitize=thread -fno-omit-frame-pointer)
fi
if command -v pkg-config >/dev/null && pkg-config --exists sdl2; then
  read -r -a includes <<< "$(pkg-config --cflags sdl2)"
else
  includes=(-I/opt/homebrew/include -I/opt/homebrew/include/SDL2)
fi
case "$(uname -s)" in
  Darwin) link=(-Wl,-dead_strip) ;;
  *) link=(-Wl,--gc-sections) ;;
esac
"${CC:-cc}" "${flags[@]}" "${includes[@]}" src/catalogo.c src/perfis.c \
  tests/catalog_episode_identity512.c "${link[@]}" -o "$work/test"
for scenario in lookups snapshot-identity ambiguity owned-snapshot stale-session concurrent-snapshot; do
  "$work/test" "$scenario"
done
