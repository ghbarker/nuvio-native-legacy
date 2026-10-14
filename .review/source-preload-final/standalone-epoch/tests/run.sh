#!/usr/bin/env bash
set -euo pipefail
epoch_test_dir=$(cd "$(dirname "$0")" && pwd)
if [ "${1:-}" = --root ]; then epoch_root=$2; else epoch_root=$1; fi
epoch_build=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-epoch.XXXXXX")
trap 'rm -rf "$epoch_build"' EXIT
epoch_flags=(-std=gnu11 -O1 -g -pthread -Werror=implicit-function-declaration -ffunction-sections -fdata-sections -I"$epoch_root/src")
if [ "$(uname -s)" = Darwin ]; then epoch_link=(-Wl,-dead_strip); else epoch_link=(-Wl,--gc-sections); fi
for epoch_mode in 0 1; do
  epoch_san=(); if [ "$epoch_mode" = 1 ]; then epoch_san=(-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all); if [ "$(uname -s)" != Darwin ]; then epoch_san+=(-fno-pie -no-pie); fi; fi
  "${CC:-cc}" "${epoch_flags[@]}" "${epoch_san[@]}" "$epoch_test_dir/epoch.c" "$epoch_test_dir/session_adapter.c" "$epoch_root/src/js.c" "$epoch_root/src/jsw.c" "${epoch_link[@]}" -o "$epoch_build/epoch-$epoch_mode"
  "$epoch_build/epoch-$epoch_mode"
  printf 'standalone-epoch: sanitize=%s PASS\n' "$epoch_mode"
done
