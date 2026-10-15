#!/bin/bash
# Review-only: actual context/pointer boundary, Bash 3.2 compatible.
set -euo pipefail
fixture_path="$(cd "$(dirname "$0")" && pwd)"
repo_path="${1:-/workspace/nuvio-profile-isolation}"
sdk_path="${2:-${SDK_INCLUDE:-/opt/homebrew/include}}"
build_dir="$(mktemp -d /tmp/nuvio-context-boundary.XXXXXX)"
trap 'rm -rf "$build_dir"' EXIT
cflags=(-std=c11 -O1 -g -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L -ffunction-sections -fdata-sections "-I$sdk_path" "-I$sdk_path/SDL2" "-I$repo_path/src" "-I$repo_path/tests" -DNV_TOUCH_UI -DNV_ANDROID)
case "$(uname -s)" in
  Darwin) cflags+=(-D_DARWIN_C_SOURCE); ldflags=(-Wl,-dead_strip -pthread -lm) ;;
  *) ldflags=(-Wl,--gc-sections -pthread -lm) ;;
esac
if [ "${RUN_ASAN:-0}" = 1 ]; then
  cflags+=(-fsanitize=address,undefined -fno-omit-frame-pointer)
fi
"${CC:-cc}" "${cflags[@]}" "$fixture_path/context-boundary.c" "$repo_path/src/ponteiro.c" "$repo_path/src/layout.c" "${ldflags[@]}" -o "$build_dir/test"
ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}" "$build_dir/test"
python3 - "$repo_path/src/app.c" <<'PY'
import sys
from pathlib import Path
text = Path(sys.argv[1]).read_text()
start = text.index('static void perfilEncerrarSessao(void) {')
end = text.index('\n}', start)
assert 'ctx_abrir_sessao();' in text[start:end], 'missing outgoing context reset hook'
print('PASS: actual outgoing profile hook resets the context UI')
PY
