"""Private emulator-only guest environment setup through existing SDL JNI.

The unchanged debuggable APK starts normally through zygote/nativebridge. The
debugger stops at the existing SDL onStart entry after JNI setup, asserts the
native main thread has not started, sets only the Activity's intended environment,
then detaches. It never changes session/profile/player fields or calls login.
"""
from pathlib import Path
import hashlib
import json
import os
import re
import selectors
import shutil
import subprocess
import time


PKG = 'space.nuvio.nativelegacy.touch'
BREAKPOINT = 'org.libsdl.app.SDLActivity.onStart'
DATA = '/data/user/0/' + PKG + '/files/dados'
ENVIRONMENT = {
    'NUVIO_DADOS': DATA,
    'NUVIO_LOG': DATA + '/nuvio.log',
    'NUVIO_LOG_ANTERIOR': DATA + '/nuvio-anterior.log',
    'HOME': DATA,
    'NUVIO_ARTE': '/data/user/0/' + PKG + '/files/res/art',
    'NUVIO_LOCALE': 'en-US',
}


def launch_with_guest_environment(package, component, output, label):
    """Return real cold-launch PID after bounded env-only debugger attach/detach."""
    if package != PKG:
        raise ValueError('Unexpected target package')
    if not component.startswith(PKG + '/') or not re.fullmatch(r'[A-Za-z0-9_.$/]+', component):
        raise ValueError('Unexpected target component')
    if not re.fullmatch(r'[A-Za-z0-9_-]+', label):
        raise ValueError('Unsafe output label')
    output = Path(output)
    output.mkdir(parents=True, exist_ok=True)
    transcript_path = output / (label + '-jdb.txt')
    receipt_path = output / (label + '-jdwp-environment.json')
    receipt = {'success': False, 'package': PKG, 'component': component,
               'breakpoint': BREAKPOINT, 'environment': ENVIRONMENT,
               'session_profile_or_player_mutation': False,
               'app_or_native_library_rebuilt': False, 'commands': []}
    transcript = bytearray()
    jdb = None
    selector = None
    port = None

    def adb(*args, timeout=30):
        got = subprocess.run(['adb', *args], stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE, timeout=timeout)
        if got.returncode:
            raise RuntimeError('ADB failed: ' + ' '.join(args[:3]) + ' ' +
                               got.stderr.decode(errors='replace')[-800:])
        return got.stdout

    def jdwp_pid_visible(pid, timeout=3):
        # `adb jdwp` streams changing PID lists and need not exit. Consume the
        # requested PID with a deadline, then stop only this listing client.
        listing = subprocess.Popen(['adb', 'jdwp'], stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE, bufsize=0)
        watch = selectors.DefaultSelector()
        seen = bytearray()
        os.set_blocking(listing.stdout.fileno(), False)
        watch.register(listing.stdout, selectors.EVENT_READ)
        try:
            deadline = time.monotonic() + timeout
            while time.monotonic() < deadline:
                for key, _ in watch.select(timeout=0.1):
                    chunk = os.read(key.fd, 65536)
                    if not chunk:
                        return False
                    seen.extend(chunk)
                    if len(seen) > 65536:
                        raise RuntimeError('Unexpectedly large JDWP PID listing')
                    if re.search(rb'(?:^|\n)' + re.escape(pid.encode('ascii')) + rb'\r?\n', seen):
                        receipt['jdwp_pid_stream_verified'] = int(pid)
                        return True
                if listing.poll() is not None:
                    return False
            return False
        finally:
            watch.close()
            if listing.poll() is None:
                listing.terminate()
            try:
                listing.communicate(timeout=3)
            except subprocess.TimeoutExpired:
                listing.kill()
                listing.communicate(timeout=3)
            receipt['jdwp_listing_client_stopped'] = listing.poll() is not None

    def read_until(pattern, start=0, timeout=30):
        deadline = time.monotonic() + timeout
        compiled = re.compile(pattern, re.MULTILINE)
        while time.monotonic() < deadline:
            if compiled.search(transcript[start:].decode(errors='replace')):
                return transcript[start:].decode(errors='replace')
            for key, _ in selector.select(timeout=0.25):
                try:
                    data = os.read(key.fd, 65536)
                except BlockingIOError:
                    continue
                if data:
                    transcript.extend(data)
                    transcript_path.write_bytes(transcript)
                elif jdb.poll() is not None:
                    raise RuntimeError('JDB exited before expected state; inspect transcript')
            if jdb.poll() is not None:
                raise RuntimeError('JDB exited before expected state; inspect transcript')
        raise RuntimeError('JDB timed out before expected state; inspect transcript')

    def command(text):
        start = len(transcript)
        receipt['commands'].append(text)
        jdb.stdin.write((text + '\n').encode())
        jdb.stdin.flush()
        return read_until(r'(?:^|\n)(?:[^\n]*\[[0-9]+\]\s*|>\s*)\Z', start)

    def require_no_error(text):
        if re.search(r'ParseException|Exception occurred|Name unknown|No such field|'
                     r'Unable to|Cannot |not found|Invalid command|is not valid', text,
                     re.IGNORECASE):
            raise RuntimeError('JDB command failed; inspect transcript')

    try:
        if shutil.which('jdb') is None:
            raise RuntimeError('The reviewed JDK must include jdb')
        package = adb('shell', 'dumpsys package ' + PKG).decode(errors='replace')
        if not re.search(r'flags=\[[^\]]*\bDEBUGGABLE\b', package):
            raise RuntimeError('Exact installed APK is not debuggable')
        adb('shell', 'am force-stop ' + PKG)
        adb('shell', 'am set-debug-app -w ' + PKG)
        # No -W: Android deliberately waits for the debugger before Activity code.
        launch = adb('shell', 'am start -n ' + component).decode(errors='replace')
        if 'Error' in launch:
            raise RuntimeError('Android rejected ordinary Activity launch')
        receipt['launch'] = launch
        deadline = time.monotonic() + 30
        pid = None
        while time.monotonic() < deadline:
            got = subprocess.run(['adb', 'shell', 'pidof ' + PKG],
                                 stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=10)
            value = got.stdout.decode().strip()
            if re.fullmatch(r'[0-9]+', value):
                pid = value
                if jdwp_pid_visible(pid):
                    break
            time.sleep(0.25)
        else:
            raise RuntimeError('Normal app process did not expose its requested JDWP endpoint')
        receipt['pid'] = int(pid)
        port = adb('forward', 'tcp:0', 'jdwp:' + pid).decode().strip()
        if not re.fullmatch(r'[0-9]+', port):
            raise RuntimeError('Unexpected dynamically allocated debugger forward')
        jdb = subprocess.Popen(['jdb', '-attach', 'localhost:' + port],
                               stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT, bufsize=0)
        selector = selectors.DefaultSelector()
        os.set_blocking(jdb.stdout.fileno(), False)
        selector.register(jdb.stdout, selectors.EVENT_READ)
        initial = read_until(r'(?:^|\n)(?:[^\n]*\[[0-9]+\]\s*|>\s*)\Z')
        explicitly_suspended = False
        if initial.rstrip().endswith('>'):
            # Stop the normally waiting Java VM before arming the deferred
            # lifecycle breakpoint, so an asynchronous onStart hit cannot be
            # accidentally resumed by the following cont command.
            suspended = command('suspend')
            require_no_error(suspended)
            if 'All threads suspended' not in suspended:
                raise RuntimeError('JDB did not acknowledge pre-breakpoint suspension')
            explicitly_suspended = True
        reply = command('stop in ' + BREAKPOINT)
        require_no_error(reply)
        if 'Deferring breakpoint' not in reply and 'Set breakpoint' not in reply:
            raise RuntimeError('Existing SDL lifecycle breakpoint was not armed')
        # Some JDWP clients suspend the VM on attach; others attach while
        # Debug.waitForDebugger is running. The breakpoint must hit in either case.
        before_continue = len(transcript)
        startup_command = 'resume' if explicitly_suspended else 'cont'
        receipt['commands'].append(startup_command)
        jdb.stdin.write((startup_command + '\n').encode())
        jdb.stdin.flush()
        hit = read_until(r'Breakpoint hit:[^\n]*org\.libsdl\.app\.SDLActivity\.onStart',
                         before_continue, timeout=45)
        read_until(r'(?:^|\n)[^\n]*\[[0-9]+\]\s*\Z', before_continue)
        receipt['breakpoint_hit_before_setters'] = True
        thread = command('print org.libsdl.app.SDLActivity.mSDLThread')
        require_no_error(thread)
        if not re.search(r'org\.libsdl\.app\.SDLActivity\.mSDLThread\s*=\s*null\b', thread):
            raise RuntimeError('Native SDL main thread already exists; refusing late environment setup')
        broken = command('print org.libsdl.app.SDLActivity.mBrokenLibraries')
        require_no_error(broken)
        if not re.search(r'org\.libsdl\.app\.SDLActivity\.mBrokenLibraries\s*=\s*false\b', broken):
            raise RuntimeError('SDL JNI libraries are broken; refusing a false startup claim')
        receipt['native_main_thread_was_null'] = True
        receipt['sdl_libraries_not_broken'] = True
        cleared = command('clear ' + BREAKPOINT)
        require_no_error(cleared)
        receipt['java_environment_readback'] = {}
        receipt['guest_environment_readback'] = {}
        for name, expected in ENVIRONMENT.items():
            read = command('print android.system.Os.getenv(' + json.dumps(name) + ')')
            require_no_error(read)
            found = re.search(r'=\s*("(?:[^"\\]|\\.)*")', read)
            if not found:
                raise RuntimeError('Approved Java environment value was absent: ' + name)
            value = json.loads(found.group(1))
            if value != expected:
                raise RuntimeError('Java Activity environment differs from approved path/value: ' + name)
            receipt['java_environment_readback'][name] = value
            expression = ('eval org.libsdl.app.SDLActivity.nativeSetenv(' +
                          json.dumps(name) + ', ' + json.dumps(value) + ')')
            require_no_error(command(expression))
            guest = command('print org.libsdl.app.SDLActivity.nativeGetHint(' +
                            json.dumps(name) + ')')
            require_no_error(guest)
            found_guest = re.search(r'=\s*("(?:[^"\\]|\\.)*")', guest)
            if not found_guest or json.loads(found_guest.group(1)) != value:
                raise RuntimeError('Existing guest SDL getenv readback differs: ' + name)
            receipt['guest_environment_readback'][name] = value
        receipt['all_existing_guest_jni_setters_completed'] = True
        # Detach resumes any remaining suspended threads. No further debugger
        # control is used while navigation, account restoration or playback runs.
        receipt['commands'].append('cont')
        jdb.stdin.write(b'cont\n')
        jdb.stdin.flush()
        receipt['commands'].append('exit')
        jdb.stdin.write(b'exit\n')
        jdb.stdin.flush()
        remaining, _ = jdb.communicate(timeout=15)
        if remaining:
            transcript.extend(remaining)
        if jdb.returncode != 0:
            raise RuntimeError('JDB did not detach cleanly')
        receipt['debugger_detached'] = True
        receipt['success'] = True
        return pid
    except Exception as exc:
        receipt['error'] = {'type': type(exc).__name__, 'message': str(exc)}
        raise
    finally:
        if jdb is not None and jdb.poll() is None:
            try:
                jdb.stdin.write(b'exit\n')
                jdb.stdin.flush()
                tail, _ = jdb.communicate(timeout=5)
                transcript.extend(tail or b'')
            except Exception:
                jdb.kill()
                jdb.wait(timeout=5)
        if selector is not None:
            selector.close()
        # Clear future startup waiting even on failure; retain the exact APK,
        # seed files and actual native logs for the driver's normal diagnostics.
        subprocess.run(['adb', 'shell', 'am clear-debug-app'],
                       stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=10)
        if port is not None:
            subprocess.run(['adb', 'forward', '--remove', 'tcp:' + port],
                           stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=10)
        transcript_path.write_bytes(transcript)
        receipt['transcript'] = {'file': transcript_path.name, 'bytes': len(transcript),
                                'sha256': hashlib.sha256(transcript).hexdigest()}
        receipt_path.write_text(json.dumps(receipt, indent=2) + '\n')
