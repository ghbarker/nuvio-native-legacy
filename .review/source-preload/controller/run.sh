#!/usr/bin/env bash
# Review-only fixture. Actual app/streams/debrid code, offline I/O stubs.
set -eu
preload_source=${1:-/workspace/nuvio-source-preload}
preload_fixtures=$(cd "$(dirname "$0")" && pwd)
preload_build=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-preload-controller.XXXXXX")
trap 'rm -rf "$preload_build"' EXIT
preload_cc=${CC:-cc}
preload_flags=(-std=gnu11 -O1 -g -Wall -Wextra -Wno-unused-function -Wno-unused-parameter
               -Wno-format-truncation -Wno-misleading-indentation -pthread
               -ffunction-sections -fdata-sections -I"$preload_source/src" -I"$preload_fixtures/sdk")
if [ "${SANITIZE:-0}" = 1 ]; then preload_flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie -no-pie); fi
preload_gc=(-Wl,--gc-sections)
if [ "$(uname -s)" = Darwin ]; then preload_gc=(-Wl,-dead_strip); fi
mkdir "$preload_build/data"
"$preload_cc" "${preload_flags[@]}" -DNV_ANDROID -DSOURCE_APP="\"$preload_source/src/app.c\"" \
  "$preload_fixtures/controller.c" "$preload_source/src/fontefalha.c" \
  "$preload_source/src/fonteantecipa.c" "$preload_source/src/inicio.c" "$preload_source/src/fontesonda.c" \
  "${preload_gc[@]}" -lm -o "$preload_build/controller"
"$preload_build/controller" "$preload_build/data"
"$preload_cc" "${preload_flags[@]}" -DNV_LINUX_DESKTOP -DSDLK_i=105 \
  -DSOURCE_STREAMS="\"$preload_source/src/streams.c\"" \
  "$preload_fixtures/worker.c" "$preload_source/src/fontefalha.c" "$preload_source/src/fonteauto.c" \
  "${preload_gc[@]}" -lm -o "$preload_build/worker"
"$preload_build/worker" "$preload_build/data"
"$preload_cc" "${preload_flags[@]}" -DNUVIO_PROFILE_COMBINED \
  "$preload_source/src/debrid.c" "$preload_source/src/js.c" "$preload_source/src/redeurl.c" \
  "$preload_fixtures/credentials.c" "${preload_gc[@]}" -lm -o "$preload_build/credentials"
"$preload_build/credentials"
"$preload_cc" "${preload_flags[@]}" -DNV_ANDROID -DSOURCE_ADDONS="\"$preload_source/src/addons.c\"" \
  "$preload_fixtures/addons_snapshot.c" "$preload_source/src/stream_parse.c" \
  "$preload_source/src/js.c" "$preload_source/src/fontefalha.c" \
  "${preload_gc[@]}" -lm -o "$preload_build/addons"
"$preload_build/addons"
"$preload_cc" "${preload_flags[@]}" -DSOURCE_PREFERENCE="\"$preload_source/src/fontepref.c\"" \
  "$preload_fixtures/preference.c" "${preload_gc[@]}" -lm -o "$preload_build/preference"
"$preload_build/preference"
mkdir "$preload_build/playback-data"
"$preload_cc" "${preload_flags[@]}" -DNV_LINUX_DESKTOP -DSOURCE_APP="\"$preload_source/src/app.c\"" \
  "$preload_fixtures/playback.c" "$preload_source/src/fontefalha.c" \
  "$preload_source/src/fonteantecipa.c" "$preload_source/src/inicio.c" "$preload_source/src/fontesonda.c" \
  "${preload_gc[@]}" -lm -o "$preload_build/playback"
"$preload_build/playback" "$preload_build/playback-data"
