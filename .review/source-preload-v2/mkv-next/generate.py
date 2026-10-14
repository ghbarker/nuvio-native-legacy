from pathlib import Path
import struct
import sys

root = Path(sys.argv[1])
def el(i, data):
    return i.to_bytes((i.bit_length() + 7) // 8, 'big') + (0x10000000 | len(data)).to_bytes(4, 'big') + data
def u(i, value):
    return el(i, value.to_bytes(4, 'big'))
header = ('[Script Info]\nScriptType: v4.00+\nPlayResX: 1920\nPlayResY: 1080\n'
          '[V4+ Styles]\nFormat: Name, Fontname, Fontsize, PrimaryColour\n'
          'Style: Default,Arial,48,&H00FFFFFF\n'
          '[Events]\nFormat: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text\n').encode()
for name, word in [('current', 'Current only'), ('next', 'Next only')]:
    tracks = el(0x1654ae6b, el(0xae, u(0xd7, 6) + u(0x83, 17) + el(0x86, b'S_TEXT/ASS') +
                el(0x22b59c, b'eng') + el(0x63a2, header)))
    info = el(0x1549a966, u(0x2ad7b1, 1000000) + el(0x4489, struct.pack('>d', 300000.0)))
    clusters, relative = [], []
    for index, start in enumerate([1000, 200000]):
        body = u(0xe7, start)
        relative.append(len(body))
        payload = b'\x86\x00\x00\x00' + f'{index},0,Default,,0,0,0,,{word}'.encode()
        body += el(0xa0, el(0xa1, payload) + u(0x9b, 10000))
        clusters.append(el(0x1f43b675, body))
    dummy = el(0x114d9b74, el(0x4dbb, el(0x53ab, bytes.fromhex('1c53bb6b')) + u(0x53ac, 0)))
    positions = [len(dummy) + len(info) + len(tracks)]
    positions.append(positions[0] + len(clusters[0]))
    cue_pos = positions[1] + len(clusters[1])
    seek = el(0x114d9b74, el(0x4dbb, el(0x53ab, bytes.fromhex('1c53bb6b')) + u(0x53ac, cue_pos)))
    cues = el(0x1c53bb6b, b''.join(el(0xbb, u(0xb3, timestamp) + el(0xb7, u(0xf7, 6) +
                   u(0xf1, position) + u(0xf0, rel)))
                   for timestamp, position, rel in zip([1000, 200000], positions, relative)))
    root.joinpath(name + '.mkv').write_bytes(el(0x1a45dfa3, el(0x4282, b'matroska')) +
            el(0x18538067, seek + info + tracks + b''.join(clusters) + cues + el(0xec, b'\0' * 4096)))
