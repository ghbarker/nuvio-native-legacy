#!/usr/bin/env bash
set -euo pipefail
log_source=${1:?source root}
log_sdk=${2:?declaration SDK}
log_here=$(cd "$(dirname "$0")" && pwd)
log_build=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-test-log-guards.XXXXXX")
trap 'rm -rf "$log_build"' EXIT
log_flags=()
if [ "${SANITIZE:-0}" = 1 ]; then log_flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
log_gc=(-Wl,--gc-sections)
if [ "$(uname -s)" = Darwin ]; then log_gc=(-Wl,-dead_strip); fi
"${CC:-cc}" -std=gnu11 -O1 -g -DNV_ANDROID -DNV_TEST_LOCAL_LOGS=1 \
  -DNV_REC_URL='"https://maintainer.invalid"' -ffunction-sections -fdata-sections \
  -I"$log_source/src" -I"$log_sdk" "$log_here/guard-audit.c" \
  "${log_flags[@]+${log_flags[@]}}" "${log_gc[@]}" -lm -pthread -o "$log_build/audit"
"$log_build/audit"
for log_mode in tv mobile; do
  log_touch=()
  if [ "$log_mode" = mobile ]; then log_touch=(-DNV_TOUCH_UI=1); fi
  "${CC:-cc}" -std=gnu11 -O1 -g -DNV_ANDROID -DNV_TEST_LOCAL_LOGS=1 \
    -ffunction-sections -fdata-sections -I"$log_source/src" -I"$log_sdk" \
    "$log_here/viewer-audit.c" "${log_touch[@]+${log_touch[@]}}" \
    "${log_flags[@]+${log_flags[@]}}" "${log_gc[@]}" -lm -pthread -o "$log_build/viewer-$log_mode"
  "$log_build/viewer-$log_mode"
done
