#!/usr/bin/env python3
"""Reuse upstream worker neighbours, replacing only its mocked native APIs.

The original upstream fixture and its assertions remain untouched. This new
ownership fixture links real streams.c, debrid.c and perfis.c instead.
"""
import re
import sys
from pathlib import Path

source = (Path(sys.argv[1]) / 'tests/fontefalha_worker.c').read_text()
source = source[:source.index('static int foiCancelado(')]
source = source.replace('#include "../src/streams.c"', '#include "streams.c"')
names = ['debrid_ativo', 'debrid_servico_nome', 'debrid_servico_por_nome',
         'debrid_resolver_escolhido', 'debrid_servico_disponivel',
         'debrid_resolver_servico_cache', 'debrid_resolver_cache', 'rede_baixar_st']
for name in names:
    match = re.search(r'^.*?\b' + re.escape(name) + r'\([^;]*?\)\s*\{', source, re.M)
    assert match, ('missing original mock boundary', name)
    start = match.start()
    i = match.end()
    depth = 1
    quoted = None
    escaped = False
    while depth:
        ch = source[i]
        if quoted:
            if escaped:
                escaped = False
            elif ch == '\\':
                escaped = True
            elif ch == quoted:
                quoted = None
        elif ch in ('"', "'"):
            quoted = ch
        elif ch == '{':
            depth += 1
        elif ch == '}':
            depth -= 1
        i += 1
    source = source[:start] + source[i:]
Path(sys.argv[2]).write_text(source)
