#!/usr/bin/env python3
"""Run the unchanged key scanner and prove its bounded filesystem matches."""
import argparse
import importlib.util
import json
from pathlib import Path
import re
import subprocess
import sys

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--case', required=True)
parser.add_argument('--output', type=Path, required=True)
args = parser.parse_args()
source = args.source.resolve()
scanner = source / 'tools/varredura-i18n.py'
result = subprocess.run([sys.executable, str(scanner)], cwd=source, text=True, capture_output=True)
args.output.mkdir(parents=True, exist_ok=True)
(args.output / 'translation-key-raw-scan.log').write_text(result.stdout + result.stderr)
print(result.stdout, end='')
spec = importlib.util.spec_from_file_location('original_i18n_key_scanner', scanner)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
missing, format_missing, function_missing = module.varrer()
expected_missing = {}
expected_formats = {}
expected_locations = {}
if args.case == 'combined-delivery':
    expected_missing = {'catalogo-rede.bin.tmp-': {'catalogo.c'}}
    expected_formats = {
        '%s/catalogo-%016llx-p%d.bin': {'catalogo.c'},
        '%s/catalogo-%016llx-p%d.bin.tmp': {'catalogo.c'},
        'catalogo-%016llx-p%d.bin.tmp-': {'catalogo.c'},
        'amigos-vistos-p%d.txt': {'recomenda.c', 'socialvis.c'},
    }
    # These exact statements use the strings as filesystem destinations or
    # filesystem prefixes. No broad ignore regex or UI label exception exists.
    statements = {
        'catalogo.c': [
            'snprintf(dst, tam, "%s/catalogo-%016llx-p%d.bin", dirCache, cacheDonoHash(usuario), perfil);',
            'snprintf(caminho, sizeof caminho, "%s/catalogo-%016llx-p%d.bin.tmp", dirCache, cacheDonoHash(cacheUltimoUsuario), p);',
            'remove(caminho);',
            'snprintf(prefixo, sizeof prefixo, "catalogo-%016llx-p%d.bin.tmp-", cacheDonoHash(cacheUltimoUsuario), p);',
            'foi |= cacheApagarTemporarios(prefixo);',
            'foi |= cacheApagarTemporarios("catalogo-rede.bin.tmp-");',
        ],
        'recomenda.c': [
            'snprintf(nome, sizeof nome, "amigos-vistos-p%d.txt", recPerfil); recApagar(nome);',
        ],
        'socialvis.c': [
            'snprintf(nome, sizeof nome, "amigos-vistos-p%d.txt", perfis_ativo()); return nome;',
            '#define SV_ARQ_VISTOS arquivoVistos()',
        ],
    }
    for filename, required in statements.items():
        original = (source / 'src' / filename).read_text()
        text = re.sub(r'\s+', ' ', original)
        for statement in required:
            assert statement in text, (filename, statement)
            for literal in set(expected_missing) | set(expected_formats):
                if '"' + literal + '"' not in statement:
                    continue
                expression = re.escape(statement).replace(r'\ ', r'\s+')
                occurrences = list(re.finditer(expression, original))
                assert len(occurrences) == 1, (filename, statement, len(occurrences))
                occurrence = occurrences[0]
                offset = occurrence.start() + occurrence.group().index(literal)
                line = original.count('\n', 0, offset) + 1
                expected_locations.setdefault(literal, set()).add(filename + ':' + str(line))
assert set(missing) == set(expected_missing), missing
assert set(format_missing) == set(expected_formats), format_missing
assert not function_missing, function_missing
for observed, expected in ((missing, expected_missing), (format_missing, expected_formats)):
    for literal, locations in observed.items():
        assert {location.split(':')[0] for location in locations} == expected[literal], (literal, locations)
        assert set(locations) == expected_locations[literal], (literal, locations, expected_locations[literal])
assert result.returncode == (1 if expected_missing or expected_formats else 0)
proof = {'original_scanner': 'tools/varredura-i18n.py', 'original_scanner_returncode': result.returncode,
         'untranslated_ui_matches': 0, 'untranslated_format_matches': 0, 'untranslated_function_matches': 0,
         'reviewed_filesystem_matches': {'missing_key': missing, 'format': format_missing},
         'broad_ignore_patterns': False, 'product_scanner_modified': False}
(args.output / 'translation-key-proof.json').write_text(json.dumps(proof, indent=2) + '\n')
print('Original scanner audit PASS: zero UI warnings; exact filesystem-only matches recorded:', len(missing) + len(format_missing))
