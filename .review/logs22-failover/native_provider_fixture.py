"""Closed, local-only native provider fixture for the unchanged test APK.

The APK still uses its real Stremio parser, TorBox/Premiumize clients, source
selection and player. DNS/TLS transport is configured separately. This server
accepts only synthetic keys and hashes and never contacts an upstream provider.
"""
from pathlib import Path
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import urlsplit, parse_qs
import argparse
import hashlib
import json
import re
import ssl
import threading
import time

SOURCE = 'f07278cddd24fbf10a59ce00b7366d7e18a3898c'
FAKE_KEYS = {'torbox': 'fixture_torbox_fake',
             'premiumize': 'fixture_premiumize_fake'}
SCENES = {'first_good': 1, 'second_delayed': 2,
          'third_good': 3, 'all_fail': 4}
HOSTS = {'api.torbox.app': 'torbox',
         'www.premiumize.me': 'premiumize'}


def make_catalogue():
    result = {}
    for scene, number in SCENES.items():
        title = 'tt99995' + str(100 + number)
        candidates = []
        for index, resolution in enumerate([2160, 1080, 720], 1):
            info_hash = hashlib.sha1(
                ('nuvio-native-provider-fixture:' + scene + ':' + str(index)).encode()).hexdigest()
            candidates.append({'scene': scene, 'candidate': index,
                'resolution': resolution, 'info_hash': info_hash,
                'torrent_id': number * 100 + index,
                'filename': 'Native.Fixture.' + str(resolution) +
                    'p.WEB-DL.H264.AAC.English.' + str(index) + '.mkv'})
        result[scene] = {'title_id': title, 'candidates': candidates}
    return result


CATALOGUE = make_catalogue()
BY_HASH = {c['info_hash']: c for x in CATALOGUE.values() for c in x['candidates']}
BY_TORRENT = {str(c['torrent_id']): c for c in BY_HASH.values()}


def media_kind(candidate, provider):
    if provider == 'premiumize':
        return 'good'
    scene, index = candidate['scene'], candidate['candidate']
    return 'bad' if (scene == 'all_fail' or
                     scene == 'second_delayed' and index == 1 or
                     scene == 'third_good' and index < 3) else 'good'


class Fixture:
    def __init__(self, args):
        self.args = args
        self.good = args.good.resolve()
        self.bad = args.bad.resolve()
        self.poster = args.poster.resolve()
        for path in [self.good, self.bad, self.poster]:
            if not path.is_file():
                raise ValueError('Fixture media is missing: ' + path.name)
        self.lock = threading.Lock()
        self.sequence = 0
        self.args.events.parent.mkdir(parents=True, exist_ok=True)

    def event(self, kind, **facts):
        # Never retain request headers, raw queries, POST bodies or credentials.
        with self.lock:
            self.sequence += 1
            event = {'sequence': self.sequence, 'kind': kind,
                     'wall_time_ns': time.time_ns(),
                     'monotonic_ns': time.monotonic_ns(), **facts}
            if 'scene' in facts:
                event['current_scene'] = facts['scene']
            with self.args.events.open('a') as out:
                out.write(json.dumps(event, ensure_ascii=True) + '\n')

    def candidate_facts(self, c, provider):
        return {'scene': c['scene'], 'provider': provider,
                'candidate': c['candidate'], 'info_hash': c['info_hash']}

    def media_url(self, c, provider):
        return (self.args.media_origin.rstrip('/') + '/media/' + c['scene'] +
                '/' + provider + '/' + str(c['candidate']) + '.mkv')


class Handler(BaseHTTPRequestHandler):
    protocol_version = 'HTTP/1.1'
    server_version = 'ClosedNativeProviderFixture/1'

    @property
    def fixture(self):
        return self.server.fixture

    def log_message(self, *_):
        pass

    def response(self, status, data, content_type='application/json'):
        self.send_response(status)
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Headers', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET,HEAD,POST,OPTIONS')
        self.send_header('Content-Type', content_type)
        self.send_header('Content-Length', str(len(data)))
        self.end_headers()
        if self.command != 'HEAD':
            self.wfile.write(data)

    def json(self, value, status=200):
        self.response(status, json.dumps(value).encode())

    def reject(self, reason, status=400):
        self.fixture.event('rejected_request', reason=reason, method=self.command)
        self.json({'status': 'error', 'message': reason}, status)

    def auth(self, provider, token=None):
        expected = FAKE_KEYS[provider]
        host = self.headers.get('Host', '').lower().split(':', 1)[0]
        if HOSTS.get(host) != provider:
            self.reject('Native provider host mismatch')
            return False
        if isinstance(self.connection, ssl.SSLSocket) and getattr(self.connection, 'fixture_sni', None) != host:
            self.reject('Native provider TLS SNI mismatch')
            return False
        valid = (token == expected if token is not None else
                 self.headers.get('Authorization') == 'Bearer ' + expected)
        if not valid:
            self.reject('Only the synthetic fixture credential is accepted', 401)
        return valid

    def native_event(self, kind, candidate, provider, **facts):
        self.fixture.event(kind, **self.fixture.candidate_facts(candidate, provider),
            request_host=self.headers.get('Host', '').lower().split(':', 1)[0],
            tls=isinstance(self.connection, ssl.SSLSocket),
            tls_sni=getattr(self.connection, 'fixture_sni', None), **facts)

    def body(self):
        length = int(self.headers.get('Content-Length', '0'))
        if length < 0 or length > 16384:
            raise ValueError('Unexpected request body length')
        return self.rfile.read(length)

    def hash_candidate(self, text):
        found = re.search(r'(?:urn:btih:|^)([a-fA-F0-9]{40})(?:$|[&\s])', text)
        return BY_HASH.get(found.group(1).lower()) if found else None

    def do_OPTIONS(self):
        self.response(204, b'')

    def do_HEAD(self):
        self.do_GET()

    def do_GET(self):
        target = urlsplit(self.path)
        path, query = target.path, parse_qs(target.query)
        host = self.headers.get('Host', '').lower().split(':', 1)[0]
        if path.startswith('/v1/api/'):
            if host != 'api.torbox.app':
                return self.reject('TorBox fixture host mismatch')
            return self.torbox_get(path, query)
        if path == '/api/account/info':
            if not self.auth('premiumize'):
                return
            return self.json({'status': 'success', 'premium_until': 2000000000,
                              'limit_used': 0, 'space_used': 0})
        if path == '/':
            # Connection warming is not a provider-link generation event.
            return self.json({'fixture': True, 'source': SOURCE})
        match = re.fullmatch(r'/addon/(first_good|second_delayed|third_good|all_fail)/(a|b)/(.*)', path)
        if match:
            return self.addon(*match.groups())
        match = re.fullmatch(r'/media/(first_good|second_delayed|third_good|all_fail)/(torbox|premiumize)/([123])\.mkv', path)
        if match:
            scene, provider, number = match.groups()
            c = CATALOGUE[scene]['candidates'][int(number) - 1]
            return self.file(self.fixture.good if media_kind(c, provider) == 'good'
                             else self.fixture.bad, 'video/x-matroska', c, provider)
        if path == '/poster.jpg':
            return self.file(self.fixture.poster, 'image/jpeg')
        if path == '/fixture-contract.json':
            return self.json(json.loads(Path(__file__).with_name('fixture-config.json').read_text()))
        self.reject('Unknown local fixture route', 404)

    def torbox_get(self, path, query):
        if path.endswith('/torrents/requestdl'):
            if not self.auth('torbox', query.get('token', [''])[0]):
                return
            c = BY_TORRENT.get(query.get('torrent_id', [''])[0])
            if c is None or query.get('file_id') != ['1'] or query.get('redirect') != ['false']:
                return self.reject('Unexpected synthetic torrent/file selection')
            self.native_event('native_link_generated', c, 'torbox',
                               media_kind=media_kind(c, 'torbox'))
            return self.json({'success': True, 'error': None,
                              'data': self.fixture.media_url(c, 'torbox')})
        if not self.auth('torbox'):
            return
        if path.endswith('/torrents/checkcached'):
            c = BY_HASH.get(query.get('hash', [''])[0])
            if c is None or query.get('format') != ['list']:
                return self.reject('Unknown synthetic cache hash')
            self.native_event('native_cache_check', c, 'torbox')
            return self.json({'success': True, 'error': None, 'data': [
                {'name': c['filename'], 'size': self.fixture.good.stat().st_size,
                 'hash': c['info_hash']}]})
        if path.endswith('/torrents/mylist'):
            c = BY_TORRENT.get(query.get('id', [''])[0])
            if c is None or query.get('bypass_cache') != ['true']:
                return self.reject('Unknown synthetic torrent list')
            return self.json({'success': True, 'data': {'id': c['torrent_id'],
                'download_finished': True, 'download_present': True,
                'files': [{'id': 1, 'name': c['filename'],
                           'size': self.fixture.good.stat().st_size}]}})
        if path.endswith('/user/me'):
            return self.json({'success': True, 'data': {'id': 495, 'plan': 3,
                              'premium_expires_at': '2033-05-18T00:00:00Z'}})
        self.reject('Unknown TorBox fixture route', 404)

    def do_POST(self):
        target = urlsplit(self.path)
        host = self.headers.get('Host', '').lower().split(':', 1)[0]
        try:
            body = self.body()
        except (ValueError, OverflowError):
            return self.reject('Invalid fixture request body')
        if target.path == '/v1/api/torrents/createtorrent':
            if not self.auth('torbox'):
                return
            content_type = self.headers.get('Content-Type', '')
            c = self.hash_candidate(body.decode(errors='replace'))
            if c is None or 'multipart/form-data; boundary=' not in content_type or not re.search(
                    rb'name="add_only_if_cached"\r\n\r\ntrue\r\n', body):
                return self.reject('Expected cached-only synthetic multipart torrent')
            self.native_event('native_torrent_created', c, 'torbox')
            return self.json({'success': True, 'error': None,
                              'data': {'torrent_id': c['torrent_id']}})
        if target.path in ['/api/cache/check', '/api/transfer/directdl']:
            if not self.auth('premiumize'):
                return
            if 'application/x-www-form-urlencoded' not in self.headers.get('Content-Type', ''):
                return self.reject('Expected native Premiumize form encoding')
            form = parse_qs(body.decode(errors='strict'))
            key = 'items[]' if target.path.endswith('/cache/check') else 'src'
            values = form.get(key, [])
            c = self.hash_candidate(values[0]) if len(values) == 1 else None
            if c is None:
                return self.reject('Unknown synthetic Premiumize magnet')
            if target.path.endswith('/cache/check'):
                self.native_event('native_cache_check', c, 'premiumize')
                return self.json({'status': 'success', 'response': [True],
                                  'transcoded': [False], 'filename': [c['filename']],
                                  'filesize': [self.fixture.good.stat().st_size]})
            self.native_event('native_link_generated', c, 'premiumize',
                               media_kind='good')
            return self.json({'status': 'success', 'content': [
                {'path': c['filename'], 'size': self.fixture.good.stat().st_size,
                 'link': self.fixture.media_url(c, 'premiumize')}]})
        self.reject('Unknown native fixture POST route', 404)

    def addon(self, scene, addon, route):
        info = CATALOGUE[scene]
        title_id = info['title_id']
        title = 'Native failover ' + scene.replace('_', ' ')
        meta = {'id': title_id, 'type': 'movie', 'name': title, 'year': '2026',
                'poster': self.fixture.args.media_origin + '/poster.jpg',
                'description': 'Locally generated video and synthetic native provider responses.',
                'runtime': '3 min'}
        if route == 'manifest.json':
            return self.json({'id': 'org.nuvio.fixture.native.' + scene + '.' + addon,
                'version': '1.0.0', 'name': 'Native fixture ' + addon.upper(),
                'description': 'Closed emulator fixture; no live provider access.',
                'types': ['movie'], 'resources': ['catalog', 'meta', 'stream'] if addon == 'a' else ['stream'],
                'catalogs': [{'type': 'movie', 'id': 'native-failover', 'name': title}] if addon == 'a' else [],
                'idPrefixes': ['tt99995']})
        if re.fullmatch(r'catalog/movie/native-failover(?:/[^/]*)?\.json', route) and addon == 'a':
            self.fixture.event('addon_catalog', scene=scene, addon=addon)
            return self.json({'metas': [meta]})
        if route == 'meta/movie/' + title_id + '.json':
            return self.json({'meta': meta})
        if route == 'stream/movie/' + title_id + '.json':
            candidates = info['candidates'][:2] if addon == 'a' else info['candidates'][2:]
            self.fixture.event('addon_streams', scene=scene, addon=addon,
                               candidates=[x['candidate'] for x in candidates])
            return self.json({'streams': [
                {'name': 'Native fixture ' + str(c['resolution']) + 'p',
                 'title': c['filename'] + '\nEnglish · WEB-DL · H264 · AAC',
                 'infoHash': c['info_hash'], 'fileIdx': 0,
                 'behaviorHints': {'filename': c['filename'],
                    'videoSize': (4 - c['candidate']) * 200000000}}
                for c in candidates]})
        self.json({'metas': [], 'streams': [], 'subtitles': []})

    def file(self, path, content_type, candidate=None, provider=None):
        size = path.stat().st_size
        start, end, status = 0, size - 1, 200
        byte_range = self.headers.get('Range', '')
        if byte_range:
            match = re.fullmatch(r'bytes=(\d*)-(\d*)', byte_range.strip())
            if not match or not any(match.groups()):
                return self.response(416, b'')
            first, last = match.groups()
            if first:
                start = int(first)
                end = min(int(last) if last else size - 1, size - 1)
            else:
                start = max(0, size - int(last))
            if start > end or start >= size:
                return self.response(416, b'')
            status = 206
        facts = self.fixture.candidate_facts(candidate, provider) if candidate else {}
        delay = (candidate and provider == 'torbox' and candidate['scene'] == 'second_delayed'
                 and candidate['candidate'] == 2 and self.command == 'GET'
                 and byte_range.strip() == 'bytes=0-65535')
        if candidate:
            self.fixture.event('media_request', **facts, method=self.command,
                range=byte_range, range_start=start, range_end=end,
                media_kind=media_kind(candidate, provider),
                prefetch_header_64k=byte_range.strip() == 'bytes=0-65535',
                header_delay_ms=2500 if delay else 0,
                user_agent=self.headers.get('User-Agent', '')[:200])
        if delay:
            time.sleep(2.5)
            self.fixture.event('prefetch_header_delay_finished', **facts, delay_ms=2500)
        self.send_response(status)
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Content-Type', content_type)
        self.send_header('Accept-Ranges', 'bytes')
        self.send_header('Content-Length', str(end - start + 1))
        if status == 206:
            self.send_header('Content-Range', 'bytes ' + str(start) + '-' + str(end) + '/' + str(size))
        self.end_headers()
        sent = 0
        complete = self.command == 'HEAD'
        if self.command != 'HEAD':
            try:
                with path.open('rb') as source:
                    source.seek(start)
                    remaining = end - start + 1
                    while remaining:
                        data = source.read(min(65536, remaining))
                        if not data:
                            break
                        self.wfile.write(data)
                        sent += len(data)
                        remaining -= len(data)
                    complete = remaining == 0
            except (BrokenPipeError, ConnectionResetError):
                pass
        if candidate:
            self.fixture.event('media_response', **facts, method=self.command,
                               status=status, bytes_sent=sent, complete=complete,
                               range_start=start, range_end=end)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--bind', default='127.0.0.1')
    parser.add_argument('--port', type=int, default=8765)
    parser.add_argument('--https-port', type=int, default=18443)
    parser.add_argument('--cert', type=Path, required=True)
    parser.add_argument('--key', type=Path, required=True)
    parser.add_argument('--good', type=Path, required=True)
    parser.add_argument('--bad', type=Path, required=True)
    parser.add_argument('--poster', type=Path, required=True)
    parser.add_argument('--events', type=Path, required=True)
    parser.add_argument('--media-origin', default='http://10.0.2.2:8765')
    args = parser.parse_args()
    if args.media_origin != 'http://10.0.2.2:8765':
        raise ValueError('This closed fixture only permits the reviewed emulator-host origin')
    fixture = Fixture(args)
    http = ThreadingHTTPServer((args.bind, args.port), Handler)
    https = ThreadingHTTPServer((args.bind, args.https_port), Handler)
    http.fixture = https.fixture = fixture
    context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    context.load_cert_chain(str(args.cert), str(args.key))
    def sni_callback(sock, server_name, _context):
        if server_name not in HOSTS:
            return ssl.ALERT_DESCRIPTION_UNRECOGNIZED_NAME
        sock.fixture_sni = server_name
        fixture.event('tls_client_hello', server_name=server_name)
    context.set_servername_callback(sni_callback)
    https.socket = context.wrap_socket(https.socket, server_side=True)
    threading.Thread(target=https.serve_forever, daemon=True).start()
    fixture.event('fixture_ready', source=SOURCE, http_port=args.port,
                  https_port=args.https_port, real_provider_access=False)
    http.serve_forever()


if __name__ == '__main__':
    main()
