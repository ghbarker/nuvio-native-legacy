#!/usr/bin/env bash
set -euo pipefail
next_fixtures="$(cd "$(dirname "$0")" && pwd)"
next_source="$(cd "$1" && pwd)"
next_output=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-preload-next.XXXXXX")
trap 'rm -rf "$next_output"' EXIT
next_flags=(-std=gnu11 -O1 -g -ffunction-sections -fdata-sections -I"$next_source/src" -I"$next_fixtures/../controller/sdk" -pthread)
next_link=(-lm)
if [[ "$(uname -s)" == Darwin ]]; then next_link+=(-Wl,-dead_strip); else next_link+=(-Wl,--gc-sections); fi
if [[ "${SANITIZE:-0}" == 1 ]]; then next_flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
cc "${next_flags[@]}" "$next_fixtures/tests/next_preload_controller.c" "$next_source/src/fontefalha.c" "${next_link[@]}" -o "$next_output/controller"
"$next_output/controller"
cc "${next_flags[@]}" "$next_fixtures/tests/next_preload_posplay.c" "$next_source/src/proximo.c" "${next_link[@]}" -o "$next_output/posplay"
"$next_output/posplay"
