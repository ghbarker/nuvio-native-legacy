#!/bin/bash
# Review-only fixture runner: Bash 3.2, macOS and Linux. No live network.
set -euo pipefail
fixture_path="$(cd "$(dirname "$0")" && pwd)"
repo_path="${1:-$PWD}"
sdk_path="${2:-${SDK_INCLUDE:-/opt/homebrew/include}}"
build_dir="$(mktemp -d /tmp/nuvio-profile-ui-tests.XXXXXX)"
trap 'rm -rf "$build_dir"' EXIT
cc_path="${CC:-cc}"
cflags=(-std=c11 -O1 -g -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L -ffunction-sections -fdata-sections "-I$sdk_path" "-I$sdk_path/SDL2" "-I$repo_path/src" "-I$fixture_path")
case "$(uname -s)" in
  Darwin) ldflags=(-Wl,-dead_strip -pthread -lm) ;;
  *) ldflags=(-Wl,--gc-sections -pthread -lm) ;;
esac
if [ "${RUN_ASAN:-0}" = 1 ]; then
  cflags+=(-fsanitize=address,undefined -fno-omit-frame-pointer)
fi
for fixture in home-test library alerts social social-actions social-logout lists-dispatch; do
  extra=()
  case "$fixture" in
    social|social-actions|social-logout) extra=("$repo_path/src/js.c" "$repo_path/src/recresp.c") ;;
    lists-dispatch) extra=("$repo_path/src/js.c") ;;
  esac
  # The guarded expansion works with empty arrays on macOS Bash 3.2.
  "$cc_path" "${cflags[@]}" "$fixture_path/$fixture.c" ${extra[@]+"${extra[@]}"} "${ldflags[@]}" -o "$build_dir/$fixture"
  mkdir "$build_dir/data-$fixture"
  PROFILE_TEST_DATA="$build_dir/data-$fixture" ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}" "$build_dir/$fixture"
done
