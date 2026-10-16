"""Four genuine native-provider playback scenarios in the unchanged Logs21 APK.

Only remote fixture credentials, media, DNS and trust are synthetic. Provider
ordering, URL resolution, subtitle prefetch, Android PLAY/errors and first frames
all run in the actual installed application. Screenshots require visual review.
"""
from pathlib import Path
import argparse
import base64
from decimal import Decimal
import hashlib
import json
import os
import re
import subprocess
import sys
import time
import traceback
import urllib.request
import zipfile
from android_transport import install_system_ca, append_native_ca, restrict_fixture_egress, verify_system_ca_visible
from jdwp_environment import launch_with_guest_environment
from startup_diagnostics import StartupDiagnostics

PKG = 'space.nuvio.nativelegacy.touch'
SOURCE = '9e55a0cb95010ee25ffb71b81671a596f6451e2e'
APK_SHA = 'd5f84067a06c2c23cfce91358b838ee6f8cbc89feea03444ac372860b43a0771'
VALIDATION_SHA = 'ac9dfdabe67d4977f602b06877be0e9e7aa20765c8ba92a30eab18df2d70a4e8'
CERT = '862d7ef63a0dbde2929f6f231cae295510726d9ac28f9292496d2a38304be5e6'
SCENES = ['first_good', 'second_delayed', 'third_good', 'all_fail']
PIPELINE = re.compile(r'^\[t\] \d+ abrir: url ao pipeline\s*$', re.M)
NATIVE_ERROR = re.compile(r'^\[video\] android evento 5 \(3001, -?\d+\)\s*$', re.M)
JAVA_ERROR = re.compile(r'onPlayerError[^\n]*ERROR_CODE_PARSING_CONTAINER_MALFORMED \(3001\)')

p = argparse.ArgumentParser()
p.add_argument('--apk', type=Path, required=True)
p.add_argument('--validation', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
p.add_argument('--media-dir', type=Path, required=True)
p.add_argument('--cert-dir', type=Path, required=True)
a = p.parse_args()
out = a.output
out.mkdir(parents=True, exist_ok=True)
here = Path(__file__).resolve().parent
events_path = out / 'api-events.jsonl'
diagnostics = StartupDiagnostics(PKG, out / 'startup')
server = server_log = None
receipt = {'execution_passed': False, 'playback_assertions_passed': False,
    'core_playback_assertions_passed': False, 'subtitle_prefetch_subtests_passed': False,
    'source': SOURCE, 'apk_sha256': APK_SHA, 'real_emulator': True,
    'actual_review_commit': os.environ.get('GITHUB_SHA'),
    'api_level': 30, 'requested_abi': 'armeabi-v7a', 'app_rebuilt': False,
    'source_or_player_hooks': False, 'synthetic_offline_profile': True,
    'guest_environment_workaround': 'Exact proven six Activity environment values mirrored through existing guest SDL JNI before native main; debugger detaches',
    'visual_result': 'Root must inspect actual screenshots; execution assertions do not assign visual pass',
    'scenarios': [], 'screenshots': [], 'steps': []}


def run(*command, check=True, data=None, timeout=45):
    got = subprocess.run(command, input=data, stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE, timeout=timeout)
    if check and got.returncode:
        raise RuntimeError('Command failed: ' + str(command[:3]) + ': ' + got.stderr.decode(errors='replace')[-700:])
    return got.stdout


def adb(*args, **kw):
    return run('adb', *args, **kw)


def shell(command, **kw):
    return adb('shell', command, **kw)


def step(name):
    receipt['steps'].append({'name': name, 'wall_time_ns': time.time_ns(), 'monotonic_ns': time.monotonic_ns()})


def wait_for(name, predicate, timeout=60, interval=.25):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if predicate():
            step(name)
            return
        time.sleep(interval)
    raise RuntimeError('Timed out: ' + name)


def native_log():
    # Never count the archived previous-session log.
    return shell('run-as ' + PKG + ' cat files/dados/nuvio.log', check=False).decode(errors='replace')


def java_log():
    return adb('logcat', '-d', '-v', 'epoch', check=False).decode(errors='replace')


def clock_calibration():
    samples = []
    for _ in range(8):
        before = time.time_ns()
        guest = shell('date +%s%N').decode().strip()
        after = time.time_ns()
        assert re.fullmatch(r'[0-9]{18,20}', guest)
        # Add one millisecond for logcat's timestamp granularity.
        samples.append({'host_before_ns': before, 'host_after_ns': after,
            'guest_ns': int(guest), 'offset_lower_ns': int(guest) - after - 1000000,
            'offset_upper_ns': int(guest) - before + 1000000})
    best = min(samples, key=lambda sample: sample['host_after_ns'] - sample['host_before_ns'])
    return {'samples': samples, 'selected': best, 'definition': 'guest Unix ns minus host Unix ns; bounded by complete ADB round trip'}


def foreground_error_times(java):
    return [int(Decimal(match[1]) * 1000000000) for match in re.finditer(
        r'(?m)^[ \t]*([0-9]+\.[0-9]+)\s+[^\n]*onPlayerError[^\n]*ERROR_CODE_PARSING_CONTAINER_MALFORMED \(3001\)', java)]


def clear_generated_catalogue_cache():
    # These are exactly catalogo.c caminhoCache/caminhoCachePerfil outputs.
    # Cold cases keep their ordinary account/preferences but must not reload
    # the previous case's generated catalogue over the newly seeded bundle.
    listing = shell('run-as ' + PKG + ' ls files/dados').decode().splitlines()
    removed = []
    for name in listing:
        if name != 'catalogo-rede.bin' and not re.fullmatch(r'catalogo-[0-9a-f]{16}-p1\.bin', name):
            continue
        relative = 'files/dados/' + name
        data = adb('exec-out', 'run-as', PKG, 'cat', relative)
        shell('run-as ' + PKG + ' rm ' + relative)
        removed.append({'file': relative, 'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()})
    return removed


def clear_generated_case_state(expected_session, ordinary_seeds):
    # These independent playback cases use a synthetic offline account only.
    # Previous successful playback creates Continue Watching; intentional
    # force-stops otherwise accumulate the ordinary startup crash journal.
    assert adb('get-serialno').decode().strip().startswith('emulator-')
    assert shell('getprop ro.kernel.qemu').strip() == b'1'
    assert shell('run-as ' + PKG + ' cat files/dados/perfil.txt') == b'1\n'
    assert shell('run-as ' + PKG + ' cat files/dados/perfis.txt') == b'1\t0\t1\t0\t#3B82F6\tEmulator Test\t\t\n'
    assert shell('run-as ' + PKG + ' cat files/dados/sessao.txt') == expected_session.encode()
    assert shell('run-as ' + PKG + ' cat files/dados/debrid.txt') == ordinary_seeds['debrid.txt'].encode()
    assert ordinary_seeds['debrid.txt'] == 'torbox=fixture_torbox_fake\npremiumize=fixture_premiumize_fake\n'
    # Source9e55: progresso.c ARQ/ARQ_OCULTO, avisos.c AV_MARCA_ARQ,
    # seguro.h SEG_ARQ and dados.c dados_despedida_ler. Never clear crash
    # evidence (queda.txt), accounts, preferences, providers or certificates.
    names = {'progresso.txt', 'cwoculto.txt', 'sessao-viva.txt', 'seguro.txt', 'despedida.txt'}
    listing = set(shell('run-as ' + PKG + ' ls files/dados').decode().splitlines())
    removed = []
    for name in sorted(names & listing):
        relative = 'files/dados/' + name
        data = adb('exec-out', 'run-as', PKG, 'cat', relative)
        shell('run-as ' + PKG + ' rm ' + relative)
        removed.append({'file': relative, 'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()})
    assert not names.intersection(shell('run-as ' + PKG + ' ls files/dados').decode().splitlines())
    return removed


def redact(text):
    return StartupDiagnostics._redact(text)


def fixture_events(scene=None, after=0):
    rows = []
    if events_path.exists():
        for line in events_path.read_text().splitlines():
            try:
                row = json.loads(line)
            except json.JSONDecodeError:
                continue  # Concurrent handler has not finished its line yet.
            if row['sequence'] > after and (scene is None or row.get('scene') == scene):
                rows.append(row)
    return rows


def links(rows):
    return [row for row in rows if row['kind'] == 'native_link_generated']


def route(row):
    return (row['provider'], row['candidate'], row['info_hash'])


def current_counts(native, java):
    return {'play_calls': len(PIPELINE.findall(native)),
            'native_3001_errors': len(NATIVE_ERROR.findall(native)),
            'foreground_3001_errors': len(JAVA_ERROR.findall(java)),
            'native_first_frames': len(re.findall(r'^\[t\] \d+ video: primeiro quadro \(android\)\s*$', native, re.M)),
            'java_first_frames': java.count('onRenderedFirstFrame renderer=video'),
            'native_playing': len(re.findall(r'^\[player\] tocando em ', native, re.M))}


def prefetch_violation(case, code, message):
    # Keep the known, independent subtitle timing failure visible while the
    # unchanged APK's remaining provider routes still receive real playback.
    failures = case['subtitle_prefetch_subtest']['violations']
    if not any(failure['code'] == code for failure in failures):
        failures.append({'code': code, 'message': message, 'wall_time_ns': time.time_ns()})


def write_asset(name, text):
    assert name in ['addons.txt', 'catalogo.txt', 'ids.txt']
    relative = 'files/res/art/' + name
    adb('shell', '-T', 'run-as ' + PKG + " sh -c 'cat > " + relative + "'", data=text.encode())
    actual = adb('exec-out', 'run-as', PKG, 'cat', relative)
    assert actual == text.encode(), 'Ordinary resource seed readback differs: ' + name


def foreground():
    window = shell('dumpsys window').decode(errors='replace')
    activity = shell('dumpsys activity activities').decode(errors='replace')
    (out / 'latest-window.txt').write_text(redact(window))
    (out / 'latest-activity.txt').write_text(redact(activity))
    focused = any(PKG in line for line in window.splitlines() if 'mCurrentFocus=' in line or 'mFocusedApp=' in line)
    resumed = any(PKG in line for line in activity.splitlines() if 'mResumedActivity' in line or 'topResumedActivity' in line)
    assert shell('pidof ' + PKG, check=False).strip() and (focused or resumed), 'Application is not foreground/resumed'


def shot(name):
    foreground()
    data = adb('exec-out', 'screencap', '-p')
    assert data.startswith(b'\x89PNG\r\n\x1a\n')
    path = out / (name + '.png')
    path.write_bytes(data)
    receipt['screenshots'].append({'file': path.name, 'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()})


def key(code):
    shell('input keyevent ' + str(code))
    time.sleep(.75)


def cold_launch(component, label):
    # The helper force-stops before starting, mirrors only six normal values,
    # asserts native main has not started, and detaches the debugger.
    pid = launch_with_guest_environment(PKG, component, out / 'guest-env', label)
    facts = json.loads((out / 'guest-env' / (label + '-jdwp-environment.json')).read_text())
    assert facts['success'] and str(facts['pid']) == pid
    wait_for(label + ': saved synthetic profile restored', lambda: '[sessao] sessao restaurada' in native_log())
    assert 'sessao gravada invalida' not in native_log()
    verify_system_ca_visible(pid, out / 'transport')
    return pid, facts


try:
    apk = a.apk.read_bytes()
    assert len(apk) == 59110404 and hashlib.sha256(apk).hexdigest() == APK_SHA
    validation_bytes = a.validation.read_bytes()
    assert hashlib.sha256(validation_bytes).hexdigest() == VALIDATION_SHA
    validation = json.loads(validation_bytes)
    assert validation['source_commit'] == SOURCE and validation['apk_sha256'] == APK_SHA and validation['all_passed']
    tools = Path(os.environ['ANDROID_HOME']) / 'build-tools/35.0.0'
    signing = run(str(tools / 'apksigner'), 'verify', '--print-certs', str(a.apk)).decode()
    assert 'Signer #1 certificate SHA-256 digest: ' + CERT in signing
    badge = run(str(tools / 'aapt'), 'dump', 'badging', str(a.apk)).decode()
    assert "package: name='" + PKG + "'" in badge and "versionCode='20031'" in badge
    assert "versionName='2.0.31-test-logs21'" in badge
    with zipfile.ZipFile(a.apk) as archive:
        assert {name.split('/')[1] for name in archive.namelist() if name.endswith('/libmain.so')} == {'arm64-v8a', 'armeabi-v7a'}
    assert adb('get-serialno').decode().strip().startswith('emulator-')
    assert shell('getprop ro.kernel.qemu').strip() == b'1'
    properties = {name: shell('getprop ' + name).decode().strip() for name in
                  ['ro.product.cpu.abilist', 'ro.dalvik.vm.native.bridge', 'ro.build.fingerprint', 'ro.build.version.sdk']}
    assert properties['ro.build.version.sdk'] == '30'
    assert 'armeabi-v7a' in properties['ro.product.cpu.abilist'].split(',')
    assert properties['ro.dalvik.vm.native.bridge'] not in ['', '0']
    receipt['emulator_properties'] = properties
    receipt['system_ca'] = install_system_ca(a.cert_dir / 'ca.pem', out / 'transport')
    adb('install', '-r', '--abi', 'armeabi-v7a', str(a.apk), timeout=120)
    package = shell('dumpsys package ' + PKG).decode()
    assert re.search(r'\bprimaryCpuAbi=([^\s]+)', package)[1] == 'armeabi-v7a'
    uid = re.search(r'uid:(\d+)', shell('cmd package list packages -U ' + PKG).decode())[1]
    receipt['actual_abi'] = 'armeabi-v7a'
    receipt['transport'] = restrict_fixture_egress(uid, out / 'transport')
    step('Installed exact APK and restricted both IP families before any synthetic profile/provider seed or app launch')

    server_log = (out / 'fixture-server.log').open('wb')
    server = subprocess.Popen([sys.executable, str(here / 'native_provider_fixture.py'),
        '--bind', '127.0.0.1', '--good', str(a.media_dir / 'healthy-english.mkv'),
        '--bad', str(a.media_dir / 'unsupported-contentcompalgo0.mkv'), '--poster', str(a.media_dir / 'poster.jpg'),
        '--cert', str(a.cert_dir / 'server.pem'), '--key', str(a.cert_dir / 'server.key'), '--events', str(events_path)],
        stdout=server_log, stderr=subprocess.STDOUT)
    contract = {}
    def fixture_ready():
        try:
            with urllib.request.urlopen('http://127.0.0.1:8765/fixture-contract.json', timeout=2) as response:
                contract.update(json.load(response))
            return contract.get('source') == SOURCE
        except (OSError, ValueError):
            return False
    wait_for('Local native API/addon fixture is ready', fixture_ready, timeout=15)
    run(sys.executable, str(here / 'transport_preflight.py'), '--ca', str(a.cert_dir / 'ca.pem'), '--output', str(out / 'transport'))
    (out / 'fixture-contract.json').write_text(json.dumps(contract, indent=2) + '\n')
    configuration = json.loads((here / 'fixture-config.json').read_text())
    assert configuration['source'] == SOURCE
    assert contract == configuration
    assert contract['provider_ids']['torbox'] == 1 and contract['provider_ids']['premiumize'] == 2
    assert contract['provider_budget_total_across_addons'] == 3
    media_receipt = json.loads((out / 'media/media-receipt.json').read_text())
    assert media_receipt['passed'] and media_receipt['ffprobe_ffmpeg_mkvextract_ebml_validations_passed']
    for name in ['healthy-english.mkv', 'unsupported-contentcompalgo0.mkv']:
        assert hashlib.sha256((a.media_dir / name).read_bytes()).hexdigest() == media_receipt['media'][name]['sha256']
    receipt['generated_media_receipt_sha256'] = hashlib.sha256((out / 'media/media-receipt.json').read_bytes()).hexdigest()
    for host in ['api.torbox.app', 'www.premiumize.me']:
        ping = shell('ping -c 1 -W 1 ' + host, check=False).decode(errors='replace')
        assert '10.0.2.2' in ping, 'Guest did not resolve the closed fixture API host'
        (out / 'transport' / (host + '-guest-dns.txt')).write_text(ping)

    shell('am force-stop ' + PKG)
    shell('run-as ' + PKG + ' mkdir -p files/dados')
    payload = base64.urlsafe_b64encode(b'{"sub":"00000000-0000-4000-8000-000000000495","exp":2000000000}').decode().rstrip('=')
    synthetic_session = 'e30.' + payload + '.offline\noffline-refresh\n0\n'
    diagnostics.checked_seed('sessao.txt', synthetic_session)
    diagnostics.checked_seed('perfil.txt', '1\n')
    diagnostics.checked_seed('perfis.txt', '1\t0\t1\t0\t#3B82F6\tEmulator Test\t\t\n')
    diagnostics.checked_seed('ajustes.txt', configuration['ordinary_seeds']['ajustes.txt'])
    marks = ['novidades-guia', 'novidades-11', 'novidades-12', 'novidades-13', 'novidades-131',
        'novidades-132', 'novidades-133', 'novidades-134', 'novidades-139', 'novidades-14-celebracao',
        'novidades-142', 'novidades-150', 'novidades-152-ui', 'novidades-160-ui', 'novidades-170-ui',
        'novidades-172-ui', 'novidades-174-ui', 'salvos-intro', 'aviso-log', 'recintro-social',
        'novidades-180-ui', 'novidades-20-guia', 'novidades-201', 'novidades-202', 'novidades-205', 'trailer-1310']
    for name in marks:
        diagnostics.checked_seed(name + '.txt', '1\n')
    diagnostics.checked_seed('diagnostico-otimizacao-intro.cfg', 'versao=1\n')
    diagnostics.checked_seed('telemetria-perguntado.txt', '0\n')
    diagnostics.checked_seed('pip-modo.txt', '0\n')
    receipt['before_launch_diagnostics'] = diagnostics.snapshot('before-launch')
    shell('wm size 1280x720')
    shell('settings put system accelerometer_rotation 0')
    shell('settings put system user_rotation 0')
    component = shell('cmd package resolve-activity --brief -a android.intent.action.MAIN -c android.intent.category.LAUNCHER ' + PKG).decode().strip().splitlines()[-1]
    assert component.startswith(PKG + '/') and re.fullmatch(r'[A-Za-z0-9_.$/]+', component)
    receipt['bootstrap_pid'], receipt['bootstrap_guest_environment'] = cold_launch(component, 'bootstrap')
    wait_for('Unchanged APK extracted ordinary resources', lambda: bool(shell('run-as ' + PKG + ' cat files/res/.versao', check=False).strip()))
    stamp = shell('run-as ' + PKG + ' cat files/res/.versao')
    shell('am force-stop ' + PKG)
    receipt['native_ca_bundle'] = append_native_ca(a.cert_dir / 'ca.pem', out / 'transport')

    for scene in SCENES:
        plan = configuration['scenes'][scene]
        title = plan['title_id']
        assert title == contract['scenes'][scene]['title_id']
        assert re.fullmatch(r'tt[0-9]+', title)
        shell('am force-stop ' + PKG)
        removed_cache = clear_generated_catalogue_cache()
        for filename, text in configuration['ordinary_seeds'].items():
            diagnostics.checked_seed(filename, text)
        removed_case_state = clear_generated_case_state(synthetic_session, configuration['ordinary_seeds'])
        write_asset('addons.txt', plan['addons_txt'])
        write_asset('catalogo.txt', plan['catalogue_txt'])
        write_asset('ids.txt', plan['ids_txt'])
        assert shell('run-as ' + PKG + ' cat files/res/.versao') == stamp
        baseline = max((event['sequence'] for event in fixture_events()), default=0)
        adb('logcat', '-c')
        pid, guest = cold_launch(component, scene)
        case = {'scene': scene, 'title_id': title, 'pid': pid, 'guest_environment': guest,
                'fixture_sequence_start': baseline, 'passed': False, 'core_playback_passed': False,
                'subtitle_prefetch_subtest': {'required': scene == 'second_delayed',
                    'passed': scene != 'second_delayed', 'violations': []},
                'delayed_prefetch_observations': [],
                'resolver_observations': [], 'removed_generated_catalogue_cache': removed_cache,
                'removed_generated_independent_case_state': removed_case_state,
                'clock_calibration_before_play': clock_calibration()}
        receipt['scenarios'].append(case)
        wait_for(scene + ': real local catalogue loaded', lambda: any(event['kind'] == 'addon_catalog' for event in fixture_events(scene, baseline)))
        time.sleep(2)
        shot(scene + '-01-real-home')
        key(23)
        detail_open = re.compile(r'^\[t\] \d+ detalhe: foco/abertura id=(tt[0-9]+)\b[^\n]*', re.M)
        wait_for(scene + ': intended fixture title actually opened before Play',
                 lambda: bool(detail_open.findall(native_log())) and detail_open.findall(native_log())[-1] == title,
                 timeout=20)
        actual_detail = list(detail_open.finditer(native_log()))[-1]
        assert actual_detail[1] == title
        case['actual_opened_title_id'] = actual_detail[1]
        case['actual_detail_open_log_marker'] = actual_detail[0]
        shot(scene + '-02-real-title')
        key(23)
        wait_for(scene + ': both real addons supplied native candidates', lambda:
                 {event['addon'] for event in fixture_events(scene, baseline) if event['kind'] == 'addon_streams'} == {'a', 'b'})
        expected = [(item['provider'], item['candidate'], item['info_hash']) for item in plan['expected_routes']]
        expected_errors = plan['expected_foreground_errors']

        def playback_ready():
            rows = fixture_events(scene, baseline)
            assert not [event for event in fixture_events(after=baseline) if event['kind'] == 'rejected_request']
            native, java = native_log(), java_log()
            count = current_counts(native, java)
            generated = links(rows)
            assert [route(event) for event in generated] == expected[:len(generated)], scene + ': native resolver skipped or repeated a route'
            for event in generated:
                original_host = 'api.torbox.app' if event['provider'] == 'torbox' else 'www.premiumize.me'
                assert event['request_host'] == original_host and event['tls'] and event['tls_sni'] == original_host
            # A later link may only be requested after the previous actual
            # foreground PLAY has failed. Network probes cannot supply either
            # the pipeline call or Media3 foreground error required here.
            for index in range(len(case['resolver_observations']), len(generated)):
                assert count['play_calls'] >= index
                assert count['foreground_3001_errors'] >= index and count['native_3001_errors'] >= index
                observation = {'event_sequence': generated[index]['sequence'],
                    'route': route(generated[index]), 'monotonic_ns': time.monotonic_ns(), 'counts': count}
                if index:
                    errors = foreground_error_times(java)
                    assert len(errors) >= index
                    offset = case['clock_calibration_before_play']['selected']
                    previous_error_host_latest = errors[index - 1] - offset['offset_lower_ns']
                    assert generated[index]['wall_time_ns'] > previous_error_host_latest, 'Next provider URL was generated before the preceding foreground error was proven'
                    observation.update({'previous_foreground_error_guest_ns': errors[index - 1],
                        'previous_error_host_latest_ns': previous_error_host_latest,
                        'next_resolver_host_ns': generated[index]['wall_time_ns'],
                        'strict_order_proven_with_clock_uncertainty': True})
                case['resolver_observations'].append(observation)
            if scene == 'second_delayed':
                starts = [event for event in rows if event['kind'] == 'media_request' and event.get('header_delay_ms', 0) > 0]
                finished = [event for event in rows if event['kind'] == 'prefetch_header_delay_finished']
                if starts and not finished:
                    checks = {
                        'native_prefetch_started': '[mkvass] pre-busca antes do video' in native,
                        'only_first_pipeline_opened_while_header_pending': count['play_calls'] == 1,
                        'only_first_foreground_error_while_header_pending': count['foreground_3001_errors'] == 1,
                        'only_first_native_error_while_header_pending': count['native_3001_errors'] == 1,
                        'only_expected_two_torbox_links_generated': [route(event) for event in generated] == expected}
                    for code, passed in checks.items():
                        if not passed:
                            prefetch_violation(case, code, 'Held native subtitle-header window did not preserve the expected pre-play state')
                    native_bytes, java_bytes = redact(native).encode(), redact(java).encode()
                    snapshot = scene + '-prefetch-window-' + str(len(case['delayed_prefetch_observations']) + 1)
                    observation = {'wall_time_ns': time.time_ns(), 'monotonic_ns': time.monotonic_ns(),
                        'counts': count, 'generated_routes': [route(event) for event in generated],
                        'delay_start_sequence': starts[-1]['sequence'],
                        'checks': checks, 'all_checks_passed': all(checks.values()),
                        'native_log_file': snapshot + '-native.txt', 'java_log_file': snapshot + '-java.txt',
                        'native_log_sha256': hashlib.sha256(native_bytes).hexdigest(),
                        'java_log_sha256': hashlib.sha256(java_bytes).hexdigest()}
                    case['delayed_prefetch_observations'].append(observation)
                    (out / observation['native_log_file']).write_bytes(native_bytes)
                    (out / observation['java_log_file']).write_bytes(java_bytes)
            if count['java_first_frames'] and count['native_first_frames'] and count['native_playing']:
                assert [route(event) for event in generated] == expected
                assert count['play_calls'] == len(expected)
                assert count['foreground_3001_errors'] == expected_errors and count['native_3001_errors'] == expected_errors
                return True
            return False

        wait_for(scene + ': actual foreground video first frame and playback', playback_ready, timeout=120, interval=.15)
        if scene == 'second_delayed':
            if not case['delayed_prefetch_observations']:
                prefetch_violation(case, 'delayed_header_window_not_observed', 'The native delayed subtitle-prefetch window was not observed')
            if not any(event['kind'] == 'prefetch_header_delay_finished' for event in fixture_events(scene, baseline)):
                prefetch_violation(case, 'delayed_header_not_finished', 'No completion event for the held subtitle header was observed before playback')
            if native_log().count('[player] pre-busca da legenda: video solto') < 2:
                prefetch_violation(case, 'native_prefetch_release_markers_missing', 'Expected native subtitle-prefetch release markers were absent')
            case['subtitle_prefetch_subtest']['passed'] = not case['subtitle_prefetch_subtest']['violations']
        foreground()
        shot(scene + '-03-real-final-video')
        time.sleep(6)
        shot(scene + '-04-real-video-later')
        key(4)
        close_pattern = re.compile(r'\[visto\] encerramento player pos=([0-9.]+) dur=([0-9.]+)[^\n]* id=' + re.escape(title) + r'\b')
        wait_for(scene + ': ordinary Back saved genuine media position', lambda: bool(close_pattern.search(native_log())))
        native, java = native_log(), java_log()
        positions = close_pattern.findall(native)
        position, duration = map(float, positions[-1])
        assert 3 <= position < 120 and 170 <= duration <= 190
        assert shell('pidof ' + PKG).decode().strip() == pid
        rows = fixture_events(scene, baseline)
        assert [route(event) for event in links(rows)] == expected
        for provider, candidate, info_hash in expected:
            assert any(event['kind'] == 'media_request' and event.get('method') == 'GET' and
                route(event) == (provider, candidate, info_hash) for event in rows), 'A resolved route was never read by the actual app'
        assert not [event for event in fixture_events(after=baseline) if event['kind'] == 'rejected_request']
        if scene != 'all_fail':
            assert not [event for event in rows if event.get('provider') == 'premiumize']
        else:
            tb = [event for event in links(rows) if event['provider'] == 'torbox']
            pm = [event for event in links(rows) if event['provider'] == 'premiumize']
            assert len(tb) == 3 and len({event['info_hash'] for event in tb}) == 3 and len(pm) == 1
            assert pm[0]['sequence'] > tb[-1]['sequence']
            assert current_counts(native, java)['foreground_3001_errors'] == 3
        assert 'FATAL EXCEPTION' not in java and not re.search(r'Fatal signal \d+', java + native)
        if expected_errors:
            assert 'ContentCompAlgo 0 not supported' in java + native, 'The actual parser error did not match the generated unsupported-compression fixture'
        case['clock_calibration_after_play'] = clock_calibration()
        first_offset = case['clock_calibration_before_play']['selected']
        last_offset = case['clock_calibration_after_play']['selected']
        assert max(first_offset['offset_lower_ns'], last_offset['offset_lower_ns']) <= min(first_offset['offset_upper_ns'], last_offset['offset_upper_ns']), 'Guest/host clock calibration drifted during the scenario'
        native_bytes, java_bytes = redact(native).encode(), redact(java).encode()
        (out / (scene + '-current-native.txt')).write_bytes(native_bytes)
        (out / (scene + '-current-logcat.txt')).write_bytes(java_bytes)
        (out / (scene + '-events.json')).write_text(json.dumps(rows, indent=2) + '\n')
        case.update({'passed': case['subtitle_prefetch_subtest']['passed'], 'core_playback_passed': True,
            'counts': current_counts(native, java), 'actual_saved_position_seconds': position,
            'actual_duration_seconds': duration, 'native_log_sha256': hashlib.sha256(native_bytes).hexdigest(),
            'logcat_sha256': hashlib.sha256(java_bytes).hexdigest(), 'resolved_routes': [route(event) for event in links(rows)],
            'distinct_final_urls': ['http://10.0.2.2:8765/media/' + scene + '/' + provider + '/' + str(candidate) + '.mkv'
                for provider, candidate, info_hash in expected],
            'http_media_requests_are_only_corroboration': True})
        shot(scene + '-05-after-normal-back')
        step(scene + ': native provider order, actual PLAY/errors, healthy frames and media progress verified')
    assert len(receipt['scenarios']) == 4 and all(case['core_playback_passed'] for case in receipt['scenarios'])
    receipt['core_playback_assertions_passed'] = True
    receipt['subtitle_prefetch_subtests_passed'] = all(case['subtitle_prefetch_subtest']['passed'] for case in receipt['scenarios'])
    if not receipt['subtitle_prefetch_subtests_passed']:
        receipt['failed_subtests'] = [{'scene': case['scene'], 'subtest': 'subtitle_prefetch',
            'violations': case['subtitle_prefetch_subtest']['violations']}
            for case in receipt['scenarios'] if not case['subtitle_prefetch_subtest']['passed']]
        raise AssertionError('Delayed subtitle-prefetch subtest failed; all four core provider playback scenarios completed')
    receipt['playback_assertions_passed'] = receipt['execution_passed'] = True
except Exception as exc:
    frames = traceback.extract_tb(exc.__traceback__)
    last = frames[-1] if frames else None
    receipt['error'] = {'type': type(exc).__name__, 'message': redact(str(exc)),
        'location': {'file': Path(last.filename).name, 'line': last.lineno, 'function': last.name} if last else None,
        'traceback': redact(traceback.format_exc())}
    try:
        shot('failure-real-screen')
    except Exception:
        pass
finally:
    for name, getter in [('final-current-native.txt', native_log), ('final-current-logcat.txt', java_log)]:
        try:
            text = getter()
            (out / name).write_text(redact(text))
            if re.search(r'FATAL EXCEPTION|Fatal signal \d+', text):
                receipt['execution_passed'] = receipt['playback_assertions_passed'] = False
        except Exception as exc:
            receipt[name + '_error'] = str(exc)
    try:
        receipt['final_startup_diagnostics'] = diagnostics.snapshot('finally')
    except Exception as exc:
        receipt['final_startup_diagnostics_error'] = str(exc)
    for tool in ['iptables', 'ip6tables']:
        try:
            (out / (tool + '-after-counters.txt')).write_bytes(shell(tool + ' -vnL OUTPUT'))
        except Exception:
            pass
    if server is not None:
        server.terminate()
        try:
            server.wait(timeout=10)
        except subprocess.TimeoutExpired:
            server.kill()
            server.wait(timeout=5)
    if server_log is not None:
        server_log.close()
    receipt['proof_files_sha256'] = {str(path.relative_to(out)): hashlib.sha256(path.read_bytes()).hexdigest()
        for path in out.rglob('*') if path.is_file() and path.name != 'receipt.json'}
    (out / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps(receipt, indent=2))
if not receipt['execution_passed']:
    raise SystemExit(1)
