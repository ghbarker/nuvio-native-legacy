"""Independent raw-artifact closure for the public owned collection producer."""
from pathlib import Path
import hashlib
import json


def verify(stage, artifact_root, directory, value, plan):
    stage, artifact_root, directory = Path(stage), Path(artifact_root).resolve(), Path(directory)
    files = {}

    def sha(path):
        return hashlib.sha256(Path(path).read_bytes()).hexdigest()

    def child(root, relative):
        relative = Path(relative)
        assert not relative.is_absolute() and '..' not in relative.parts
        return Path(root) / relative

    def checked(path, expected=None):
        path = Path(path)
        assert path.is_file() and not path.is_symlink() and path.resolve().is_relative_to(artifact_root)
        actual = sha(path)
        if expected is not None:
            assert actual == expected
        files[str(path.relative_to(artifact_root))] = actual
        return path

    contract_path = stage / 'fixtures/parallel/contract.json'
    contract = json.loads(contract_path.read_text())
    assert value['contract_sha256'] == sha(contract_path) == plan['parallel_contract_sha256']
    assert len(contract['reader_cases']) == 45 and len(contract['ui_markers']) == 10
    records = value['records']
    assert len(records) == 6
    assert {(r['component'], r['mode']) for r in records} == {(n, m) for n in ['reader', 'ui', 'owned'] for m in ['normal', 'asan']}
    manifest = json.loads((stage / 'source-manifest.json').read_text())
    changed = {entry['path']: entry['sha256'] for entry in manifest['paths']}
    assert {'src/descoberta.c', 'src/descoberta.h', 'src/vertudo.c', 'src/perfis.c'} <= set(value['source_inputs_sha256'])
    for relative, expected in value['source_inputs_sha256'].items():
        assert relative.startswith('src/') and len(expected) == 64
        if relative in changed:
            assert changed[relative] == expected
    for relative, expected in value['fixture_inputs_sha256'].items():
        assert sha(child(stage / 'fixtures', relative)) == expected
    for relative, expected in value['artifacts_sha256'].items():
        checked(child(directory, relative), expected)
    for record in records:
        name, sanitized = record['component'], record['mode'] == 'asan'
        assert record['cases'] == (45 if name == 'reader' else 10 if name == 'ui' else 1)
        assert not any(flag in record['command'] for flag in ['--case', '--baseline', '--expect-bug', '--private', '--matrix', '--extended'])
        checked(child(directory, record['producer_log_relative']), record['producer_log_sha256'])
        rp = checked(child(directory, record['raw_receipt_relative']), record['raw_receipt_sha256'])
        raw = json.loads(rp.read_text())
        if name == 'reader':
            assert raw['all_passed'] is True and raw['expected_cases'] == 45 and raw['source_unchanged'] is True
            assert raw['source_sha256_before'] == raw['source_sha256']
            assert raw['sanitized'] is sanitized and len(raw['records']) == 46
            assert [r['name'] for r in raw['records']] == ['compile', *contract['reader_cases']]
            for r in raw['records']:
                assert r['exit_code'] == 0
                log = checked(child(rp.parent, r['log']), r['log_sha256'])
                if r['name'] != 'compile':
                    text = log.read_text()
                    assert 'PASS ' + r['name'] + ' ' in text
                    assert ('actual-reader-to-legacy-continuation' if r['name'].startswith('manual-') else 'singleton=unchanged') in text
            for relative, expected in raw['source_sha256'].items():
                assert value['source_inputs_sha256'][relative] == expected
            for relative, expected in raw['fixture_sha256'].items():
                assert sha(child(stage / 'fixtures/parallel/reader', relative)) == expected
            checked(rp.parent / 'backend-reader', raw['binary_sha256'])
            commands = [raw['records'][0]['command']]
            assert any(flag.startswith('-DDISCOVERY_SOURCE=') and flag.endswith('/src/descoberta.c"') for flag in commands[0])
        elif name == 'ui':
            assert raw['source_commit'] == plan['source_commit'] and raw['source_status'] == ''
            assert raw['private'] is False and raw['sanitized'] is sanitized
            assert raw['exit_code'] == 0 and raw['abort_traps_fired'] is False
            assert len(raw['runs']) == 10 and {r['case'].removeprefix('v7-o0-z0-r0-') for r in raw['runs']} == set(contract['ui_markers'])
            for r in raw['runs']:
                assert r['exit_code'] == 0 and r['abort_traps_fired'] is False
                log = rp.parent / ('run-' + r['case'] + '.log')
                checked(log, value['artifacts_sha256'][str(log.relative_to(directory))])
                text = log.read_text()
                case = r['case'].removeprefix('v7-o0-z0-r0-')
                assert contract['ui_markers'][case] in text
                if not case.startswith('parallel-manual-'):
                    assert 'PARALLEL_FIRST_THREE_STARTED_WITH_FIRST_BLOCKED calls=3 active=3 max=3' in text
            for relative, expected in raw['tooling_sha256'].items():
                assert sha(child(stage / 'fixtures/async', relative)) == expected
            checked(rp.parent / 'probe', raw['binary_sha256'])
            commands = [c for c in raw['commands'] if '-c' in c]
            assert commands and all('-DNV_ANDROID=1' in c and '-DNV_TOUCH_UI=1' in c for c in commands)
        else:
            assert raw['passed'] is True and raw['expected_bug'] is False and raw['run_exit'] == 0
            assert raw['source_unchanged'] is True and raw['head'] == plan['source_commit']
            assert raw['sanitize'] is sanitized
            assert raw['source_sha256_before'] == raw['source_sha256'] == value['source_inputs_sha256']['src/descoberta.c']
            assert sha(stage / 'fixtures/parallel/owned-url/retained.c') == raw['fixture_sha256']
            for relative, expected in raw['support_sha256'].items():
                assert sha(child(stage / 'fixtures/parallel/owned-support', relative)) == expected
            log = checked(rp.parent / 'run.log', raw['log_sha256'])
            assert contract['owned_marker'] in log.read_text()
            checked(rp.parent / 'retained', raw['binary_sha256'])
            commands = [raw['compile']]
            assert ('-fsanitize-address-use-after-scope' in commands[0]) is sanitized
        for command in commands:
            assert command[0] == value['compiler']
            assert ('-fsanitize=address,undefined' in command) is sanitized
            assert not any('NV_TEST_LOCAL_LOGS' in flag for flag in command)
    return {'reader_cases_per_mode': 45, 'ui_cases_per_mode': 10, 'owned_url_cases_per_mode': 1,
            'modes': ['normal', 'asan'], 'actual_public_reader_ui_owned_url': True}, files
