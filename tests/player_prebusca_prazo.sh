#!/usr/bin/env bash
set -euo pipefail
root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
tmp=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-player-prebusca.XXXXXX")
trap 'rm -rf "$tmp"' EXIT

python3 - "$root" "$tmp" <<'PY'
from pathlib import Path
import re, sys

root, temporary = map(Path, sys.argv[1:])
source = (root / 'src/player.c').read_text()

def closing_brace(start):
    depth, state, index = 0, 'code', start
    while index < len(source):
        char, pair = source[index], source[index:index + 2]
        if state == 'line':
            if char == '\n': state = 'code'
        elif state == 'comment':
            if pair == '*/': state, index = 'code', index + 1
        elif state in ('"', "'"):
            if char == '\\': index += 1
            elif char == state: state = 'code'
        elif pair == '//': state, index = 'line', index + 1
        elif pair == '/*': state, index = 'comment', index + 1
        elif char in ('"', "'"): state = char
        elif char == '{': depth += 1
        elif char == '}':
            depth -= 1
            if depth == 0: return index + 1
        index += 1
    raise AssertionError('Unclosed production block')

function = re.search(r'\bvoid\s+player_atualizar\s*\([^;{}]*\)\s*\{', source)
assert function, 'The actual production player update function is required'
function_end = closing_brace(function.end() - 1)
blocks = list(re.finditer(r'^  if \(prebuscaUrl\[0\]\) \{', source[function.end():function_end], re.M))
assert len(blocks) == 1, 'Select exactly the actual prefetch-release block'
start = function.end() + blocks[0].start()
end = closing_brace(source.index('{', start))
(temporary / 'player_prebusca_prazo.inc').write_text(source[start:end] + '\n')
header = (root / 'src/mkvass.h').read_text()
constant = re.findall(r'^#define\s+MKVASS_PREBUSCA_MS\s+[0-9]+[^\n]*', header, re.M)
assert len(constant) == 1, 'Use the actual production timeout constant'
(temporary / 'player_prebusca_constante.inc').write_text(constant[0] + '\n')
PY

flags=(-std=c99 -O1 -g -Wall -Wextra -Werror)
if [ "${SANITIZE:-0}" = 1 ]; then
  flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer)
  if [ "$(uname -s)" = Linux ]; then
    flags+=(-fno-pie -no-pie)
  fi
fi
"${CC:-cc}" "${flags[@]}" -I"$tmp" "$root/tests/player_prebusca_prazo.c" -o "$tmp/player_prebusca_prazo"
"$tmp/player_prebusca_prazo"
