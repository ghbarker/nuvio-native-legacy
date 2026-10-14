#!/usr/bin/env bash
set -euo pipefail
product="${1:-/workspace/nuvio-profile-isolation}"
fixture="$(cd "$(dirname "$0")" && pwd)"
build="$(mktemp -d "${TMPDIR:-/tmp}/nuvio-global-modal.XXXXXX")"
trap 'rm -rf "$build"' EXIT
flags=(-std=gnu11 -O1 -g -ffunction-sections -fdata-sections -Isrc -Itests -DNV_TOUCH_UI -DNV_ANDROID)
cd "$product"
case "$(uname -s)" in
  Darwin) flags+=(-I/opt/homebrew/include -I/opt/homebrew/include/SDL2 -Wl,-dead_strip) ;;
  *) flags+=("-I${NV_PROFILE_TEST_SDK:-/workspace/review/runtime-syntax-sdk}" -Wl,--gc-sections) ;;
esac
case "${SANITIZE:-0}" in 1) flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer) ;; esac
cc "${flags[@]}" "$fixture/global-modal-boundary.c" src/estatvivoui.c src/celbotao.c src/ponteiro.c src/layout.c -lm -pthread -o "$build/modals"
ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}" "$build/modals"
# The integration must close these global consumers in the outgoing hook.
python3 - "$product/src/app.c" <<'PY'
import sys
from pathlib import Path
text = Path(sys.argv[1]).read_text()
start = text.index('static void perfilEncerrarSessao(void) {')
end = text.index('\n}', start)
body = text[start:end]
for call in ('estatvivo_fechar();', 'celb_fechar();'):
    assert call in body, (sys.argv[1], 'missing outgoing global modal close', call)
print('PASS: actual profile outgoing hook contains both public modal closes')
PY
