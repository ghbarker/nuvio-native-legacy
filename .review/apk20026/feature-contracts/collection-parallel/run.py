"""Run the actual collection scheduler and request-owned reader assertions."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import subprocess
import sys

ROOT = Path(__file__).resolve().parent
FEATURES = ROOT.parent
READERS = ['addon', 'genre', 'received-supplement', 'late-supplement', 'reserve-hidden-off',
           'watched-pages', 'watched-bounded', 'raw-skip', 'duplicate-pages', 'top100', 'top250',
           'tmdb-discover', 'tmdb-list', 'tmdb-collection', 'tmdb-person', 'tmdb-director',
           'tmdb-company', 'tmdb-network', 'trakt', 'trakt-stale', 'trakt-compact-first',
           'trakt-compact-last', 'trakt-compact-both', 'empty-spaces', 'trakt-empty-spaces',
           'null-array', 'object-array', 'string-array', 'number-array', 'truncated-array',
           'trakt-truncated-array', 'empty', 'failure', 'invalid', 'disabled', 'stale-before',
           'stale-during', 'overlap', 'overlap-mixed']
UI = {
    'parallel-order-scroll-tap': 'PASS parallel rows preserve source order artwork scroll and title taps',
    'parallel-close-reopen': 'PASS parallel close reopen rejects old results without exceeding global cap',
    'parallel-profile-aba': 'PASS parallel real profile ABA epoch rejects old results',
    'parallel-source-replaced': 'PASS replaced collection source rejects old owned request result',
    'parallel-addon-disabled': 'PASS disabled addon stops requests and reenable recovers exactly once',
    'parallel-shrink-focused-row': 'PASS shrinking collection clamps focus and rejects removed row target',
}


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def git(source, *args):
    return subprocess.check_output(['git', '-C', str(source), *args], text=True).strip()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ['source', 'output', 'cc']:
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--private', action='store_true')
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    source, output, compiler = args.source.resolve(), args.output.resolve(), args.cc.resolve()
    assert compiler.is_file() and not output.exists()
    assert not git(source, 'status', '--porcelain')
    commit, tree = git(source, 'rev-parse', 'HEAD'), git(source, 'rev-parse', 'HEAD^{tree}')
    before = {str(p.relative_to(source)): sha(p) for p in (source / 'src').glob('*.h')}
    before.update({name: sha(source / name) for name in [
        'src/vertudo.c', 'src/descoberta.c', 'src/perfis.c', 'src/js.c', 'src/redeurl.c',
        'src/metaprov.c', 'src/dados.c', 'src/layout.c', 'src/ponteiro.c', 'src/ctxlista.c']})
    output.mkdir(parents=True)
    env = dict(os.environ, CC=str(compiler))
    modes = (['--private'] if args.private else []) + (['--sanitize'] if args.sanitize else [])
    commands = {
        'reader': [sys.executable, str(ROOT / 'reader/run.py'), '--source', str(source),
                   '--out', str(output / 'reader'), '--sdk',
                   str(FEATURES / 'startup-concurrency/discovery/sdk'), *modes],
        'ui': [sys.executable, str(FEATURES / 'collection-taps/async/run_parallel.py'),
               '--source', str(source), '--output', str(output / 'ui'), '--cc', str(compiler), *modes],
    }
    components = {}
    source_inputs = dict(before)
    fixture_inputs = {}
    for name, command in commands.items():
        producer_command = list(command)
        result = subprocess.run(command, env=env, text=True, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, timeout=600)
        log = output / (name + '-producer.log')
        log.write_text(result.stdout)
        assert result.returncode == 0, result.stdout[-8000:]
        directory = output / name
        receipt = directory / 'receipt.json'
        value = json.loads(receipt.read_text())
        assert value['source'] == str(source)
        assert value['private'] is args.private and value['sanitized'] is args.sanitize
        if name == 'reader':
            assert value['all_passed'] is True
            records = value['records']
            assert [r['name'] for r in records] == ['compile', *READERS]
            for record in records:
                assert record['exit_code'] == 0
                path = directory / record['log']
                assert sha(path) == record['log_sha256']
                if record['name'] != 'compile':
                    text = path.read_text()
                    assert 'PASS ' + record['name'] + ' ' in text and 'singleton=unchanged' in text
            assert sha(directory / 'backend-reader') == value['binary_sha256']
            compile_commands = [records[0]['command']]
            assert any(x.startswith('-DDISCOVERY_SOURCE=') and str(source / 'src/descoberta.c') in x
                       for x in compile_commands[0])
            for relative, expected in value['source_sha256'].items():
                assert sha(source / relative) == expected
                source_inputs[relative] = expected
            for relative, expected in value['fixture_sha256'].items():
                assert sha(ROOT / 'reader' / relative) == expected
        else:
            assert value['exit_code'] == 0 and value['abort_traps_fired'] is False
            assert value['source_commit'] == commit and value['source_status'] == ''
            assert {r['case'].removeprefix('v7-o0-z0-r0-') for r in value['runs']} == set(UI)
            assert len(value['runs']) == 6
            for record in value['runs']:
                assert record['exit_code'] == 0 and record['abort_traps_fired'] is False
                text = (directory / ('run-' + record['case'] + '.log')).read_text()
                case = record['case'].removeprefix('v7-o0-z0-r0-')
                assert UI[case] in text
                assert 'PARALLEL_FIRST_THREE_STARTED_WITH_FIRST_BLOCKED calls=3 active=3 max=3' in text
            assert sha(directory / 'probe') == value['binary_sha256']
            compile_commands = [c for c in value['commands'] if '-c' in c]
            assert compile_commands
            for path, expected in value['dependency_sha256'].items():
                path = Path(path).resolve()
                assert sha(path) == expected
                if path.is_relative_to(source):
                    source_inputs[str(path.relative_to(source))] = expected
                elif path.is_relative_to(FEATURES):
                    fixture_inputs[str(path.relative_to(FEATURES))] = expected
                else:
                    assert path.is_relative_to(output), 'Unbound fixture dependency: ' + str(path)
            for relative, expected in value['tooling_sha256'].items():
                assert sha(FEATURES / 'collection-taps/async' / relative) == expected
        for command in compile_commands:
            assert command[0] == str(compiler)
            assert ('-DNV_TEST_LOCAL_LOGS=1' in command) is args.private
            assert ('-fsanitize=address,undefined' in command) is args.sanitize
        components[name] = {'raw_receipt_relative': str(receipt.relative_to(output)),
                            'raw_receipt_sha256': sha(receipt), 'command': producer_command,
                            'producer_log': log.name, 'producer_log_sha256': sha(log),
                            'cases': len(READERS) if name == 'reader' else len(UI)}
    for relative, expected in {**before, **source_inputs}.items():
        assert sha(source / relative) == expected
    assert not git(source, 'status', '--porcelain')
    assert git(source, 'rev-parse', 'HEAD') == commit and git(source, 'rev-parse', 'HEAD^{tree}') == tree
    artifacts = {str(p.relative_to(output)): sha(p) for p in output.rglob('*')
                 if p.is_file() and p.name != 'receipt.json'}
    # Include both raw receipts in the graph; exclude only the outer receipt being created below.
    artifacts.update({entry['raw_receipt_relative']: entry['raw_receipt_sha256']
                      for entry in components.values()})
    receipt = {'passed': True, 'source_commit': commit, 'source_tree': tree,
               'private': args.private, 'sanitizers': ['address', 'undefined'] if args.sanitize else [],
               'source_clean': True, 'groups': ['reader', 'ui'], 'components': components,
               'reader_cases': READERS, 'ui_cases': list(UI), 'source_inputs_sha256': source_inputs,
               'fixture_inputs_sha256': fixture_inputs,
               'artifacts_sha256': artifacts, 'compiler': str(compiler), 'compiler_sha256': sha(compiler),
               'fixture_manifest_sha256': sha(ROOT / 'fixture-manifest.json'),
               'scope': 'Actual owned discovery reader39 cases and actual collection scheduler/render/pointer6 '
                        'cases with deterministic request latches, real profiles and typed external boundaries.'}
    (output / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print('PASS collection parallelism: actual reader39 + UI6 cases')


if __name__ == '__main__':
    main()
