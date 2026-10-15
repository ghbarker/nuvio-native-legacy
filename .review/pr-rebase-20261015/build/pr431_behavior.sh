#!/bin/bash
# The existing actual-source fixtures, split from duplicate full application builds.
set -euo pipefail
source tests/pr431_native_flags.inc
review_temp=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-pr431.XXXXXXXX")
trap 'rm -rf "$review_temp"' EXIT
if [[ "${SANITIZE:-0}" == 1 ]]; then
  nv_test_cflags+=(-fsanitize=address,undefined -fno-omit-frame-pointer)
  nv_test_libs+=(-fsanitize=address,undefined)
fi
for screen in interface_layout agenda_runtime busca_runtime settings_runtime scroll_runtime; do
  for support in off on; do
    if [[ "$screen" == scroll_runtime && "$support" == off ]]; then continue; fi
    flags=("${nv_test_cflags[@]}")
    if [[ "$support" == on ]]; then flags+=(-DNV_TOUCH_UI); else flags+=(-UNV_TOUCH_UI); fi
    companions=()
    if [[ "$screen" == settings_runtime ]]; then companions=(src/fontefalha.c src/linguas.c); fi
    cc "${flags[@]}" "tests/pr431_${screen}.c" src/layout.c ${companions[@]+"${companions[@]}"} \
      "${nv_test_libs[@]}" -o "$review_temp/$screen-$support"
    "$review_temp/$screen-$support" > "$review_temp/$screen-$support.txt"
  done
  if [[ "$screen" == agenda_runtime || "$screen" == busca_runtime || "$screen" == settings_runtime ]]; then
    while IFS= read -r state; do
      [[ "$state" == "$(cat "$review_temp/$screen-off.txt")" ]]
    done < "$review_temp/$screen-on.txt"
  fi
done
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/ajustes.c) continue;; esac
  sources+=("$source")
done
cc "${nv_test_cflags[@]}" -DNV_TOUCH_UI "${sources[@]}" tests/pr431_interface_mode.c \
  "${nv_test_libs[@]}" -o "$review_temp/interface-mode"
mkdir "$review_temp/data"
"$review_temp/interface-mode" "$review_temp/data"
if [[ "${SANITIZE:-0}" == 0 ]]; then
  [[ "$(/bin/bash -c 'printf "%s" "$BASH_VERSION"')" == 3.2.* ]]
  BASH32=/bin/bash python3 tests/pr431_android_ffmpeg_bash.py
  command -v kotlinc >/dev/null
  python3 tests/pr431_android_interface_mode.py
  python3 tests/pr431_window_geometry.py
fi
printf 'PR431 actual screen parity, runtime toggles, persistence and parser PASS (SANITIZE=%s)\n' "${SANITIZE:-0}"
