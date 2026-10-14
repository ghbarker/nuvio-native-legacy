#!/bin/bash
# Portable review-only actual Plex query gate. Bash 3.2, macOS + Linux.
set -euo pipefail
fixture_dir="$(cd "$(dirname "$0")" && pwd)"
repo_path="$PWD"
output_path=""
sdk_path="${SDK_INCLUDE:-}"
while [ "$#" -gt 0 ]; do
  case "$1" in
    --root) repo_path="$2"; shift 2 ;;
    --output) output_path="$2"; shift 2 ;;
    --sdk) sdk_path="$2"; shift 2 ;;
    *) echo "unknown argument: $1" >&2; exit 2 ;;
  esac
done
if [ -z "$output_path" ]; then output_path="$(mktemp -d "${TMPDIR:-/tmp}/nuvio-plex-preload.XXXXXX")"; fi
mkdir -p "$output_path"
flags=(-std=gnu11 -O1 -g -Wall -Wextra -Werror=implicit-function-declaration -Werror=incompatible-pointer-types -ffunction-sections -fdata-sections "-I$repo_path/src")
case "$(uname -s)" in
  Darwin) flags+=(-D_DARWIN_C_SOURCE -I/opt/homebrew/include -I/opt/homebrew/include/SDL2); libs=(-Wl,-dead_strip -pthread -lm) ;;
  *) flags+=(-D_DEFAULT_SOURCE); libs=(-Wl,--gc-sections -pthread -lm); if command -v sdl2-config >/dev/null 2>&1; then read -r -a sdl_flags <<< "$(sdl2-config --cflags)"; flags+=("${sdl_flags[@]}"); fi ;;
esac
if [ -n "$sdk_path" ]; then flags+=("-I$sdk_path" "-I$sdk_path/SDL2"); fi
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
"${CC:-cc}" "${flags[@]}" "$fixture_dir/snapshot.c" "$repo_path/src/js.c" "$repo_path/src/jsw.c" "${libs[@]}" -o "$output_path/plex-snapshot" > "$output_path/compile.log" 2>&1 || { cat "$output_path/compile.log"; exit 1; }
ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0:halt_on_error=1}" UBSAN_OPTIONS="${UBSAN_OPTIONS:-halt_on_error=1}" "$output_path/plex-snapshot" > "$output_path/run.log" 2>&1 || { cat "$output_path/run.log"; exit 1; }
cat "$output_path/run.log"
echo 'Plex immutable preparation gate: PASS'
