#!/usr/bin/env bash
set -euo pipefail
fixture=$(cd "$(dirname "$0")" && pwd)
source_root=${1:?source root required}
build=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-owned-mkv.XXXXXX")
trap 'rm -rf "$build"' EXIT
python3 "$fixture/generate.py" "$build"
flags=(-std=gnu11 -O1 -g -pthread -Wall -Wextra -Wno-unused-function -Wno-misleading-indentation
       -DMKVASS_RANGES_POR_SEG=1000 -DMKVASS_PONTOS_INTEIRA=1 -I"$source_root/src")
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
"${CC:-cc}" "${flags[@]}" "$fixture/owned.c" "$source_root/src/legenda.c" "$source_root/src/assrender.c" "$source_root/src/dados.c" -lm -o "$build/test"
mkdir "$build/dados"
NUVIO_DADOS="$build/dados" "$build/test" "$build/current.mkv" "$build/next.mkv"
