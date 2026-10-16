#!/bin/bash
# Complete production handoff helpers, with typed platform/media boundaries.
set -eu
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-externo-native.XXXXXX")
trap 'rm -rf "$work"' EXIT
python3 - "$work" <<'PY'
from pathlib import Path
import re,sys
s=Path('src/player.c').read_text()
names=['externoCancelar','externoLiberarRecursos','externoPedir','externoPasso']
blocks=[]
for name in names:
    m=re.search(r'^static (?:void|int) '+name+r'\([^;\n]*\) \{',s,re.M)
    assert m,name
    i=m.end();depth=1;quote=None;line=False;block=False
    while depth:
        c=s[i];n=s[i+1:i+2]
        if line:
            if c=='\n':line=False
        elif block:
            if c=='*' and n=='/':block=False;i+=1
        elif quote:
            if c=='\\':i+=1
            elif c==quote:quote=None
        elif c=='/' and n=='/':line=True;i+=1
        elif c=='/' and n=='*':block=True;i+=1
        elif c in "\"'":quote=c
        elif c=='{':depth+=1
        elif c=='}':depth-=1
        i+=1
    blocks.append('#line '+str(s[:m.start()].count('\n')+1)+' "src/player.c"\n'+s[m.start():i])
Path(sys.argv[1],'playerexterno_native.inc').write_text('\n\n'.join(blocks)+'\n')
PY
flags=()
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
${CC:-cc} tests/playerexterno_integracao.c src/playerexterno.c -std=c11 -O1 -g -Wall -Wextra -Werror \
  -I"$work" -Isrc ${flags[@]+"${flags[@]}"} -lm -o "$work/t"
"$work/t"
