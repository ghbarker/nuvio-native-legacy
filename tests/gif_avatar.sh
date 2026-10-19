#!/bin/bash
set -eu
cd "$(dirname "$0")/.."
dir=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-gif-avatar.XXXXXX")
trap 'rm -rf "$dir"' EXIT
flags=(-O1 -g -Isrc -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 -pthread -DNV_ANDROID)
if command -v pkg-config >/dev/null && pkg-config --exists sdl2; then
  read -r -a sdlflags <<< "$(pkg-config --cflags sdl2)"
  flags+=("${sdlflags[@]}")
fi
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
cc "${flags[@]}" src/gif.c tests/gif_avatar.c -lm -o "$dir/teste"
"$dir/teste"
