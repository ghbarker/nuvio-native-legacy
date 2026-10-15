#!/usr/bin/env bash
set -euo pipefail
product="${1:-/workspace/nuvio-profile-isolation}"
fixture="$(cd "$(dirname "$0")" && pwd)"
cd "$product"
build="$(mktemp -d "${TMPDIR:-/tmp}/nuvio-tracker-existing.XXXXXX")"
trap 'rm -rf "$build"' EXIT
flags=(-std=gnu11 -O1 -g -ffunction-sections -fdata-sections -Isrc -lpthread)
case "$(uname -s)" in
  Darwin) flags+=(-I/opt/homebrew/include -I/opt/homebrew/include/SDL2 -Wl,-dead_strip) ;;
  *) flags+=("-I${NV_PROFILE_TEST_SDK:-/workspace/review/runtime-syntax-sdk}" -Wl,--gc-sections) ;;
esac
case "${SANITIZE:-0}" in 1) flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer) ;; esac
legacy=("$fixture/legacy-module-mocks.c" -DNV_LEGACY_MODULE_FIXTURE)
for name in trakt_cw_dup trakt_ep_cinemeta trakt_arranque trakt_contracts; do
  cc "${flags[@]}" "${legacy[@]}" src/trakt.c src/traktscrobble.c tests/stub_fichameta.c src/js.c src/metaprov.c "tests/$name.c" -o "$build/$name"
  "$build/$name" > "$build/$name.txt"
  tail -1 "$build/$name.txt"
done
COM_CERT=1 "$build/trakt_cw_dup" > /dev/null
SEM_NOTA=1 "$build/trakt_cw_dup" > /dev/null
cc "${flags[@]}" "${legacy[@]}" tests/trakt_filmes_vistos.c src/js.c -o "$build/trakt_filmes_vistos"
"$build/trakt_filmes_vistos"
cc "${flags[@]}" "${legacy[@]}" -DNV_LEGACY_SCOPED_ENRICH src/simkl.c src/js.c tests/simkl_cw.c -o "$build/simkl_cw"
"$build/simkl_cw"
cc "${flags[@]}" "${legacy[@]}" -DNV_LEGACY_SCOPED_CREDENTIAL src/credfio.c tests/credfio.c -o "$build/credfio"
"$build/credfio"
cc "${flags[@]}" "${legacy[@]}" -DNV_LEGACY_ACCOUNT_RPC src/visto.c src/contapend.c src/simkl.c src/syncprog.c src/progresso.c src/trakt.c tests/stub_fichameta.c src/traktscrobble.c src/cwordem.c src/vistoep.c src/js.c src/metaprov.c src/jsw.c tests/visto.c -o "$build/visto"
"$build/visto"
cc "${flags[@]}" "${legacy[@]}" -DNV_LEGACY_REMOVAL src/trakt.c src/tirarremoto.c src/traktscrobble.c tests/stub_fichameta.c src/js.c src/jsw.c src/metaprov.c tests/tirarremoto.c -o "$build/tirarremoto"
"$build/tirarremoto"
cc "${flags[@]}" "${legacy[@]}" -DNV_LEGACY_ACCOUNT_RPC src/syncprog.c src/progresso.c src/js.c src/jsw.c tests/syncprog.c -o "$build/syncprog"
"$build/syncprog"
printf 'All ten existing tracker/credential/watched suites passed.\n'
