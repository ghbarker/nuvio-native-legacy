#!/bin/bash
# Exact production credential-state block; real mutexes, no SDL or network.
set -eu
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-trakt-lease512.XXXXXX")
trap 'rm -rf "$work"' EXIT
python3 - "$work/lease-actual.inc" <<'PY'
import hashlib,pathlib,sys
p=pathlib.Path('src/trakt.c');text=p.read_text()
a=text.index('static char token[128], cliente[80];')
b=text.index('// Estado da ultima escrita iniciada pelo menu.',a)
block=text[a:b]
for name in ['pedidoCredTravar','pedidoTravar','pedidoDestravar','trakt_historico_aplicar_se_atual']:
 assert name+'(' in block
assert 'rede_' not in block
pathlib.Path(sys.argv[1]).write_text(block)
print('trakt_historico_lease512 source_sha256='+hashlib.sha256(p.read_bytes()).hexdigest()+' block_sha256='+hashlib.sha256(block.encode()).hexdigest(),flush=True)
PY
flags=(-std=gnu11 -O1 -g -Wall -Wextra -Wno-unused-function -Isrc -I"$work" -pthread)
if [ "${SANITIZE:-0}" = 1 ]; then
  flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer)
  if [ "$(uname -s)" = Darwin ]; then
    export ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0:halt_on_error=1}"
  else
    flags+=(-no-pie)
  fi
fi
"${CC:-cc}" "${flags[@]}" tests/trakt_historico_lease512.c -o "$work/test"
"$work/test"
