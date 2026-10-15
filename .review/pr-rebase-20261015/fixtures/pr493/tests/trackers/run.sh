#!/usr/bin/env bash
set -euo pipefail
product="${1:-/workspace/nuvio-profile-isolation}"
fixture="$(cd "$(dirname "$0")" && pwd)"
cd "$product"
build="$(mktemp -d "${TMPDIR:-/tmp}/nuvio-profile-trackers.XXXXXX")"
trap 'rm -rf "$build"' EXIT
flags=(-std=gnu11 -O1 -g -ffunction-sections -fdata-sections -Isrc "-I$fixture" -lpthread)
case "$(uname -s)" in
  Darwin) flags+=(-I/opt/homebrew/include -I/opt/homebrew/include/SDL2 -Wl,-dead_strip) ;;
  *) flags+=("-I${NV_PROFILE_TEST_SDK:-/workspace/review/runtime-syntax-sdk}" -Wl,--gc-sections) ;;
esac
case "${SANITIZE:-0}" in
  1) flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer) ;;
  thread) flags+=(-fsanitize=thread) ;;
esac
cc "${flags[@]}" "$fixture/trakt-isolation.c" src/perfis.c src/traktscrobble.c src/tirarremoto.c src/metaprov.c src/js.c src/jsw.c tests/stub_fichameta.c -o "$build/trakt"
cc "${flags[@]}" "$fixture/simkl-isolation.c" src/perfis.c src/js.c -o "$build/simkl"
cc "${flags[@]}" "$fixture/credfio-isolation.c" src/credfio.c src/perfis.c -o "$build/credfio"
cc "${flags[@]}" "$fixture/visto-isolation.c" src/visto.c src/contapend.c src/simkl.c src/syncprog.c src/progresso.c src/trakt.c tests/stub_fichameta.c src/traktscrobble.c src/cwordem.c src/vistoep.c src/js.c src/metaprov.c src/jsw.c src/perfis.c -o "$build/visto"
cc "${flags[@]}" "$fixture/account-removal.c" src/syncprog.c src/perfis.c src/jsw.c src/js.c -o "$build/account-removal"
cc "${flags[@]}" "$fixture/jellyfin-reports.c" src/js.c src/jsw.c -o "$build/jellyfin-reports"
cc "${flags[@]}" "$fixture/plex-reports.c" src/jellyfin.c src/js.c src/jsw.c -o "$build/plex-reports"
cc "${flags[@]}" "$fixture/sync-progress-isolation.c" src/syncprog.c src/perfis.c src/progresso.c src/jsw.c src/js.c -o "$build/sync-progress"
for name in trakt simkl credfio visto account-removal jellyfin-reports plex-reports sync-progress; do "$build/$name"; done
printf 'All eight tracker/profile integration suites passed (SANITIZE=%s).\n' "${SANITIZE:-0}"
