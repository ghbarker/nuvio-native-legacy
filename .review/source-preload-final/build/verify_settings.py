#!/usr/bin/env python3
"""Check saved-setting compatibility against a separately declared contract."""
import argparse
import json
from pathlib import Path
import re

def uncomment(value):
    return re.sub(r'/\*.*?\*/|//[^\n]*', '', value, flags=re.S)

def settings(root):
    text = (root / 'src/ajustes.c').read_text()
    enum = re.search(r'typedef enum \{(.*?)\n\} OpcaoId;', text, re.S).group(1)
    ids = [v.strip() for v in uncomment(enum).split(',') if v.strip() and v.strip() != 'AJ_N']
    keys = re.search(r'static const char \*CHAVE\[\] = \{(.*?)\n\};', text, re.S).group(1)
    keys = re.findall(r'"((?:[^"\\]|\\.)*)"', uncomment(keys))
    defaults = uncomment((root / 'src/ajustes_ux_padrao.inc').read_text())
    defaults = [v.strip() for v in defaults.split(',') if v.strip()]
    assert len(ids) == len(keys) == len(defaults)
    return ids, keys, defaults

parser = argparse.ArgumentParser()
parser.add_argument('--baseline', type=Path, required=True)
parser.add_argument('--candidate', type=Path, required=True)
parser.add_argument('--contract', type=Path, required=True)
parser.add_argument('--output', type=Path, required=True)
parser.add_argument('--runtime-fixture', type=Path, required=True)
args = parser.parse_args()
contract = json.loads(args.contract.read_text())
before, after = settings(args.baseline), settings(args.candidate)
assert len(before[0]) == 289
assert after[0][:289] == before[0]
assert after[1][:289] == before[1]
exceptions = contract.get('default_exceptions', {})
for i, (old, new) in enumerate(zip(before[2], after[2])):
    key = before[1][i]
    if key in exceptions:
        allowed = exceptions[key]
        assert old == str(allowed['before']) and new == str(allowed['after']), key
        assert allowed['reason']
    else:
        assert old == new, (key, old, new)
for key in contract['required_keys']:
    assert after[1].count(key) == 1, key
assert contract['required_keys'] and contract['runtime']
assert len(after[0]) == contract['total_count']
assert len(contract['appended']) == len(after[0]) - 289
for rule in contract['appended']:
    index = rule['index']
    assert index >= 289
    assert after[0][index] == rule['id'] and after[1][index] == rule['key']
    assert after[2][index] == str(rule['default'])
options = re.search(r'static const Opcao OPCOES\[AJ_N\]\s*=\s*\{(.*?)\n\};', (args.candidate / 'src/ajustes.c').read_text(), re.S).group(1)
assert len(re.findall(r'^\s*(?:ESC|NUM|LER|ACAO)\(', options, re.M)) == len(after[0])
fixture = ['#include "ajustes.c"', '#include <assert.h>', 'int main(void) {']
for rule in contract['runtime']:
    key = rule['key']
    assert after[1].count(key) == 1
    index = after[1].index(key)
    assert after[2][index] == str(rule['default']), key
    fixture += [
        f'  assert(!strcmp(CHAVE[{index}], {json.dumps(key)}));',
        f'  assert(valor[{index}] == {rule["default"]});',
        f'  assert(somenteDesteAparelho({index}) == {int(rule["device_only"])});',
        f'  assert(perfilLocal({index}) == {int(rule["profile_local"])});',
        f'  assert(dePerfil({index}) == {int(rule["per_profile"])});',
    ]
fixture += ['  puts("Settings runtime classification/default contract PASS");', '  return 0;', '}']
args.runtime_fixture.parent.mkdir(parents=True, exist_ok=True)
args.runtime_fixture.write_text('\n'.join(fixture) + '\n')
proof = {'retained_setting_ids': 289, 'retained_setting_keys': 289,
         'default_changes': exceptions, 'required_keys': contract['required_keys'],
         'runtime_classification_pending_compiler_execution': contract['runtime'],
         'before_count': len(before[0]), 'after_count': len(after[0])}
args.output.write_text(json.dumps(proof, indent=2) + '\n')
print('All289 original setting IDs/keys preserved; declared defaults and new-setting contract checked')
