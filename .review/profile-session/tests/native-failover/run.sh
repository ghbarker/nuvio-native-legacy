#!/bin/bash
# Offline production ownership regressions; Bash 3.2 compatible.
set -euo pipefail
repo_path="${1:-$PWD}"
fixture_path="$(cd "$(dirname "$0")" && pwd)"
output_path="${NUVIO_PROFILE_RESULTS:-/tmp/nuvio-native-profile-review}/native-failover"
build_dir="$(mktemp -d "${TMPDIR:-/tmp}/nuvio-native-profile.XXXXXX")"
trap 'rm -rf "$build_dir"' EXIT
mkdir -p "$output_path"
cd "$repo_path"
python3 "$fixture_path/prepare-support.py" "$repo_path" "$build_dir/native-worker-support.h"
flags=(-std=gnu11 -O1 -g -D_DEFAULT_SOURCE -ffunction-sections -fdata-sections -Isrc -Itests -I. "-I$build_dir" -DNV_LINUX_DESKTOP)
case "$(uname -s)" in
  Darwin)
    flags+=(-D_DARWIN_C_SOURCE -I/opt/homebrew/include -I/opt/homebrew/include/SDL2)
    linker=(-Wl,-dead_strip -pthread -lm)
    ;;
  *)
    linker=(-Wl,--gc-sections -pthread -lm)
    if [ -n "${NUVIO_PROFILE_TEST_SDK:-}" ]; then
      flags+=("-I$NUVIO_PROFILE_TEST_SDK" -DSDLK_i=105)
    elif command -v sdl2-config >/dev/null; then
      read -r -a sdk_flags <<< "$(sdl2-config --cflags)"
      flags+=("${sdk_flags[@]}")
    fi
    ;;
esac
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
"${CC:-cc}" "${flags[@]}" "$fixture_path/request-ownership.c" \
  src/debrid.c src/perfis.c src/fontefalha.c src/fonteauto.c src/js.c src/redeurl.c \
  "${linker[@]}" -o "$output_path/request-ownership"
ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}" "$output_path/request-ownership" | tee "$output_path/request-ownership.log"
"${CC:-cc}" "${flags[@]}" "$fixture_path/mailbox-ownership.c" \
  src/fontefalha.c src/perfis.c src/fonteantecipa.c src/inicio.c src/fontesonda.c \
  "${linker[@]}" -o "$output_path/mailbox-ownership"
mkdir "$build_dir/policy"
ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}" "$output_path/mailbox-ownership" "$build_dir/policy" | tee "$output_path/mailbox-ownership.log"
"${CC:-cc}" "${flags[@]}" "$fixture_path/provenance.c" \
  src/perfis.c src/js.c src/addonstats.c src/fontefalha.c \
  "${linker[@]}" -o "$output_path/provenance"
ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}" "$output_path/provenance" | tee "$output_path/provenance.log"
