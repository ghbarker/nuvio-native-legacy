#!/usr/bin/env bash
# Review-only fixture: includes the selected tree's real src/player.c.
# Runs real open/source/close flows against owned future-MKV and native transports.
# Uses real SDL headers on macOS/Linux, or explicit declaration-only SDK headers.
set -euo pipefail
player_fixture_dir=$(cd "$(dirname "$0")" && pwd)
player_product_root=${PRELOAD_PLAYER_SOURCE_ROOT:-}
if [ "${1:-}" = --root ]; then
  player_product_root=$2
  shift 2
elif [ "$#" -gt 0 ]; then
  player_product_root=$1
  shift
fi
if [ -z "$player_product_root" ] || [ "$#" -ne 0 ]; then
  printf '%s\n' 'Usage: preload_player.sh --root <product-root> (SANITIZE=0|1|both)' >&2
  exit 2
fi
player_product_root=$(cd "$player_product_root" && pwd)
test -f "$player_product_root/src/player.c"
player_build_dir=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-preload-player.XXXXXX")
trap 'rm -rf "$player_build_dir"' EXIT
player_compiler=${CC:-cc}
player_flags=(-std=gnu11 -O1 -g -pthread -DNV_ANDROID
              -Wall -Wextra -Werror=implicit-function-declaration
              -Wno-unused-function -Wno-unused-parameter -Wno-deprecated-declarations
              -ffunction-sections -fdata-sections -I"$player_product_root/src"
              -include "$player_fixture_dir/sdk-extra.h")
player_host=$(uname -s)
if [ "$player_host" = Darwin ]; then
  player_flags+=(-D_DARWIN_C_SOURCE -I/opt/homebrew/include -I/usr/local/include)
  player_linker=(-Wl,-dead_strip)
else
  player_linker=(-Wl,--gc-sections)
fi
if [ -n "${PRELOAD_PLAYER_SDK:-}" ]; then
  player_flags+=(-I"$PRELOAD_PLAYER_SDK")
elif command -v pkg-config >/dev/null 2>&1 && pkg-config --exists sdl2; then
  read -r -a player_sdl_flags <<< "$(pkg-config --cflags sdl2)"
  player_flags+=(${player_sdl_flags[@]+"${player_sdl_flags[@]}"} -I"$(pkg-config --variable=includedir sdl2)")
fi
if [ -n "${CFLAGS:-}" ]; then
  read -r -a player_extra_flags <<< "$CFLAGS"
  player_flags+=(${player_extra_flags[@]+"${player_extra_flags[@]}"})
fi
case "${SANITIZE:-both}" in
  0) player_modes=(0) ;;
  1) player_modes=(1) ;;
  both) player_modes=(0 1) ;;
  *) printf '%s\n' 'SANITIZE must be 0, 1, or both' >&2; exit 2 ;;
esac
for player_mode in "${player_modes[@]}"; do
  player_sanitizers=()
  if [ "$player_mode" = 1 ]; then
    player_sanitizers=(-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all)
    if [ "$player_host" != Darwin ]; then player_sanitizers+=(-fno-pie -no-pie); fi
  fi
  "$player_compiler" "${player_flags[@]}" ${player_sanitizers[@]+"${player_sanitizers[@]}"} \
    "$player_fixture_dir/preload_player.c" "${player_linker[@]}" -lm \
    -o "$player_build_dir/preload-player-$player_mode"
  "$player_build_dir/preload-player-$player_mode"
  printf 'preload-player: %s sanitize=%s PASS\n' "$player_host" "$player_mode"
done
