#!/bin/bash
# Real external-player handoff state, independent of Android and SDL.
set -eu
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-playerexterno.XXXXXX")
trap 'rm -rf "$work"' EXIT
flags=()
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
cc tests/playerexterno.c -std=c11 -O1 -g -Wall -Wextra -Werror \
  ${flags[@]+"${flags[@]}"} -lm -o "$work/t"
"$work/t"
