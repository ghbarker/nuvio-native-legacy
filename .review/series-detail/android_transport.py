"""Private API30 fixture transport; no APK/source or TLS verification flag edits."""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import time

PKG = 'space.nuvio.nativelegacy.touch'
HOSTS = ['catalog.nuvio.tv', 'v3-cinemeta.strem.io']

def command(*args, check=True, data=None, timeout=45):
    got = subprocess.run(args, input=data, stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE, timeout=timeout)
    if check and got.returncode:
        raise RuntimeError('Command failed: ' + ' '.join(args[:3]) + ': ' + got.stderr.decode(errors='replace')[-500:])
    return got

def adb(*args, **kwargs):
    return command('adb', *args, **kwargs)

def install_system_ca(certificate, output):
    """Ephemeral API30 system trust, retaining every stock CA without a reboot."""
    certificate, output = Path(certificate), Path(output)
    output.mkdir(parents=True, exist_ok=True)
    assert adb('shell', 'getprop ro.build.version.sdk').stdout.strip() == b'30'
    assert adb('shell', 'getprop ro.kernel.qemu').stdout.strip() == b'1'
    commands = []
    def traced(label, *args, timeout=45):
        row = {'label': label, 'argv': list(args), 'timeout_seconds': timeout}
        commands.append(row)
        try:
            got = command(*args, check=False, timeout=timeout)
            row.update({'returncode': got.returncode,
                        'stdout': got.stdout.decode(errors='replace'),
                        'stderr': got.stderr.decode(errors='replace')})
        except subprocess.TimeoutExpired as exc:
            row.update({'timed_out': True,
                        'stdout': (exc.stdout or b'').decode(errors='replace'),
                        'stderr': (exc.stderr or b'').decode(errors='replace')})
            raise
        finally:
            (output / 'system-ca-setup-commands.json').write_text(json.dumps(commands, indent=2) + '\n')
        assert got.returncode == 0, 'CA preparation failed: ' + label + ': ' + row['stderr'][-400:]
        return got.stdout
    traced('root adbd only; no Android reboot', 'adb', 'root')
    traced('wait for restarted adbd', 'adb', 'wait-for-device', timeout=90)
    assert traced('require root', 'adb', 'shell', 'id -u').strip() == b'0'
    assert traced('require enforcing SELinux', 'adb', 'shell', 'getenforce').strip() == b'Enforcing'
    store = '/system/etc/security/cacerts'
    directory_stat = traced('stock directory mode owner', 'adb', 'shell', "stat -c '%a %u %g' " + store).decode().strip()
    assert re.fullmatch(r'[0-7]{3,4} [0-9]+ [0-9]+', directory_stat)
    directory_context = traced('stock directory context', 'adb', 'shell', 'ls -Zd ' + store).decode().split()[0]
    assert re.fullmatch(r'u:object_r:[a-zA-Z0-9_]+:s0', directory_context)
    stock = _ca_inventory(store, lambda label, script: traced(label, 'adb', 'shell', script))
    assert stock, 'The stock system trust store must not be empty'
    (output / 'system-ca-stock-inventory.json').write_text(json.dumps(stock, indent=2) + '\n')
    mode, uid, gid = directory_stat.split()
    temporary = '/data/local/tmp/nuvio-fixture-stock-cacerts'
    # Copy all original public certificates before mounting. cp -p preserves
    # their individual modes/owners; restorecon restores their original labels.
    script = ('set -e; test ! -e ' + temporary + '; mkdir -m 0700 ' + temporary +
              '; cp -p ' + store + '/* ' + temporary + '/; mount -t tmpfs -o mode=' + mode +
              ',uid=' + uid + ',gid=' + gid + ',fscontext=u:object_r:tmpfs:s0,context=' + directory_context + ' tmpfs ' + store +
              '; cp -p ' + temporary + '/* ' + store + '/; restorecon -R ' + store)
    traced('mount ephemeral normal system CA store and restore all stock certificates', 'adb', 'shell', script)
    restored = _ca_inventory(store, lambda label, script: traced(label, 'adb', 'shell', script))
    assert restored == stock, 'Every stock CA byte, mode, owner and context must survive the overlay'
    assert traced('verify directory mode owner', 'adb', 'shell', "stat -c '%a %u %g' " + store).decode().strip() == directory_stat
    assert traced('verify directory context', 'adb', 'shell', 'ls -Zd ' + store).decode().split()[0] == directory_context
    old_hash = command('openssl', 'x509', '-in', str(certificate), '-subject_hash_old', '-noout').stdout.decode().strip()
    assert re.fullmatch(r'[0-9a-f]{8}', old_hash)
    target = None
    for index in range(10):
        candidate = '/system/etc/security/cacerts/' + old_hash + '.' + str(index)
        if candidate.rsplit('/', 1)[1] not in stock:
            target = candidate
            break
    assert target, 'Refusing to overwrite an existing system CA'
    adb('push', str(certificate), target)
    adb('shell', 'chown 0:0 ' + target + ' && chmod 0644 ' + target + ' && restorecon ' + target)
    copied = adb('exec-out', 'cat', target).stdout
    assert copied == certificate.read_bytes()
    complete = _ca_inventory(store, lambda label, script: traced(label, 'adb', 'shell', script))
    fixture_name = target.rsplit('/', 1)[1]
    assert set(complete) == set(stock) | {fixture_name}
    assert all(complete[name] == facts for name, facts in stock.items())
    assert complete[fixture_name] == {'sha256': hashlib.sha256(copied).hexdigest(),
                                    'mode_owner': '644 0 0', 'context': directory_context}
    assert traced('SELinux remains enforcing', 'adb', 'shell', 'getenforce').strip() == b'Enforcing'
    proof = {'system_ca_path': target, 'certificate_sha256': hashlib.sha256(copied).hexdigest(),
             'mode_and_context': adb('shell', 'ls -lZ ' + target).stdout.decode(),
             'api': 30, 'unchanged_apk': True, 'tls_bypass_flags_added': False,
             'method': 'Ephemeral tmpfs over the ordinary API30 system CA directory; no reboot or verity change',
             'stock_ca_count': len(stock), 'all_stock_ca_bytes_modes_owners_contexts_preserved': True,
             'directory_mode_owner': directory_stat, 'directory_context': directory_context,
             'selinux_enforcing': True, 'complete_inventory': complete}
    (output / 'system-ca.json').write_text(json.dumps(proof, indent=2) + '\n')
    (output / 'fixture-ca-public.pem').write_bytes(copied)
    return proof

def _ca_inventory(store, execute):
    """Read all CA bytes and metadata through the selected real process root."""
    raw = execute('inventory ' + store,
        "set -e; sha256sum " + store + "/*; stat -c '%n %a %u %g' " + store + "/*; ls -Zd " + store + "/*").decode()
    rows = {}
    for line in raw.splitlines():
        parts = line.split()
        if len(parts) == 2 and re.fullmatch(r'[0-9a-f]{64}', parts[0]):
            path, key, value = parts[1], 'sha256', parts[0]
        elif len(parts) == 4 and parts[0].startswith(store + '/'):
            path, key, value = parts[0], 'mode_owner', ' '.join(parts[1:])
        elif len(parts) == 2 and re.fullmatch(r'u:object_r:[a-zA-Z0-9_]+:s0', parts[0]):
            path, key, value = parts[1], 'context', parts[0]
        else:
            raise AssertionError('Unexpected CA inventory record: ' + line)
        assert path.startswith(store + '/')
        name = path[len(store) + 1:]
        assert re.fullmatch(r'[0-9a-f]{8}\.[0-9]+', name)
        row = rows.setdefault(name, {})
        assert key not in row
        row[key] = value
    assert rows and all(set(row) == {'sha256', 'mode_owner', 'context'} for row in rows.values())
    return rows

def verify_system_ca_visible(pid, output):
    """Verify actual launched app mount visibility without entering its runtime."""
    pid = str(pid)
    assert re.fullmatch(r'[0-9]+', pid)
    output = Path(output)
    proof = json.loads((output / 'system-ca.json').read_text())
    root = '/proc/' + pid + '/root/system/etc/security/cacerts'
    observed = _ca_inventory(root, lambda label, script: adb('shell', script).stdout)
    assert observed == proof['complete_inventory'], 'Launched APK cannot see the complete normal system CA store'
    facts = {'pid': pid, 'app_process_root': root, 'all_stock_and_fixture_ca_bytes_modes_owners_contexts_visible': True,
             'ca_count': len(observed), 'certificate_sha256': proof['certificate_sha256'],
             'mount_namespace': adb('shell', 'readlink /proc/' + pid + '/ns/mnt').stdout.decode().strip()}
    (output / ('app-' + pid + '-system-ca-visibility.json')).write_text(json.dumps(facts, indent=2) + '\n')
    return facts

def append_native_ca(certificate, output):
    """Call after real APK asset extraction and force-stop, before cold relaunch."""
    output = Path(output); output.mkdir(parents=True, exist_ok=True)
    relative = 'files/res/art/discord-ca.pem'
    before = adb('exec-out', 'run-as', PKG, 'cat', relative).stdout
    ca = Path(certificate).read_bytes()
    assert b'BEGIN CERTIFICATE' in before and b'BEGIN CERTIFICATE' in ca
    # Existing production CA bundle is retained byte-for-byte; add one public
    # fixture trust anchor only. No private key enters the application.
    addition = b'\n' + ca
    adb('shell', '-T', 'run-as ' + PKG + " sh -c 'cat >> " + relative + "'", data=addition)
    after = adb('exec-out', 'run-as', PKG, 'cat', relative).stdout
    assert after == before + addition
    proof = {'file': relative, 'before_sha256': hashlib.sha256(before).hexdigest(),
             'after_sha256': hashlib.sha256(after).hexdigest(),
             'original_bytes_preserved': True, 'fixture_ca_sha256': hashlib.sha256(ca).hexdigest(),
             'strict_native_request_trust_configured': True, 'tls_bypass_flags_added': False}
    (output / 'native-ca-bundle.json').write_text(json.dumps(proof, indent=2) + '\n')
    return proof

def restrict_fixture_egress(uid, output):
    """Before any fake credentials; exact fixtures + closed-world emulator DNS."""
    uid = str(uid)
    assert re.fullmatch(r'[0-9]+', uid)
    output = Path(output); output.mkdir(parents=True, exist_ok=True)
    for tool in ['iptables', 'ip6tables']:
        adb('shell', tool + ' -I OUTPUT 1 -m owner --uid-owner ' + uid + ' -j REJECT')
        adb('shell', tool + ' -C OUTPUT -m owner --uid-owner ' + uid + ' -j REJECT')
    for port in [8765, 18443]:
        rule = '-m owner --uid-owner ' + uid + ' -d 10.0.2.2 -p tcp --dport ' + str(port) + ' -j ACCEPT'
        adb('shell', 'iptables -I OUTPUT 1 ' + rule)
        adb('shell', 'iptables -C OUTPUT ' + rule)
    for protocol in ['udp', 'tcp']:
        rule = '-m owner --uid-owner ' + uid + ' -d 10.0.2.3 -p ' + protocol + ' --dport 53 -j ACCEPT'
        adb('shell', 'iptables -I OUTPUT 1 ' + rule)
        adb('shell', 'iptables -C OUTPUT ' + rule)
    nat = '-m owner --uid-owner ' + uid + ' -d 10.0.2.2 -p tcp --dport 443 -j DNAT --to-destination 10.0.2.2:18443'
    adb('shell', 'iptables -t nat -I OUTPUT 1 ' + nat)
    adb('shell', 'iptables -t nat -C OUTPUT ' + nat)
    adb('shell', 'settings put global private_dns_mode off')
    for tool in ['iptables', 'ip6tables']:
        rules = adb('shell', tool + ' -S OUTPUT').stdout.decode()
        (output / (tool + '-fixture-rules.txt')).write_text(rules)
        owned = [line for line in rules.splitlines() if '--uid-owner ' + uid + ' ' in line]
        assert len(owned) == (5 if tool == 'iptables' else 1)
        assert re.search(r'(?:^| )-j REJECT(?: |$)', owned[-1])
    (output / 'iptables-fixture-nat.txt').write_bytes(adb('shell', 'iptables -t nat -S OUTPUT').stdout)
    return {'app_uid': int(uid), 'only_tcp_fixtures': ['10.0.2.2:8765', '10.0.2.2:18443'],
            'only_dns': '10.0.2.3:53 UDP/TCP, upstream closed fixture127.0.0.1:53',
            'other_ipv4_rejected': True, 'all_ipv6_rejected': True,
            'original_https_host_and_sni_retained': True}
