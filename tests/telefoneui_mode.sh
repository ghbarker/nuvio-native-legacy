#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
test_dir=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-telefoneui-mode.XXXXXXXX")
trap 'rm -rf "$test_dir"' EXIT
cc -O1 -g -Isrc -DNV_TOUCH_UI tests/telefoneui_mode.c src/layout.c -lm -o "$test_dir/test"
"$test_dir/test"
