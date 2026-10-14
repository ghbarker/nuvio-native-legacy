#!/bin/bash
set -eu
tv_fixture_dir=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
tv_repo_dir=${1:-/workspace/nuvio-profile-isolation}
cd "$tv_repo_dir"
mkdir -p "$tv_fixture_dir/build"
flags=(-std=c11 -D_GNU_SOURCE -ffunction-sections -fdata-sections -O1 -g -Isrc -I"$tv_fixture_dir")
if [ "$(uname -s)" = Darwin ]; then
  flags+=(-Wl,-dead_strip -I"$tv_fixture_dir/../sdk" -I/opt/homebrew/include -I/opt/homebrew/include/SDL2)
else
  flags+=(-Wl,--gc-sections -I"$tv_fixture_dir/../sdk")
fi
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
cc "${flags[@]}" src/perfis.c src/xtream.c src/js.c "$tv_fixture_dir/support.c" "$tv_fixture_dir/xtream-ownership.c" -pthread -o "$tv_fixture_dir/build/xtream-ownership"
cc "${flags[@]}" src/perfis.c src/stalker.c src/js.c "$tv_fixture_dir/support.c" "$tv_fixture_dir/stalker-ownership.c" -pthread -o "$tv_fixture_dir/build/stalker-ownership"
cc "${flags[@]}" src/perfis.c "$tv_fixture_dir/support.c" "$tv_fixture_dir/epg-ownership.c" -pthread -lz -o "$tv_fixture_dir/build/epg-ownership"
cc "${flags[@]}" src/perfis.c "$tv_fixture_dir/support.c" "$tv_fixture_dir/xtepg-ownership.c" -pthread -o "$tv_fixture_dir/build/xtepg-ownership"
cc "${flags[@]}" -include "$tv_fixture_dir/sdk-extra.h" src/perfis.c src/lembrete.c src/guiaaddons.c src/js.c "$tv_fixture_dir/support.c" "$tv_fixture_dir/guide-state.c" -pthread -lm -o "$tv_fixture_dir/build/guide-state"
cc "${flags[@]}" src/perfis.c "$tv_fixture_dir/support.c" "$tv_fixture_dir/island-ownership.c" -pthread -lm -o "$tv_fixture_dir/build/island-ownership"
cc "${flags[@]}" src/perfis.c "$tv_fixture_dir/support.c" "$tv_fixture_dir/save-actions.c" -pthread -lm -o "$tv_fixture_dir/build/save-actions"
cc "${flags[@]}" src/perfis.c "$tv_fixture_dir/support.c" "$tv_fixture_dir/signal-reset.c" -pthread -lm -o "$tv_fixture_dir/build/signal-reset"
cc "${flags[@]}" src/perfis.c "$tv_fixture_dir/support.c" "$tv_fixture_dir/agenda-card.c" -pthread -lm -o "$tv_fixture_dir/build/agenda-card"
for tv_case in xtream-ownership stalker-ownership epg-ownership xtepg-ownership guide-state island-ownership save-actions signal-reset agenda-card; do
  python3 -c 'import subprocess,sys; subprocess.run([sys.argv[1]], timeout=20, check=True)' "$tv_fixture_dir/build/$tv_case"
done
