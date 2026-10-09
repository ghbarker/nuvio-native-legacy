#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
dir=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-snap-retrato.XXXXXX")
trap 'rm -rf "$dir"' EXIT
flags=(-O1 -g -Isrc -ffunction-sections -fdata-sections)
read -r -a includes <<< "$(pkg-config --cflags sdl2 SDL2_image)"
flags+=("${includes[@]}")
case "$(uname -s)" in
  Darwin) flags+=(-Wl,-dead_strip) ;;
  *) flags+=(-DNV_LINUX_DESKTOP -Wl,--gc-sections) ;;
esac
cc "${flags[@]}" tests/snap_retrato.c -lm -o "$dir/teste"
"$dir/teste"
