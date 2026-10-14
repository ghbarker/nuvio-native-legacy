#!/usr/bin/env bash
set -euo pipefail
product="${1:-/workspace/nuvio-profile-isolation}"
fixture="$(cd "$(dirname "$0")" && pwd)"
cd "$product"
build="$(mktemp -d "${TMPDIR:-/tmp}/nuvio-auth-existing.XXXXXX")"
trap 'rm -rf "$build"' EXIT
flags=(-std=gnu11 -O1 -g -ffunction-sections -fdata-sections -Isrc -lpthread)
case "$(uname -s)" in
  Darwin) flags+=(-I/opt/homebrew/include -I/opt/homebrew/include/SDL2 -Wl,-dead_strip) ;;
  *) flags+=("-I${NV_PROFILE_TEST_SDK:-/workspace/review/runtime-syntax-sdk}" -Wl,--gc-sections) ;;
esac
case "${SANITIZE:-0}" in 1) flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer) ;; esac
for name in traktauth_renov traktauth_perfil simklauth_perfil simklauth_poll; do
  case "$name" in traktauth_*) module=traktauth ;; *) module=simklauth ;; esac
  cc "${flags[@]}" "$fixture/legacy-module-mocks.c" -DNV_LEGACY_MODULE_FIXTURE -DNV_LEGACY_AUTH_ACCOUNT -DNV_LEGACY_SCOPED_CREDENTIAL "src/$module.c" src/credfio.c src/js.c src/jsw.c "tests/$name.c" -o "$build/$name"
  "$build/$name"
done
printf 'All four existing OAuth suites passed.\n'
