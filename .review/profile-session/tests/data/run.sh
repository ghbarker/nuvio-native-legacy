#!/bin/bash
# Review-only fixtures; product checkout and upstream tests stay unchanged.
set -euo pipefail
checkout=${1:-$PWD}
fixtures=$(cd "$(dirname "$0")" && pwd)
cd "$checkout"
out=${NUVIO_PROFILE_RESULTS:-/tmp/nuvio-profile-data}/data
mkdir -p "$out"
flags=(-std=gnu11 -D_GNU_SOURCE -O1 -g -ffunction-sections -fdata-sections -pthread -Isrc -Itests -I/opt/homebrew/include -I/opt/homebrew/include/SDL2)
case "$(uname -s)" in Darwin) flags+=(-Wl,-dead_strip);; *) flags+=(-Wl,--gc-sections);; esac
if [ -n "${NUVIO_SYNTAX_SDK:-}" ]; then flags+=(-I"$NUVIO_SYNTAX_SDK"); fi
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
run_test() {
  if "$out/$1" > "$out/$1.log" 2>&1; then
    printf '%s: PASS\n' "$1"
  else
    cat "$out/$1.log"
    return 1
  fi
}
cc "${flags[@]}" src/perfis.c src/catalogo.c src/contalib.c src/vistoep.c src/js.c src/jsw.c "$fixtures/private-state.c" -o "$out/private-state"
run_test private-state
cc "${flags[@]}" "$fixtures/sync-cycle-identity.c" src/perfis.c src/js.c src/jsw.c -o "$out/sync-cycle-identity"
run_test sync-cycle-identity
cc "${flags[@]}" src/perfis.c src/jsw.c src/catalogo.c src/progresso.c src/cwordem.c src/cwretido.c "$fixtures/cw-session-race.c" src/cotacat.c src/js.c src/metaprov.c src/colecoes.c src/redeurl.c src/catordem.c "$fixtures/fixture-neighbours.c" -o "$out/cw-session-race"
run_test cw-session-race
cc "${flags[@]}" src/catalogo.c src/progresso.c src/cwordem.c src/cwretido.c tests/cwremover.c src/cotacat.c src/js.c src/metaprov.c src/colecoes.c src/redeurl.c src/catordem.c "$fixtures/fixture-neighbours.c" "$fixtures/single-session.c" -o "$out/cwremover"
run_test cwremover
for name in herosinopse_corrida localizar_corrida; do
  cc "${flags[@]}" -DNV_CAT_TEST_ANTES_TRAVA=nv_cat_teste_antes_trava src/catalogo.c src/cwordem.c "tests/$name.c" src/cotacat.c src/js.c src/metaprov.c src/colecoes.c src/redeurl.c src/catordem.c src/artefontes.c "$fixtures/single-session.c" -o "$out/$name"
  run_test "$name"
done
