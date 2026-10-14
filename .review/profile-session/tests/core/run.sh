#!/bin/bash
set -eu
checkout=${1:-$PWD}
fixtures=$(cd "$(dirname "$0")" && pwd)
cd "$checkout"
out=${NUVIO_PROFILE_RESULTS:-/tmp/nuvio-profile-core}/core
mkdir -p "$out"
flags=(-std=gnu11 -O1 -g -ffunction-sections -fdata-sections -pthread -Isrc -Itests)
case "$(uname -s)" in Darwin) flags+=(-Wl,-dead_strip);; *) flags+=(-Wl,--gc-sections);; esac
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
cc "${flags[@]}" src/perfis.c src/js.c src/jsw.c "$fixtures/session.c" -o "$out/session"
"$out/session"
cc "${flags[@]}" src/catalogo.c src/perfis.c src/js.c src/jsw.c "$fixtures/cache.c" -o "$out/cache"
"$out/cache"
cc "${flags[@]}" -DNV_CAT_TEST_CACHE_MEIO=nv_cache_session_mid src/catalogo.c src/perfis.c src/js.c src/jsw.c "$fixtures/cache-session.c" -o "$out/cache-session"
"$out/cache-session"
cc "${flags[@]}" src/perfis.c src/syncprog.c src/progresso.c src/js.c src/jsw.c "$fixtures/progress.c" -o "$out/progress"
"$out/progress"
