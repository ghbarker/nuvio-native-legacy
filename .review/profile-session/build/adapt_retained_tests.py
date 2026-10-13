#!/usr/bin/env python3
"""Keep the retained #366 fixture valid after appending one new Settings row.

This changes only the CI working copy. The original feature and fixture remain
unchanged in the retained branch and PR. All existing index/behavior assertions
are preserved; the last-row assertion now checks both append-only additions.
"""
from pathlib import Path

path = Path('.review/hide-watched/tests/settings_contract.c')
text = path.read_text()
old = '  assert(AJ_N == AJ_OCULTAR_FILMES_VISTOS + 1);'
new = ('  assert(AJ_PROVEDORES_ESCOPO == AJ_OCULTAR_FILMES_VISTOS + 1);\n'
       '  assert(AJ_N == AJ_PROVEDORES_ESCOPO + 1);')
assert text.count(old) == 1, path
path.write_text(text.replace(old, new))
print('Retained Settings fixture: original indices preserved; appended provider scope asserted')
