#!/usr/bin/env bash
set -eu
cd "$(dirname "$0")/.."
dir=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-saved-phone-content.XXXXXX")
trap 'rm -rf "$dir"' EXIT
"${PYTHON:-python3}" - "$dir/fonts.txt" <<'PY'
from pathlib import Path
from PIL import ImageFont
import ast
import re
import sys
import unicodedata

source = Path('src/salvospainel.c').read_text(encoding='utf-8')
def plain(s):
    return ''.join(c for c in unicodedata.normalize('NFD', s) if not unicodedata.combining(c)).lower()
prefixes = ('se voce aceitar,', 'eles nao veem', 'se voce recusar,',
            'da para mudar essa resposta', 'seus amigos podem ver',
            'amigos dos seus amigos veem so', 'da para mudar quando quiser',
            'aparecer para outras pessoas?', 'quem ve o que voce assiste?')
literal_tokens = [m.group(0) for m in re.finditer(r'"(?:[^"\\]|\\.)*"|/\*.*?\*/|//[^\n]*', source, re.S)
                  if m.group(0).startswith('"')]
texts = [ast.literal_eval(token) for token in literal_tokens
         if plain(token[1:-1]).startswith(prefixes)]
paragraphs = [s for s in texts if len(s.split()) > 6]
sub = {' '}
for paragraph in paragraphs:
    words = paragraph.split()
    sub.update(' '.join(words[i:j]) for i in range(len(words)) for j in range(i + 1, len(words) + 1))
values = {'Mais antigos', 'Progresso', 'Paisagem', 'Nova categoria', 'Lista', 'Grade',
          'Sim, pode me mostrar', 'Ordenar', 'Agrupar', 'Estilo'}
for array in ('ORDEM_CURTO', 'GRUPO_CURTO', 'ESTILO_CURTO', 'ALC_SEG'):
    body = re.search(r'\b' + array + r'\[[^\]]*\]\s*=\s*\{(.*?)\}', source, re.S).group(1)
    values.update(ast.literal_eval('"' + s + '"') for s in re.findall(r'"([^"\\]*(?:\\.[^"\\]*)*)"', body))
for token in literal_tokens:
    literal = token[1:-1]
    if literal.startswith('N') and 'quero aparecer' in literal:
        values.add(ast.literal_eval('"' + literal + '"'))
sets = (sub, values, {'ORDENAR', 'AGRUPAR', 'ESTILO'}, set(texts) | {'Ag'})
styles = ((19, 'Regular'), (19, 'Bold'), (15, 'Bold'), (24, 'Bold'))
rows = 0
with Path(sys.argv[1]).open('w', encoding='utf-8', newline='\n') as output:
    for family, name in enumerate(('InterDisplay', 'Montserrat')):
        for scale_index, scale in enumerate((1, 1.2, 1.3, 1.5)):
            for style, ((size, weight), labels) in enumerate(zip(styles, sets)):
                font = ImageFont.truetype(f'deploy/app/fonts/{name}-{weight}.ttf', int(size * scale + .5))
                # SDL_ttf raster extents include the final baseline row.
                height = int((sum(font.getmetrics()) + 1) / scale + .5)
                for label in sorted(labels):
                    assert len(label.encode('utf-8')) < 512
                    output.write(f'{family} {scale_index} {style} {font.getlength(label) / scale:.6f} {height}\t{label}\n')
                    rows += 1
assert rows < 20000
PY
flags=(-O2 -g -DNV_LINUX_DESKTOP -D_FORTIFY_SOURCE=3 -Werror=stringop-overflow -Isrc -ffunction-sections -fdata-sections)
if command -v pkg-config >/dev/null && pkg-config --exists sdl2; then
  read -r -a includes <<< "$(pkg-config --cflags sdl2)"
  flags+=("${includes[@]}")
else flags+=(-I/opt/homebrew/include); fi
case "$(uname -s)" in
  Darwin) flags+=(-Wl,-dead_strip);;
  MINGW*|MSYS*) flags+=(-fwhole-program -Wl,--gc-sections);;
  *) flags+=(-Wl,--gc-sections);;
esac
"${CC:-cc}" "${flags[@]}" tests/salvospainel_phone_content.c -lm -o "$dir/teste"
"$dir/teste" "$dir/fonts.txt" "$@"
for module in ctx_inline_phone app_saved_inline; do
  "${CC:-cc}" "${flags[@]}" "tests/$module.c" -lm -o "$dir/$module"
  "$dir/$module"
done
