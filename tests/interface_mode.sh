#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
test_dir=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-interface-mode.XXXXXXXX")
trap 'rm -rf "$test_dir"' EXIT
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/ajustes.c) continue;; esac
  sources+=("$source")
done
flags=(-O1 -g -Isrc -DNV_TOUCH_UI -ffunction-sections -fdata-sections)
if [ "$(uname -s)" = Darwin ]; then
  flags+=(-I/opt/homebrew/include -I/opt/homebrew/include/SDL2 -L/opt/homebrew/lib -Wl,-dead_strip)
  libs=(-lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL)
else
  flags+=(-DNV_LINUX_DESKTOP -Wl,--gc-sections)
  read -r -a pkg_flags <<< "$(pkg-config --cflags --libs sdl2 SDL2_image SDL2_ttf)"
  libs=("${pkg_flags[@]}" -lz -lGL -ldl -pthread -lm)
fi
cc "${flags[@]}" "${sources[@]}" tests/interface_mode.c "${libs[@]}" -o "$test_dir/test"
mkdir "$test_dir/data"
"$test_dir/test" "$test_dir/data"
