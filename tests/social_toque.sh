#!/bin/bash
set -eu
cd "$(dirname "$0")/.."
dir=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-social-toque.XXXXXX")
trap 'rm -rf "$dir"' EXIT
flags=(-O1 -g -Wall -Wextra -Isrc -DSDL_MAIN_HANDLED -ffunction-sections -fdata-sections)
if command -v pkg-config >/dev/null && pkg-config --exists sdl2; then
  read -r -a sdl_flags <<< "$(pkg-config --cflags sdl2)"
  flags+=("${sdl_flags[@]}")
else flags+=(-I/opt/homebrew/include -I/opt/homebrew/include/SDL2); fi
case "$(uname -s)" in
  Darwin) flags+=(-Wl,-dead_strip);;
  *) flags+=(-DNV_LINUX_DESKTOP -Wl,--gc-sections);;
esac
for tela in PESSOAS OPINIOES PERFIL AMIGOS ENVIAR ARTE GUIA; do
  cc "${flags[@]}" -DTESTE_$tela tests/social_toque.c -lm -o "$dir/$tela"
  "$dir/$tela"
done
