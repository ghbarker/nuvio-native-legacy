"""Fresh actual discovery and sync startup contracts on one frozen source layer."""
from pathlib import Path
import argparse
import hashlib
import json
import subprocess

import discovery_contract
import sync_contract

ROOT = Path(__file__).resolve().parent


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
    before = {str(path.relative_to(source)): sha(path) for path in (source / 'src').glob('*.h')}
    before.update({relative: sha(source / relative)
                   for relative in ['src/descoberta.c', 'src/sync.c', 'src/main.c',
                                    'tests/stub_contapend.c', 'tests/syncordem.c']})
    output.mkdir(parents=True)
    components = {}
    source_inputs = {path: value for path, value in before.items() if path.startswith('src/')}
    test_inputs = {path: value for path, value in before.items() if path.startswith('tests/')}
    for name, producer in [('discovery', discovery_contract), ('sync', sync_contract)]:
        directory = output / name
        value = producer.run(source, directory, compiler, args.private, args.sanitize)
        receipt = directory / 'normalized.json'
        receipt.write_text(json.dumps(value, indent=2) + '\n')
        components[name] = {'receipt_relative': str(receipt.relative_to(output)),
                            'receipt_sha256': sha(receipt)}
        source_inputs.update(value['source_inputs_sha256'])
        test_inputs.update(value.get('test_inputs_sha256', {}))
    for relative, expected in {**before, **source_inputs, **test_inputs}.items():
        assert sha(source / relative) == expected, 'Input changed during startup contracts: ' + relative
    assert not git(source, 'status', '--porcelain')
    assert git(source, 'rev-parse', 'HEAD') == commit and git(source, 'rev-parse', 'HEAD^{tree}') == tree
    receipt = {
        'passed': True, 'source_commit': commit, 'source_tree': tree,
        'private': args.private, 'sanitizers': ['address', 'undefined'] if args.sanitize else [],
        'source_clean': True, 'groups': ['discovery', 'sync'], 'components': components,
        'source_inputs_sha256': source_inputs, 'test_inputs_sha256': test_inputs,
        'compiler': str(compiler), 'compiler_sha256': sha(compiler),
        'compiler_version': subprocess.check_output([str(compiler), '--version'], text=True).splitlines()[0],
        'fixture_manifest_sha256': sha(ROOT / 'fixture-manifest.json'),
        'baseline_provenance_sha256': sha(ROOT / 'baseline-evidence.json'),
        'logs_sha256': {str(path.relative_to(output)): sha(path) for path in output.rglob('*.log')},
        'scope': 'Fresh candidate actual discovery/CW/social and sync/main-apply latch assertions; '
                 'typed offline storage/transport boundaries. Baseline reproduction is separately pinned '
                 'local provenance and is not counted as a fresh CI pass.',
    }
    (output / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print('PASS startup concurrency: actual discovery4 + sync3 scenarios')


if __name__ == '__main__':
    main()
