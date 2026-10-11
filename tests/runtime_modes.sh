#!/bin/bash
# Same production screen geometry with Mobile support absent and present.
set -eu
cd "$(dirname "$0")/.."
source tests/native_flags.inc
tmp=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-runtime-modes.XXXXXX")
trap 'rm -rf "$tmp"' EXIT
for support in off on; do
  flags=("${nv_test_cflags[@]}")
  if [ "$support" = on ]; then flags+=(-DNV_TOUCH_UI); else flags+=(-UNV_TOUCH_UI); fi
  cc "${flags[@]}" tests/interface_layout.c src/layout.c -o "$tmp/layout-$support"
  "$tmp/layout-$support"
done
for screen in agenda busca; do
  for support in off on; do
    flags=("${nv_test_cflags[@]}")
    if [ "$support" = on ]; then flags+=(-DNV_TOUCH_UI); else flags+=(-UNV_TOUCH_UI); fi
    cc "${flags[@]}" "tests/${screen}_runtime.c" src/layout.c \
      "${nv_test_libs[@]}" -o "$tmp/$screen-$support"
    "$tmp/$screen-$support" > "$tmp/$screen-$support.txt"
  done
  # Every TV return after a Mobile run must match the support-OFF baseline.
  while IFS= read -r state; do
    [ "$state" = "$(cat "$tmp/$screen-off.txt")" ]
  done < "$tmp/$screen-on.txt"
  echo "$screen: support-OFF == support-ON TV; Mobile portrait/16:9/16:10/square and return-to-TV PASS"
done
