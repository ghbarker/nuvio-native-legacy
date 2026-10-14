#!/usr/bin/env bash
set -euo pipefail
plugins_fixture_dir=$(cd "$(dirname "$0")" && pwd)
if [ "${1:-}" = --root ]; then plugins_product_root=$2; shift 2; else plugins_product_root=${1:-}; [ "$#" -eq 0 ] || shift; fi
if [ -z "$plugins_product_root" ] || [ "$#" -ne 0 ]; then printf '%s\n' 'Usage: plugins_snapshot.sh --root <product-root> (SANITIZE=0|1|both)' >&2; exit 2; fi
plugins_product_root=$(cd "$plugins_product_root" && pwd)
plugins_build_dir=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-plugins-snapshot.XXXXXX")
trap 'rm -rf "$plugins_build_dir"' EXIT
plugins_flags=(-std=gnu11 -O1 -g -pthread -Wall -Wextra -Werror=implicit-function-declaration -Wno-unused-function -Wno-unused-parameter -ffunction-sections -fdata-sections -I"$plugins_product_root/src")
plugins_host=$(uname -s)
if [ "$plugins_host" = Darwin ]; then plugins_flags+=(-D_DARWIN_C_SOURCE -I/opt/homebrew/include -I/usr/local/include); plugins_linker=(-Wl,-dead_strip); else plugins_linker=(-Wl,--gc-sections); fi
if [ -n "${PRELOAD_PLUGINS_SDK:-}" ]; then plugins_flags+=(-I"$PRELOAD_PLUGINS_SDK"); elif command -v pkg-config >/dev/null 2>&1 && pkg-config --exists sdl2; then read -r -a plugins_sdl <<< "$(pkg-config --cflags sdl2)"; plugins_flags+=("${plugins_sdl[@]}" -I"$(pkg-config --variable=includedir sdl2)"); fi
case "${SANITIZE:-both}" in 0) plugins_modes=(0);; 1) plugins_modes=(1);; both) plugins_modes=(0 1);; *) exit 2;; esac
for plugins_session in strong weak; do
for plugins_mode in "${plugins_modes[@]}"; do
  plugins_sanitizers=()
  if [ "$plugins_mode" = 1 ]; then plugins_sanitizers=(-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all); if [ "$plugins_host" != Darwin ]; then plugins_sanitizers+=(-fno-pie -no-pie); fi; fi
  plugins_session_flags=(); if [ "$plugins_session" = weak ]; then plugins_session_flags=(-DPLUGINS_WEAK_SESSION); fi
  "${CC:-cc}" "${plugins_flags[@]}" "${plugins_sanitizers[@]}" "${plugins_session_flags[@]}" "$plugins_fixture_dir/plugins_snapshot.c" "$plugins_product_root/src/js.c" "$plugins_product_root/src/stream_parse.c" "$plugins_product_root/src/badges.c" "${plugins_linker[@]}" -lm -o "$plugins_build_dir/plugins-$plugins_mode"
  "$plugins_build_dir/plugins-$plugins_mode"
  printf 'plugins-snapshot: %s session=%s sanitize=%s PASS\n' "$plugins_host" "$plugins_session" "$plugins_mode"
done
done
