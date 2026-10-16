"""Remote-only positive/negative fixture DNS and ordinary verified TLS probes."""
from pathlib import Path
import argparse
import hashlib
import json
import socket
import ssl
import struct

HOSTS = ['api.torbox.app', 'www.premiumize.me']

def probe(certificate, output):
    output = Path(output); output.mkdir(parents=True, exist_ok=True)
    result = {'dns': [], 'tls': [], 'verified': False}
    for index, host in enumerate(HOSTS + ['nuvio-isolated-fixture.invalid']):
        query_id = 0x4950 + index
        question = b''.join(bytes([len(label)]) + label.encode() for label in host.split('.')) + b'\0' + struct.pack('!HH', 1, 1)
        query = struct.pack('!6H', query_id, 0x100, 1, 0, 0, 0) + question
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as connection:
            connection.settimeout(5)
            connection.sendto(query, ('127.0.0.1', 53))
            reply, peer = connection.recvfrom(4096)
        ident, flags, count, answers, _, _ = struct.unpack('!6H', reply[:12])
        assert ident == query_id and peer == ('127.0.0.1', 53) and count == 1
        assert reply[12:12 + len(question)] == question
        if host in HOSTS:
            assert flags & 15 == 0 and answers == 1
            assert reply[-4:] == socket.inet_aton('10.0.2.2')
        else:
            assert flags & 15 == 3 and answers == 0
        result['dns'].append({'host': host, 'rcode': flags & 15,
            'answers': answers, 'response_sha256': hashlib.sha256(reply).hexdigest()})
    trust = ssl.create_default_context(cafile=str(certificate))
    for host in HOSTS:
        with socket.create_connection(('127.0.0.1', 18443), timeout=5) as connection:
            with trust.wrap_socket(connection, server_hostname=host) as tls:
                result['tls'].append({'host': host, 'hostname_and_peer_verified': True,
                    'protocol': tls.version(), 'cipher': tls.cipher(),
                    'leaf_sha256': hashlib.sha256(tls.getpeercert(binary_form=True)).hexdigest()})
    result['verified'] = True
    (output / 'host-transport-preflight.json').write_text(json.dumps(result, indent=2) + '\n')
    return result

if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument('--ca', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    a = p.parse_args()
    print(json.dumps(probe(a.ca, a.output), indent=2))
