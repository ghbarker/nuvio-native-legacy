#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-cache-android.XXXXXX")
trap 'rm -rf "$work"' EXIT
flags=(-std=gnu99 -Wall -Wextra -Werror -O1 -g)
if [ "${SANITIZE:-0}" = 1 ]; then
  flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer)
fi
cc "${flags[@]}" -Isrc tests/cache_android.c src/cachedisco.c -o "$work/test"
"$work/test" "$work"
