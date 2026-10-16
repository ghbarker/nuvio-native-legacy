"""Exact released APK, real local sample playback and ordinary clock Resume.

No source patches, login bypass hook, injected player/card state or fake images.
Screenshots always require a human visual verdict, even when state checks pass.
"""
from pathlib import Path
import argparse
import base64
import hashlib
import json
import os
import re
import subprocess
import sys
import time
import urllib.request
import zipfile
from startup_diagnostics import StartupDiagnostics
from jdwp_environment import launch_with_guest_environment

p = argparse.ArgumentParser()
p.add_argument('--apk', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
p.add_argument('--media', type=Path, required=True)
p.add_argument('--poster', type=Path, required=True)
p.add_argument('--api-level', type=int, choices=[30, 35], required=True)
p.add_argument('--install-abi', choices=['armeabi-v7a', 'arm64-v8a'], required=True)
a = p.parse_args()
out = a.output
out.mkdir(parents=True, exist_ok=True)
PKG = 'space.nuvio.nativelegacy.touch'
TITLE = 'tt99990495'
APK_SHA = '8a826174b2035a65a0c280500af79b2345ac73ef464c5a251dc357b625df7c8c'
CERT = '862d7ef63a0dbde2929f6f231cae295510726d9ac28f9292496d2a38304be5e6'
SOURCE = 'be1183f7dde83d8c8b5b9c0881537d0e4b814f0f'
r = {'execution_passed': False, 'playback_assertions_passed': False,
     'ui_result': 'Actual screenshots require root visual review; no automatic visual pass',
     'source': SOURCE, 'apk_sha256': APK_SHA, 'app_rebuilt': False,
     'real_emulator': True, 'api_level': a.api_level, 'requested_abi': a.install_abi,
     'synthetic_offline_profile': True, 'source_or_player_hooks': False,
     'guest_environment_workaround': 'Six normal Java Activity path/locale values reapplied through existing guest SDL JNI before native main; debugger then detaches',
     'screenshots': [], 'steps': []}
server = None
server_log = None
access = out / 'local-addon-access.jsonl'
diagnostics = StartupDiagnostics(PKG, out / 'startup')

def run(*args, check=True, input=None, timeout=45):
    q = subprocess.run(args, input=input, stdout=subprocess.PIPE,
                       stderr=subprocess.PIPE, timeout=timeout)
    if check and q.returncode:
        raise RuntimeError('Command failed: ' + str(args[:4]) + ' ' +
                           q.stderr.decode(errors='replace')[-1000:])
    return q.stdout

def adb(*args, **kwargs):
    return run('adb', *args, **kwargs)

def shell(text, **kwargs):
    return adb('shell', text, **kwargs)

def step(name):
    r['steps'].append({'name': name, 'time': time.time()})

def wait_for(name, predicate, timeout=60):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if predicate():
            step(name)
            return
        time.sleep(1)
    raise RuntimeError('Timed out: ' + name)

def foreground():
    window = shell('dumpsys window').decode(errors='replace')
    activity = shell('dumpsys activity activities').decode(errors='replace')
    (out / 'latest-window.txt').write_text(diagnostics._redact(window))
    (out / 'latest-activity.txt').write_text(diagnostics._redact(activity))
    focused = any(PKG in line for line in window.splitlines()
                  if 'mCurrentFocus=' in line or 'mFocusedApp=' in line)
    resumed = any(PKG in line for line in activity.splitlines()
                  if 'mResumedActivity' in line or 'topResumedActivity' in line)
    assert shell('pidof ' + PKG, check=False).strip() and (focused or resumed), \
        'App is not the foreground/resumed Android activity; see real Android dumps'

def shot(name, require_foreground=True):
    if require_foreground:
        foreground()
    data = adb('exec-out', 'screencap', '-p')
    assert data.startswith(b'\x89PNG\r\n\x1a\n')
    path = out / (name + '.png')
    path.write_bytes(data)
    r['screenshots'].append({'file': path.name, 'bytes': len(data),
                            'sha256': hashlib.sha256(data).hexdigest()})

def write_app(relative, text, append=False):
    assert re.fullmatch(r'files/(?:dados|res/art)/[A-Za-z0-9_.-]+', relative)
    before = adb('exec-out', 'run-as', PKG, 'cat', relative) if append else b''
    adb('shell', '-T', 'run-as ' + PKG + " sh -c 'cat " + ('>>' if append else '>') +
        ' ' + relative + "'", input=text.encode())
    actual = adb('exec-out', 'run-as', PKG, 'cat', relative)
    assert actual == before + text.encode(), 'File write readback differs: ' + relative

def seed(name, text):
    diagnostics.checked_seed(name, text)

def key(code):
    shell('input keyevent ' + str(code))
    time.sleep(1)

def native_log():
    return shell('run-as ' + PKG + ' cat files/dados/nuvio.log', check=False).decode(errors='replace')

def java_log():
    return adb('logcat', '-d', '-v', 'threadtime', check=False).decode(errors='replace')

def requests():
    if not access.exists():
        return []
    result = []
    for line in access.read_text().splitlines():
        try:
            result.append(json.loads(line))
        except json.JSONDecodeError:
            # A concurrent handler may still be appending the final JSON line.
            continue
    return result

def requested(path):
    return any(x['path'] == path and x['status'] in [200, 206] for x in requests())

def close_positions():
    pattern = r'\[visto\] encerramento player pos=([0-9.]+) dur=([0-9.]+)[^\n]* id=' + TITLE + r'\b'
    return [(float(x), float(y)) for x, y in re.findall(pattern, native_log())]

def redact(text):
    text = re.sub(r'https?://\S+', '<url>', text)
    return re.sub(r'(?i)(bearer\s+)\S+', r'\1<redacted>', text)

try:
    data = a.apk.read_bytes()
    assert len(data) == 59110672 and hashlib.sha256(data).hexdigest() == APK_SHA
    tools = Path(os.environ['ANDROID_HOME']) / 'build-tools/35.0.0'
    sign = run(str(tools / 'apksigner'), 'verify', '--print-certs', str(a.apk)).decode()
    assert 'Signer #1 certificate SHA-256 digest: ' + CERT in sign
    badge = run(str(tools / 'aapt'), 'dump', 'badging', str(a.apk)).decode()
    assert "package: name='" + PKG + "'" in badge and "versionCode='20030'" in badge
    assert "versionName='2.0.30-test-logs20'" in badge
    with zipfile.ZipFile(a.apk) as archive:
        assert {x.split('/')[1] for x in archive.namelist() if x.endswith('/libmain.so')} == {'arm64-v8a', 'armeabi-v7a'}
    assert a.media.is_file() and a.poster.is_file()
    probe = json.loads(run('ffprobe', '-v', 'error', '-show_streams', '-show_format',
                          '-of', 'json', str(a.media)))
    assert 179 <= float(probe['format']['duration']) <= 181
    assert {'h264', 'aac'} <= {x['codec_name'] for x in probe['streams']}
    r['generated_media'] = {'bytes': a.media.stat().st_size,
        'sha256': hashlib.sha256(a.media.read_bytes()).hexdigest(),
        'duration_seconds': float(probe['format']['duration']), 'ffprobe': probe}
    assert adb('get-serialno').decode().strip().startswith('emulator-')
    assert shell('getprop ro.kernel.qemu').decode().strip() == '1'
    props = {k: shell('getprop ' + k).decode().strip() for k in
             ['ro.product.cpu.abilist', 'ro.dalvik.vm.native.bridge', 'ro.build.fingerprint', 'ro.build.version.sdk']}
    r['emulator_properties'] = props
    assert props['ro.build.version.sdk'] == str(a.api_level)
    assert a.install_abi in props['ro.product.cpu.abilist'].split(',')
    assert props['ro.dalvik.vm.native.bridge'] not in ['', '0']
    adb('root')
    adb('wait-for-device')
    assert shell('id -u').decode().strip() == '0'
    adb('install', '-r', '--abi', a.install_abi, str(a.apk), timeout=120)
    package = shell('dumpsys package ' + PKG).decode()
    assert re.search(r'\bprimaryCpuAbi=([^\s]+)', package)[1] == a.install_abi
    uid = re.search(r'uid:(\d+)', shell('cmd package list packages -U ' + PKG).decode())[1]
    r['actual_abi'] = a.install_abi
    r['app_uid'] = int(uid)
    for tool in ['iptables', 'ip6tables']:
        shell(tool + ' -I OUTPUT 1 -m owner --uid-owner ' + uid + ' -j REJECT')
        shell(tool + ' -C OUTPUT -m owner --uid-owner ' + uid + ' -j REJECT')
    step('Blocked both app-UID IP families before any synthetic session or app launch')
    shell('am force-stop ' + PKG)
    shell('run-as ' + PKG + ' mkdir -p files/dados')
    payload = base64.urlsafe_b64encode(b'{"sub":"00000000-0000-4000-8000-000000000495","exp":2000000000}').decode().rstrip('=')
    seed('sessao.txt', 'e30.' + payload + '.offline\noffline-refresh\n0\n')
    seed('perfil.txt', '1\n')
    seed('perfis.txt', '1\t0\t1\t0\t#3B82F6\tEmulator Test\t\t\n')
    seed('ajustes.txt', 'interfaceModoLocal 0\nidioma 1\nidiomaAutoLocal 0\nrelogioTelaLocal 0\ntrailerAuto 1\ntrailerHero 1\nheroSectionEnabled 0\nhomeLayoutLocal 0\nenvioAuto 1\nmenuExplorarLocal 1\nmenuGuiaLocal 1\nmenuAgendaLocal 1\nmenuPerfilLocal 1\n')
    marks = ['novidades-guia', 'novidades-11', 'novidades-12', 'novidades-13', 'novidades-131',
        'novidades-132', 'novidades-133', 'novidades-134', 'novidades-139', 'novidades-14-celebracao',
        'novidades-142', 'novidades-150', 'novidades-152-ui', 'novidades-160-ui', 'novidades-170-ui',
        'novidades-172-ui', 'novidades-174-ui', 'salvos-intro', 'aviso-log', 'recintro-social',
        'novidades-180-ui', 'novidades-20-guia', 'novidades-201', 'novidades-202', 'novidades-205', 'trailer-1310']
    for name in marks:
        seed(name + '.txt', '1\n')
    seed('diagnostico-otimizacao-intro.cfg', 'versao=1\n')
    seed('telemetria-perguntado.txt', '0\n')
    seed('pip-modo.txt', '0\n')
    r['before_launch_diagnostics'] = diagnostics.snapshot('before-launch')
    assert all(x['byte_equal'] for x in r['before_launch_diagnostics']['seed_readbacks'])
    shell('wm size 1280x720')
    shell('settings put system accelerometer_rotation 0')
    shell('settings put system user_rotation 0')
    resolved = shell('cmd package resolve-activity --brief -a android.intent.action.MAIN -c android.intent.category.LAUNCHER ' + PKG).decode().strip().splitlines()[-1]
    assert resolved.startswith(PKG + '/') and resolved.count('/') == 1 and re.fullmatch(r'[A-Za-z0-9_.$/]+', resolved)
    r['launcher'] = resolved
    first_pid = launch_with_guest_environment(PKG, resolved, out / 'guest-env', 'first-launch')
    first_guest = json.loads((out / 'guest-env/first-launch-jdwp-environment.json').read_text())
    assert first_guest['success'] and str(first_guest['pid']) == first_pid
    r['first_launch'] = first_guest['launch']
    r['first_guest_environment'] = first_guest
    wait_for('Ordinary first launch extracted real APK assets', lambda:
             bool(shell('run-as ' + PKG + ' cat files/res/.versao', check=False).strip()))
    version_stamp = shell('run-as ' + PKG + ' cat files/res/.versao')
    time.sleep(15)
    r['first_launch_diagnostics'] = diagnostics.snapshot('after-first-launch')
    assert '[sessao] sessao restaurada' in native_log(), \
        'Synthetic saved session did not restore; inspect seed readbacks/native log before any UI claims'
    phase1_pid = shell('pidof ' + PKG).decode().strip()
    assert phase1_pid
    shot('phase1-01-home-before')
    for _ in range(3):
        key(19)
    shot('phase1-02-home-clock-focus')
    key(23)
    shot('phase1-03-home-clock-ok-panel')
    key(4)
    key(20)
    shot('phase1-04-home-after-return')
    key(21)
    key(20)
    key(23)
    shot('phase1-05-search-before')
    for _ in range(4):
        key(19)
    shot('phase1-06-search-clock-focus')
    key(23)
    shot('phase1-07-search-clock-ok-panel')
    key(4)
    key(20)
    shot('phase1-08-search-after-return')
    key(21)
    key(20)
    key(20)
    key(23)
    shot('phase1-09-settings-before')
    for _ in range(6):
        key(19)
    shot('phase1-10-settings-clock-focus')
    key(23)
    shot('phase1-11-settings-clock-ok-panel')
    key(4)
    key(20)
    shot('phase1-12-settings-after-return')
    assert shell('pidof ' + PKG).decode().strip() == phase1_pid
    r['phase1_remote_inputs_completed'] = True
    step('Captured genuine Home/Search/Settings remote routes in the same emulator boot; visual verdict remains pending')
    shell('am force-stop ' + PKG)
    write_app('files/res/art/addons.txt', 'Local emulator playback fixture\thttp://10.0.2.2:8765/manifest.json\t1\t1\t0\n')
    write_app('files/res/art/catalogo.txt', '00.jpg|poster/00.jpg||Remote resume test sample|Movie  ·  Test|2026  ·  3 min|0|Locally generated emulator video; no account or paid provider.\n')
    write_app('files/res/art/ids.txt', TITLE + '\tmovie\n')
    write_app('files/dados/ajustes.txt', 'saidaPlayerLocal 0\nmanterVideoLocal 0\ncwMinimoLocal 1\nescolherFonteManual 1\nrelogioTelaLocal 0\ntrailerAuto 1\ntrailerHero 1\n', append=True)
    assert shell('run-as ' + PKG + ' cat files/res/.versao') == version_stamp
    r['ordinary_asset_stamp_preserved_sha256'] = hashlib.sha256(version_stamp).hexdigest()
    step('Seeded only ordinary addon/catalogue resources and appended real playback preferences')
    server_log = (out / 'local-addon-server.log').open('wb')
    server = subprocess.Popen([sys.executable, str(Path(__file__).with_name('local_addon.py')),
        '--media', str(a.media), '--poster', str(a.poster), '--access-log', str(access)],
        stdout=server_log, stderr=subprocess.STDOUT)
    def server_ready():
        try:
            with urllib.request.urlopen('http://127.0.0.1:8765/manifest.json', timeout=2) as response:
                return json.load(response)['id'] == 'org.nuvio.emulator.local.resume'
        except (OSError, ValueError):
            return False
    wait_for('Actual local addon is serving the generated video', server_ready, 15)
    shell('iptables -I OUTPUT 1 -m owner --uid-owner ' + uid + ' -d 10.0.2.2 -p tcp --dport 8765 -j ACCEPT')
    shell('iptables -C OUTPUT -m owner --uid-owner ' + uid + ' -d 10.0.2.2 -p tcp --dport 8765 -j ACCEPT')
    for tool in ['iptables', 'ip6tables']:
        text = shell(tool + ' -S OUTPUT').decode()
        (out / (tool + '-before.txt')).write_text(text)
        owner = [line for line in text.splitlines() if '--uid-owner ' + uid in line]
        assert owner and re.search(r'(?:^|\s)-j\s+REJECT(?:\s|$)', owner[-1])
        if tool == 'iptables':
            assert len(owner) == 2 and '-d 10.0.2.2/32 ' in owner[0] and '--dport 8765 ' in owner[0] and owner[0].endswith('-j ACCEPT')
        else:
            assert len(owner) == 1
    r['egress'] = 'Only app UID ->10.0.2.2 TCP8765 allowed; all other IPv4 and all IPv6 rejected'
    adb('logcat', '-c')
    playback_pid = launch_with_guest_environment(PKG, resolved, out / 'guest-env', 'playback-launch')
    playback_guest = json.loads((out / 'guest-env/playback-launch-jdwp-environment.json').read_text())
    assert playback_guest['success'] and str(playback_guest['pid']) == playback_pid
    r['playback_guest_environment'] = playback_guest
    wait_for('Actual app process is alive', lambda: bool(shell('pidof ' + PKG, check=False).strip()))
    pid = shell('pidof ' + PKG).decode().strip()
    assert pid == playback_pid
    r['playback_process_pid'] = pid
    wait_for('App requested real local manifest and catalogue', lambda:
             requested('/manifest.json') and any(x['path'].startswith('/catalog/movie/resume-demo') and x['status'] == 200 for x in requests()))
    time.sleep(3)
    shot('01-real-local-catalogue-home')
    key(23)
    shot('02-real-title-before-play')
    key(23)
    wait_for('Normal source path requested the local stream', lambda: requested('/stream/movie/' + TITLE + '.json'))
    wait_for('Actual Android player rendered its first video frame', lambda:
             requested('/video.mp4') and 'onRenderedFirstFrame renderer=video' in java_log())
    wait_for('Native player reports ordinary active playback', lambda: '[player] tocando em ' in native_log())
    shot('03-actual-video-playing')
    time.sleep(10)
    shot('04-actual-video-after-ten-seconds')
    key(4)
    wait_for('Ordinary Back retained the real video session and published its live card', lambda:
             '[player] sessao pausada pronta para retomar' in native_log() and
             '[ilha] atividade ao vivo: ' + TITLE + ' ' in native_log() and len(close_positions()) >= 1)
    first_position, first_duration = close_positions()[0]
    assert 5 <= first_position < 100 and 170 <= first_duration <= 190
    r['first_saved_position_seconds'] = first_position
    r['actual_duration_seconds'] = first_duration
    shot('05-real-home-retained-video-card')
    key(19)
    key(19)
    shot('06-real-clock-focused-over-retained-card')
    before_resume = native_log().count('[player] retomada pronta:')
    assert before_resume == 0
    key(23)
    shot('07-real-resume-modal')
    assert native_log().count('[player] retomada pronta:') == before_resume, 'Clock activation unexpectedly played instead of opening card controls'
    key(23)
    wait_for('Second short OK resumed the actual retained player', lambda:
             native_log().count('[player] retomada pronta:') == before_resume + 1)
    shot('08-actual-video-resumed')
    time.sleep(6)
    shot('09-actual-video-after-resume-interval')
    key(4)
    wait_for('Second real playback exit saved a later media position', lambda: len(close_positions()) >= 2)
    second_position, second_duration = close_positions()[-1]
    assert second_position >= first_position + 3 and abs(second_duration - first_duration) < 2
    assert shell('pidof ' + PKG).decode().strip() == pid
    r['second_saved_position_seconds'] = second_position
    r['retained_resume_marker_count'] = native_log().count('[player] retomada pronta:')
    assert r['retained_resume_marker_count'] == 1
    shot('10-real-home-after-second-exit')
    r['playback_assertions_passed'] = True
    r['execution_passed'] = True
    step('Verified ordinary video frames, live card creation, one retained Resume and increasing real media position')
except Exception as exc:
    r['error'] = {'type': type(exc).__name__, 'message': str(exc)}
    try:
        shot('failure-real-screen', require_foreground=False)
    except Exception:
        pass
finally:
    try:
        r['final_startup_diagnostics'] = diagnostics.snapshot('finally')
    except Exception as exc:
        r['final_startup_diagnostics_error'] = str(exc)
    for name, getter in [('native-log-redacted.txt', native_log), ('adb-logcat-redacted.txt', java_log)]:
        try:
            text = getter()
            r[name + '_original_sha256'] = hashlib.sha256(text.encode()).hexdigest()
            (out / name).write_text(redact(text))
            if re.search(r'FATAL EXCEPTION|Fatal signal \d+', text):
                r['execution_passed'] = r['playback_assertions_passed'] = False
                r['native_or_java_crash_seen'] = True
        except Exception as exc:
            r[name + '_error'] = str(exc)
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
    r['artifact_files_sha256'] = {str(x.relative_to(out)): hashlib.sha256(x.read_bytes()).hexdigest()
        for x in out.rglob('*') if x.is_file() and x.name != 'receipt.json'}
    (out / 'receipt.json').write_text(json.dumps(r, indent=2) + '\n')
    print(json.dumps(r, indent=2))
if not r['execution_passed']:
    raise SystemExit(1)
