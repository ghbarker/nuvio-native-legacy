"""Private emulator fixture: serve generated media through a real local addon.

No authentication, upstream proxying, production hooks, or fabricated UI state.
Run only on the CI host, with app-UID egress restricted to this port.
"""
import argparse
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json
from pathlib import Path
import re
import threading
from urllib.parse import urlsplit

parser = argparse.ArgumentParser()
parser.add_argument('--media', required=True, type=Path)
parser.add_argument('--poster', required=True, type=Path)
parser.add_argument('--port', type=int, default=8765)
parser.add_argument('--access-log', required=True, type=Path)
args = parser.parse_args()
if not args.media.is_file() or not args.poster.is_file():
    parser.error('Generate the real video and its poster before starting the fixture')
base = 'http://10.0.2.2:' + str(args.port)
title_id = 'tt99990495'
meta = {'id': title_id, 'type': 'movie', 'name': 'Remote resume test sample',
        'poster': base + '/poster.jpg', 'background': base + '/poster.jpg',
        'description': 'Locally generated 180-second emulator test video. No account or paid provider is used.',
        'releaseInfo': '2026', 'runtime': '3 min'}
manifest = {'id': 'org.nuvio.emulator.local.resume', 'version': '1.0.0',
            'name': 'Local emulator playback fixture', 'types': ['movie'],
            'resources': ['catalog', 'meta', 'stream'], 'idPrefixes': [title_id],
            'catalogs': [{'id': 'resume-demo', 'type': 'movie', 'name': 'Resume test sample'}]}
lock = threading.Lock()
args.access_log.parent.mkdir(parents=True, exist_ok=True)

class Handler(BaseHTTPRequestHandler):
    def log_message(self, *_):
        pass

    def record(self, status):
        row = {'method': self.command, 'path': urlsplit(self.path).path,
               'range': self.headers.get('Range'), 'status': status}
        with lock, args.access_log.open('a') as stream:
            stream.write(json.dumps(row, sort_keys=True) + '\n')

    def send_headers(self, status, mime, size, content_range=None):
        self.send_response(status)
        self.send_header('Content-Type', mime)
        self.send_header('Content-Length', str(size))
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Accept-Ranges', 'bytes')
        if content_range:
            self.send_header('Content-Range', content_range)
        self.end_headers()
        self.record(status)

    def json_response(self, obj, status=200):
        data = json.dumps(obj).encode()
        self.send_headers(status, 'application/json', len(data))
        if self.command != 'HEAD':
            self.wfile.write(data)

    def file_response(self, path, mime):
        size = path.stat().st_size
        start, end, status = 0, size - 1, 200
        requested = self.headers.get('Range')
        if requested:
            match = re.fullmatch(r'bytes=(\d*)-(\d*)', requested)
            if not match or not any(match.groups()):
                self.send_headers(416, mime, 0, 'bytes */' + str(size))
                return
            if match[1]:
                start = int(match[1])
                end = min(int(match[2]), size - 1) if match[2] else size - 1
            else:
                start = max(0, size - int(match[2]))
            if start >= size or end < start:
                self.send_headers(416, mime, 0, 'bytes */' + str(size))
                return
            status = 206
        remaining = end - start + 1
        self.send_headers(status, mime, remaining,
                     'bytes %d-%d/%d' % (start, end, size) if status == 206 else None)
        if self.command == 'HEAD':
            return
        try:
            with path.open('rb') as stream:
                stream.seek(start)
                while remaining:
                    chunk = stream.read(min(128 * 1024, remaining))
                    if not chunk:
                        break
                    self.wfile.write(chunk)
                    remaining -= len(chunk)
        except (BrokenPipeError, ConnectionResetError):
            pass  # A normal player seek cancels the preceding HTTP read.

    def do_HEAD(self):
        self.do_GET()

    def do_GET(self):
        path = urlsplit(self.path).path
        if path == '/manifest.json':
            self.json_response(manifest)
        elif path.startswith('/catalog/movie/resume-demo') and path.endswith('.json'):
            self.json_response({'metas': [] if re.search(r'skip=[1-9]', path) else [meta]})
        elif path == '/meta/movie/' + title_id + '.json':
            self.json_response({'meta': meta})
        elif path == '/stream/movie/' + title_id + '.json':
            self.json_response({'streams': [{'name': 'Local generated video',
                'title': '180 seconds, H.264/AAC; local emulator test only',
                'url': base + '/video.mp4'}]})
        elif path == '/video.mp4':
            self.file_response(args.media, 'video/mp4')
        elif path == '/poster.jpg':
            self.file_response(args.poster, 'image/jpeg')
        else:
            self.json_response({'error': 'Unknown local fixture resource'}, 404)

print(json.dumps({'fixture': 'local generated playback', 'listen': args.port,
                  'title_id': title_id, 'external_backends': False}), flush=True)
ThreadingHTTPServer(('0.0.0.0', args.port), Handler).serve_forever()
