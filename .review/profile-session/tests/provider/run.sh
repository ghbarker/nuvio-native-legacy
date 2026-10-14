#!/bin/bash
# External review fixtures only; no product-tree mutations. Bash 3.2 compatible.
set -eu
provider_source=${1:-$PWD}
provider_fixtures=$(cd "$(dirname "$0")" && pwd)
provider_build=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-provider-review.XXXXXX")
trap 'rm -rf "$provider_build"' EXIT
cd "$provider_source"
provider_cc=${CC:-cc}
provider_sanitize=()
if [ "${SANITIZE:-0}" = 1 ]; then
  provider_sanitize=(-fsanitize=address,undefined -fno-omit-frame-pointer)
fi
provider_gc=(-Wl,--gc-sections)
if [ "$(uname -s)" = Darwin ]; then provider_gc=(-Wl,-dead_strip); fi
provider_scope_extra=(src/fontefalha.c)
provider_flags=(-std=gnu11 -O1 -g -Isrc -I/opt/homebrew/include -I/opt/homebrew/include/SDL2)
if [ -n "${NUVIO_PROFILE_TEST_SDK:-}" ]; then
  provider_flags+=("-I$NUVIO_PROFILE_TEST_SDK")
  if [ -n "${NUVIO_PROFILE_TEST_SDK_EXTRA:-}" ]; then provider_flags+=("-I$NUVIO_PROFILE_TEST_SDK_EXTRA"); fi
elif command -v sdl2-config >/dev/null; then
  read -r -a provider_sdl_flags <<< "$(sdl2-config --cflags)"
  provider_flags+=("${provider_sdl_flags[@]}")
fi
provider_flags+=(-DNUVIO_PROFILE_NATIVE_FAILOVER)
"$provider_cc" "${provider_flags[@]}" ${provider_sanitize[@]+"${provider_sanitize[@]}"} \
  src/fontefalha.c "$provider_fixtures/bank.c" -o "$provider_build/bank"
ASAN_OPTIONS=${ASAN_OPTIONS:-detect_leaks=0} "$provider_build/bank"
"$provider_cc" "${provider_flags[@]}" ${provider_sanitize[@]+"${provider_sanitize[@]}"} \
  -ffunction-sections -fdata-sections "$provider_fixtures/scope.c" src/debrid.c ${provider_scope_extra[@]+"${provider_scope_extra[@]}"} \
  "${provider_gc[@]}" -pthread -lm -o "$provider_build/scope"
ASAN_OPTIONS=${ASAN_OPTIONS:-detect_leaks=0} "$provider_build/scope"
"$provider_cc" "${provider_flags[@]}" ${provider_sanitize[@]+"${provider_sanitize[@]}"} \
  src/debrid.c src/js.c src/redeurl.c "$provider_fixtures/ownership.c" \
  -pthread -o "$provider_build/ownership"
ASAN_OPTIONS=${ASAN_OPTIONS:-detect_leaks=0} "$provider_build/ownership"
"$provider_cc" "${provider_flags[@]}" ${provider_sanitize[@]+"${provider_sanitize[@]}"} \
  src/fonteregra.c src/regexjs.c src/dados.c src/js.c "$provider_fixtures/fonteregra_missing.c" \
  -pthread -o "$provider_build/fonteregra"
ASAN_OPTIONS=${ASAN_OPTIONS:-detect_leaks=0} "$provider_build/fonteregra"
