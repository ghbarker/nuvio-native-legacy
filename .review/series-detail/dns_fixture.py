"""Closed-world CI DNS fixture: only the two metadata fixture names get IPv4."""
import argparse
import ipaddress
import json
from pathlib import Path
import socket
import struct
import time

HOSTS = {'catalog.nuvio.tv', 'v3-cinemeta.strem.io'}

def response(packet):
    if len(packet) < 12:
        raise ValueError('Short DNS header')
    ident, flags, questions, _, _, _ = struct.unpack('!6H', packet[:12])
    if questions != 1 or flags & 0x8000:
        raise ValueError('Only a single DNS query is accepted')
    labels, pos = [], 12
    while True:
        if pos >= len(packet):
            raise ValueError('Missing DNS label')
        size = packet[pos]
        pos += 1
        if size == 0:
            break
        if size > 63 or pos + size > len(packet):
            raise ValueError('Compressed or invalid question label')
        labels.append(packet[pos:pos + size].decode('ascii'))
        pos += size
        if len(labels) > 32:
            raise ValueError('Too many labels')
    if pos + 4 > len(packet):
        raise ValueError('Missing DNS query type')
    query_type, query_class = struct.unpack('!HH', packet[pos:pos + 4])
    question = packet[12:pos + 4]
    host = '.'.join(labels).lower()
    permitted = host in HOSTS and query_class == 1
    answer = b''
    if permitted and query_type == 1:
        answer = b'\xc0\x0c' + struct.pack('!HHIH', 1, 1, 1, 4) + ipaddress.ip_address('10.0.2.2').packed
    rcode = 0 if permitted else 3
    reply_flags = 0x8080 | (flags & 0x0100) | rcode
    header = struct.pack('!6H', ident, reply_flags, 1, int(bool(answer)), 0, 0)
    return header + question + answer, {'host': host, 'type': query_type,
        'rcode': rcode, 'answer': '10.0.2.2' if answer else None}

def main():
    p = argparse.ArgumentParser()
    p.add_argument('--log', type=Path, required=True)
    a = p.parse_args()
    a.log.parent.mkdir(parents=True, exist_ok=True)
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(('127.0.0.1', 53))
    while True:
        packet, peer = sock.recvfrom(4096)
        try:
            reply, record = response(packet)
            sock.sendto(reply, peer)
        except (ValueError, UnicodeDecodeError, struct.error) as exc:
            record = {'invalid_query': type(exc).__name__}
        record['time'] = time.time()
        with a.log.open('a') as output:
            output.write(json.dumps(record, sort_keys=True) + '\n')

if __name__ == '__main__':
    main()
