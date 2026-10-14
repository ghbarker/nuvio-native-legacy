#!/usr/bin/env python3
import hashlib
import json
import os
from pathlib import Path
import subprocess

output = Path(os.environ['NUVIO_SOURCE_PRELOAD_RESULTS'])
case = json.loads((output / 'case.json').read_text())
assert case['phase'] == 'final'
required = ['translations', 'native-i18n', 'translation-keys', 'home-state']
flags = case['case']
if flags.get('test_local_logs'):
    required += ['local-log-guards', 'local-log-guards-asan']
if flags.get('run_standalone_epoch'):
    required += ['standalone-epoch']
if flags['run_preload']:
    required += ['controller', 'controller-asan', 'player-boundary', 'player-boundary-asan']
if flags.get('run_legacy_mkv'):
    required += ['mkvass_pool_obsoleto', 'mkvass_limites', 'mkvass_limites-asan', 'mkvass_quadro', 'mkvass_quadro-asan', 'mkvass_referencia', 'mkvass_referencia-asan']
if flags.get('run_owned_mkv'):
    required += ['owned-mkv', 'owned-mkv-asan']
if flags.get('run_next_ready'):
    assert flags.get('run_next'), 'Expanded Next suite must retain the original gate labels'
if flags.get('run_original_full_source_asan'):
    required += ['fontepref-original-asan', 'fontepref_file-original-asan', 'player_retido-original-asan']
if flags.get('run_settings_proof'):
    required += ['settings-source', 'settings-runtime']
if flags['run_watched']:
    required += ['watched', 'watched-asan']
if flags.get('run_cache_epoch'):
    required += ['cache-epoch', 'cache-epoch-asan']
if flags.get('run_origin_snapshots'):
    required += ['origin-plugins', 'origin-plugins-asan', 'origin-plex', 'origin-plex-asan']
if flags.get('run_next'):
    required += ['next-controller', 'next-controller-asan']
if flags.get('run_release_date'):
    required += ['release-date', 'release-date-asan']
if flags.get('run_image_download'):
    required += ['image-download']
if flags.get('run_seek_regression'):
    required += ['seek-regression']
if flags.get('run_original_seek_guards'):
    required += ['player_recuo-original', 'posplay_recuo-original',
                 'creditosauto-original', 'creditosauto-original-asan']
if flags.get('run_skip_next'):
    required += ['skip-next-player', 'skip-next-player-asan', 'skip-next-app', 'skip-next-app-asan']
if flags.get('run_screens_watched'):
    required += ['screens-watched', 'screens-watched-asan']
if flags.get('run_profiles'):
    required += ['profile-ui', 'profile-ui-asan', 'profile-data', 'profile-data-asan',
                 'profile-live-tv', 'profile-live-tv-asan']
if flags['run_retained']:
    required += ['stream_parser', 'streamfit_parser', 'fonte_ordem_addon', 'fontecache_vod',
                 'fontefalha', 'fontefalha_worker', 'ajustes_failover']
if flags['run_automatic']:
    required += ['automatic-runtime', 'bash32']
seen = (output / 'results.txt').read_text().splitlines()
assert sorted(seen) == sorted(name + ' PASS' for name in required), (seen, required)
assert (output / 'app-tv').is_file() and (output / 'app-mobile').is_file()
source = json.loads((output.parent / 'source-verified.json').read_text())
assert source['verified'] and source['phase'] == 'final' and source['case'] == flags['id']
image_download_proofs = []
if flags.get('run_image_download'):
    image_report = json.loads((output / 'image-download/report.json').read_text())
    image_runs = image_report['runs']
    assert len(image_runs) == 2 and {run['mode'] for run in image_runs} == {'normal', 'asan'}
    image_source_hash = hashlib.sha256(Path('src/rede.c').read_bytes()).hexdigest()
    for run in image_runs:
        assert Path(run['source_root']).resolve() == Path.cwd().resolve()
        assert run['rede_sha256'] == image_source_hash
        assert run['exit_code'] == run['expected_exit_code'] == 0
        assert len(run['checks']) == 13 and all(check.startswith('CHECK PASS ') for check in run['checks'])
        assert len(set(run['checks'])) == 13
        assert ('-fsanitize=address,undefined' in run['compile_command']) == (run['mode'] == 'asan')
        assert '-DNV_ANDROID=1' in run['compile_command']
        image_download_proofs.append({'mode': run['mode'], 'checks_passed': 13,
            'actual_rede_sha256': image_source_hash,
            'sanitizers': ['address', 'undefined'] if run['mode'] == 'asan' else [],
            'transport_boundaries': ['Android HTTP/JNI', 'curl transport']})
seek_regression_proofs = []
if flags.get('run_seek_regression'):
    seek_report = json.loads((output / 'seek-regression/report.json').read_text())
    assert seek_report['schema'] == 1 and seek_report['passed'] is True
    assert seek_report['baseline_reproduction'] is False
    assert Path(seek_report['source_root']).resolve() == Path.cwd().resolve()
    seek_paths = {'src/player.c', 'src/player.h', 'src/posplay.c', 'src/posplay.h',
                  'src/creditosauto.h', 'src/proximo.c', 'src/proximo.h',
                  'src/proximopreparo.c', 'src/proximopreparo.h',
                  'src/fontefalha.c', 'src/velocidade.c', 'src/relogio.c'}
    seek_hashes = {relative: hashlib.sha256(Path(relative).read_bytes()).hexdigest() for relative in seek_paths}
    assert seek_report['source_hashes'] == seek_hashes
    seek_runs = seek_report['runs']
    assert len(seek_runs) == 6
    assert {(run['case'], run['mode']) for run in seek_runs} == {
        ('seek_gestures', 'normal'), ('seek_gestures', 'asan'),
        ('seek_controller', 'normal'), ('seek_controller', 'asan'),
        ('window_reentry', 'normal'), ('window_reentry', 'asan')}
    for run in seek_runs:
        assert run['passed'] is True
        assert Path(run['source_root']).resolve() == Path.cwd().resolve()
        assert run['source_hashes'] == seek_hashes
        assert run['build_exit_code'] == run['exit_code'] == run['expected_exit_code'] == 0
        assert ('-fsanitize=address,undefined' in run['build_command']) == (run['mode'] == 'asan')
        assert ('-fsanitize=address,undefined' in run['sanitizer_flags']) == (run['mode'] == 'asan')
        assert all(flag in run['build_command'] for flag in run['sanitizer_flags'])
        assert '-DNV_ANDROID' in run['build_command']
        seek_regression_proofs.append({'case': run['case'], 'mode': run['mode'],
            'passed': True, 'actual_source_sha256': seek_hashes,
            'sanitizers': ['address', 'undefined'] if run['mode'] == 'asan' else [],
            'transport_boundaries': ['native video/GFX', 'source/resolver']})
records = [json.loads(path.read_text()) for path in (output.parent / 'compile').glob('*-invocation.json')]
assert records, 'No original/effective compiler flag records'
for record in records:
    assert record['returncode'] == 0, record
    original, effective = record['arguments'], record['effective_arguments']
    assert effective == original + ([record['added_link_rpath']] if record['added_link_rpath'] else [])
    if record['added_link_rpath']:
        assert record['added_link_rpath'].startswith('-Wl,-rpath,')
original_proofs = []
if flags.get('run_original_full_source_asan'):
    sources = {str(path) for path in Path('src').glob('*.c')} | {str(path) for path in Path('src/dts').glob('*.c')}
    for name, omitted in [('fontepref', {'src/main.c', 'src/fontepref.c'}),
                          ('fontepref_file', {'src/main.c', 'src/fontepref.c'}),
                          ('player_retido', {'src/main.c', 'src/player.c', 'src/video.c'})]:
        matches = [record for record in records if 'tests/' + name + '.c' in record['arguments']]
        assert len(matches) == 1, (name, matches)
        record = matches[0]
        assert '-fsanitize=address,undefined' in record['arguments'], name
        compiled = {arg for arg in record['arguments'] if arg in sources}
        assert compiled == sources - omitted, (name, compiled ^ (sources - omitted))
        assert record['added_link_rpath']
        original_proofs.append({'original_script': 'tests/' + name + '.sh',
                                'source_files_compiled': len(compiled),
                                'sanitizers': ['address', 'undefined'],
                                'original_flags_unchanged': True,
                                'only_added_flag': record['added_link_rpath']})
original_seek_proofs = []
if flags.get('run_original_seek_guards'):
    canonical = '17b7769dac4273557e1b07f5fb8734df91dd718f'
    original_bytes = {}
    for name in ('player_recuo', 'posplay_recuo', 'creditosauto'):
        for suffix in ('.c', '.sh'):
            relative = 'tests/' + name + suffix
            expected = subprocess.check_output(['git', 'show', canonical + ':' + relative])
            actual = Path(relative).read_bytes()
            assert actual == expected, ('Original seek guard changed', relative)
            original_bytes[relative] = hashlib.sha256(actual).hexdigest()
    all_sources = {str(path) for path in Path('src').glob('*.c')} | {str(path) for path in Path('src/dts').glob('*.c')}
    for name, omitted in [('player_recuo', {'src/main.c', 'src/player.c'}),
                          ('posplay_recuo', {'src/main.c'})]:
        matches = [record for record in records if 'tests/' + name + '.c' in record['arguments']]
        assert len(matches) == 1, (name, matches)
        record = matches[0]
        assert not any(arg.startswith('-fsanitize=') for arg in record['arguments'])
        compiled = {arg for arg in record['arguments'] if arg in all_sources}
        assert compiled == all_sources - omitted, (name, compiled ^ (all_sources - omitted))
        assert record['added_link_rpath']
        original_seek_proofs.append({'original_script': 'tests/' + name + '.sh',
            'canonical_bytes_verified': {relative: original_bytes[relative] for relative in ('tests/' + name + '.c', 'tests/' + name + '.sh')},
            'source_files_compiled': len(compiled), 'sanitizers': [],
            'original_flags_unchanged': True, 'only_added_flag': record['added_link_rpath']})
    creditos_records = [record for record in records if 'tests/creditosauto.c' in record['arguments']]
    assert len(creditos_records) == 2
    assert sum('-fsanitize=address,undefined' in record['arguments'] for record in creditos_records) == 1
    original_seek_proofs.append({'original_script': 'tests/creditosauto.sh',
        'canonical_bytes_verified': {relative: original_bytes[relative] for relative in ('tests/creditosauto.c', 'tests/creditosauto.sh')},
        'normal_and_supported_asan_ubsan': True, 'original_flags_unchanged': True})
import re
translation_log = (output / 'translations.log').read_text()
assert re.search(r'idiomas: ' + str(flags['catalog_entries']) + r' entradas x 28 idiomas .* ok', translation_log)
proof = {'catalog_entries': flags['catalog_entries'], 'catalog_count': 29,
         'original_glyph_checker': 'tools/idiomas.py',
         'original_key_scanner': 'tools/varredura-i18n.py',
         'outcome': 'passed', 'source_commit': source['source_commit'],
         'review_commit': source['build_commit'], 'source_scope': source['scope'], 'case': flags['id'], 'gates': required,
         'full_app_compile_modes': ['TV support off', 'Mobile support on'],
         'actual_original_full_source_asan': original_proofs,
         'actual_original_seek_guards': original_seek_proofs,
         'compiler_invocations_verified': len(records),
         'declaration_only_cpu_fixture_boundaries': ['controller SDK', 'NV_ANDROID player SDK'],
         'same_product_source_reverification_required': True,
         'release_date_regression_modes': ['normal', 'address+undefined'] if flags.get('run_release_date') else [],
         'release_date_actual_product_paths': ['src/descoberta.c', 'src/catalogo.h', 'src/proximo.c', 'src/proximo.h', 'src/posplay.c', 'src/proximopreparo.c'] if flags.get('run_release_date') else [],
         'image_download_regression_modes': ['normal', 'address+undefined'] if flags.get('run_image_download') else [],
         'image_download_actual_product_paths': ['src/rede.c', 'src/redeurl.c'] if flags.get('run_image_download') else [],
         'image_download_actual_regressions': image_download_proofs,
         'seek_regression_modes': ['normal', 'address+undefined'] if flags.get('run_seek_regression') else [],
         'seek_regression_actual_product_paths': ['src/player.c', 'src/player.h', 'src/posplay.c', 'src/posplay.h'] if flags.get('run_seek_regression') else [],
         'seek_regression_actual_regressions': seek_regression_proofs,
         'expanded_next_ready_cases': ['next_preload_controller', 'next_preload_posplay', 'next_ready_posplay_joined', 'next_ready_source_only'] if flags.get('run_next_ready') else [],
         'legacy_mkv_original_api_runs': 7 if flags.get('run_legacy_mkv') else 0,
         'owned_mkv_modes': ['normal', 'address+undefined'] if flags.get('run_owned_mkv') else [],
         'skip_next_modes': ['normal', 'address+undefined'] if flags.get('run_skip_next') else [],
         'skip_next_actual_product_paths': ['src/player.c', 'src/intro.c', 'src/posplay.c', 'src/proximopreparo.c', 'src/app.c'] if flags.get('run_skip_next') else [],
         'review_only_reduced_fixture_adaptations': {
             'stable_epoch_omitted_by_original_subset': ['fonte_ordem_addon', 'fontecache_vod', 'fontefalha_worker'] if flags['run_retained'] else [],
             'source_order_quality_expectations': 'Canonical #205: negatives last within each quality group; addon order and all assertions retained' if flags['run_retained'] else None,
             'original_full_source_asan_scripts_and_source_arguments': 'unchanged',
             'account_transport_ownership_subset': 'Unexpected identity/refusal calls abort; checked sync-cycle remains original' if flags.get('run_profiles') else None,
             'plugin_weak_macos_case': 'Explicit stable-zero compatibility producer; absent producer tested on Linux'
         }}
(output / 'native-gates-verified.json').write_text(json.dumps(proof, indent=2) + '\n')
print('Every declared final native gate passed:', flags['id'], len(required))
