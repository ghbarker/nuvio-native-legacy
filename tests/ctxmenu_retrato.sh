#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
dir=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-ctxmenu-retrato.XXXXXX")
trap 'rm -rf "$dir"' EXIT
flags=(-O1 -g -Isrc -DSDL_MAIN_HANDLED -ffunction-sections -fdata-sections)
if command -v pkg-config >/dev/null && pkg-config --exists sdl2; then
  read -r -a sdl_flags <<< "$(pkg-config --cflags sdl2)"; flags+=("${sdl_flags[@]}")
else flags+=(-I/opt/homebrew/include -I/opt/homebrew/include/SDL2); fi
case "$(uname -s)" in
  Darwin) flags+=(-Wl,-dead_strip) ;;
  *) flags+=(-DNV_LINUX_DESKTOP -Wl,--gc-sections) ;;
esac
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
for oficial in 0 1; do
  extra=(); [ "$oficial" = 0 ] || extra+=(-DNV_CTX_TESTE_OFICIAL)
  cc "${flags[@]}" "${extra[@]}" tests/ctxmenu_retrato.c -lm -o "$dir/ctxmenu-$oficial"
  "$dir/ctxmenu-$oficial"
done
