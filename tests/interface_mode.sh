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
source tests/native_flags.inc
nv_test_cflags+=(-DNV_TOUCH_UI)
cc "${nv_test_cflags[@]}" "${sources[@]}" tests/interface_mode.c "${nv_test_libs[@]}" -o "$test_dir/test"
mkdir "$test_dir/data"
"$test_dir/test" "$test_dir/data"
