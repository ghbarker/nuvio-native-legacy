#!/usr/bin/env bash
set -euo pipefail
product="${1:-/workspace/nuvio-profile-isolation}"
fixtures="$(cd "$(dirname "$0")" && pwd)"
cd "$product"
build="$(mktemp -d "${TMPDIR:-/tmp}/nuvio-auth-owned.XXXXXX")"
trap 'rm -rf "$build"' EXIT
flags=(-std=gnu11 -O1 -g -ffunction-sections -fdata-sections -Isrc -lpthread -Wall -Wextra -Werror)
case "$(uname -s)" in
  Darwin) flags+=(-I/opt/homebrew/include -I/opt/homebrew/include/SDL2 -Wl,-dead_strip) ;;
  *) flags+=("-I${NV_PROFILE_TEST_SDK:-/workspace/review/runtime-syntax-sdk}" -Wl,--gc-sections) ;;
esac
case "${SANITIZE:-0}" in 1) flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer) ;; esac
cc "${flags[@]}" -DTEST_TRAKT "$fixtures/oauth-account-isolation.c" src/js.c src/jsw.c -o "$build/trakt"
"$build/trakt"
cc "${flags[@]}" "$fixtures/oauth-account-isolation.c" src/js.c src/jsw.c -o "$build/simkl"
"$build/simkl"
