#!/usr/bin/env python3
"""Replay defaults assertions with only the stale final-enum expectation adapted."""
import argparse, hashlib, json, os, subprocess
from pathlib import Path

ORIGINAL_C_SHA = '9ac5e166c34c2f5fb7e36da57bd603c3c41668c57aa501bd1cf4a08487efa078'
OLD = 'assert(AJ_N == AJ_PROXIMO_CONTAGEM + 1);'
NEW = 'assert(AJ_N == AJ_FONTE_PRECARREGAR + 1);'
BASE = '3c28c0d73664fe82fabced499bd7171810e0c91b'

def prepare(source, output):
    source = Path(source).resolve(); output = Path(output).resolve(); output.mkdir(parents=True, exist_ok=True)
    fixture = source / 'tests/ajustes_padroes.c'; script = source / 'tests/ajustes_padroes.sh'
    original = fixture.read_text(); shell = script.read_text()
    assert hashlib.sha256(fixture.read_bytes()).hexdigest() == ORIGINAL_C_SHA
    assert fixture.read_bytes() == subprocess.check_output(['git', '-C', str(source), 'show', BASE + ':tests/ajustes_padroes.c'])
    assert script.read_bytes() == subprocess.check_output(['git', '-C', str(source), 'show', BASE + ':tests/ajustes_padroes.sh'])
    assert original.count(OLD) == 1 and original.count('#include "../src/ajustes.c"') == 1
    # Source binding changes include resolution only; every behavioral/default assertion stays byte-identical.
    adapted = original.replace(OLD, NEW, 1).replace('#include "../src/ajustes.c"', '#include "' + str(source / 'src/ajustes.c') + '"', 1)
    assert adapted.replace(NEW, OLD, 1).replace('#include "' + str(source / 'src/ajustes.c') + '"', '#include "../src/ajustes.c"', 1) == original
    cfile = output / 'ajustes_padroes-append-only-expectation.c'; cfile.write_text(adapted)
    workdir = 'cd "$(dirname "$0")/.."'
    assert shell.count(workdir) == 1 and shell.count('tests/ajustes_padroes.c') == 1
    replay = shell.replace(workdir, 'cd "$REVIEW_PRODUCT_ROOT"', 1).replace('tests/ajustes_padroes.c', '"$REVIEW_DEFAULTS_FIXTURE"', 1)
    replayfile = output / 'ajustes_padroes-append-only-expectation.sh'; replayfile.write_text(replay)
    return fixture, script, cfile, replayfile

def verify_known_baseline():
    here = Path(__file__).resolve().parent
    known = json.loads((here / 'baseline-defaults-tail.json').read_text())
    table = here / 'baseline-defaults-table.tsv'
    digest = hashlib.sha256(table.read_bytes()).hexdigest()
    assert known['base'] == BASE and known['candidate'] == '1cd707586a5bc94579904e5601de996e62326067'
    assert known['actual_desktop_settings_tables_byte_identical'] and known['table_sha256'] == digest
    host = json.loads((here / 'host-proof/c-result.json').read_text())
    assert host['outcome'] == 'passed' and host['baseline'] == BASE
    for name in ('baseline-desktop-tv-execute', 'candidate-desktop-tv-execute'):
        record = next(r for r in host['records'] if r['name'] == name)
        assert record['exit_code'] == 0 and record['output_sha256'] == digest
    rows = [line.split('\t') for line in table.read_text().splitlines()]
    assert len(rows) == 290 and rows[-1][0:2] == ['289', 'fontePrecarregarLocal']
    countdown = next(int(row[0]) for row in rows if row[1] == 'proximoContagemLocal')
    assert countdown == 278 and len(rows) != countdown + 1
    assert known['source_original_test_sha256'] == ORIGINAL_C_SHA
    return {'baseline': BASE, 'compiled_baseline_and_candidate_table_sha256': digest,
            'baseline_and_candidate_AJ_N': len(rows), 'stale_original_expected_AJ_N': countdown + 1,
            'actual_last_upstream_key': rows[-1][1], 'original_stale_assertion_holds': False}

def main():
    p = argparse.ArgumentParser(); p.add_argument('--source', type=Path, required=True); p.add_argument('--output', type=Path, required=True); a = p.parse_args()
    baseline_proof = verify_known_baseline()
    fixture, script, cfile, replayfile = prepare(a.source, a.output)
    env = dict(os.environ, REVIEW_PRODUCT_ROOT=str(a.source.resolve()), REVIEW_DEFAULTS_FIXTURE=str(cfile))
    result = subprocess.run(['bash', str(replayfile)], cwd=a.source.resolve(), env=env, timeout=900)
    proof = {'original_fixture': 'tests/ajustes_padroes.c', 'original_fixture_sha256': ORIGINAL_C_SHA,
        'original_script_sha256': hashlib.sha256(script.read_bytes()).hexdigest(), 'adapted_fixture_sha256': hashlib.sha256(cfile.read_bytes()).hexdigest(),
        'replay_script_sha256': hashlib.sha256(replayfile.read_bytes()).hexdigest(), 'baseline': BASE,
        'old_final_enum_expectation': OLD, 'new_final_enum_expectation': NEW,
        'only_assertion_change': 'Final-enum metadata now names actual latest-upstream tail AJ_FONTE_PRECARREGAR; no hardcoded count',
        'behavioral_default_assertions_unchanged': True, 'working_directory_and_include_bound_to_exact_product_source': True,
        'original_unadapted_test_counted_as_pass': False, 'known_baseline_failure_proof': baseline_proof,
        'product_and_original_tests_modified': False, 'exit_code': result.returncode}
    (a.output / 'defaults-adapted.json').write_text(json.dumps(proof, indent=2) + '\n')
    raise SystemExit(result.returncode)

if __name__ == '__main__': main()
