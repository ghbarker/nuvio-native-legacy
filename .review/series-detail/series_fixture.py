"""Hosted-only ordinary addon + original-host Nuvio metadata fixture.

Control routes operate the fixture response gate only, never the application.
No production source, APK, memory, player or app-backend hooks are used.
"""
import argparse,hashlib,json,ssl,struct,threading,time,zlib
from http.server import BaseHTTPRequestHandler,ThreadingHTTPServer
from pathlib import Path
from urllib.parse import unquote,urlsplit

TITLE='tt21975436'
NAME='Kaiju No. 8'
CASES={'known_series':'series','known_swipe':'series','known_search':'series'}
ORIGIN='http://10.0.2.2:8765'

def png():
    w,h=240,360
    rows=b''.join(b'\0'+bytes([20+y//6,45+y//9,80+y//4])*w for y in range(h))
    def chunk(kind,body):return struct.pack('!I',len(body))+kind+body+struct.pack('!I',zlib.crc32(kind+body)&0xffffffff)
    return b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('!2I5B',w,h,8,2,0,0,0))+chunk(b'IDAT',zlib.compress(rows))+chunk(b'IEND',b'')
ART=png()

def primary(case):
    return {'id':TITLE,'imdb_id':TITLE,'type':CASES[case],'name':NAME,'year':'2024',
        'description':'A synthetic series fixture. The primary addon intentionally has no episode videos; the ordinary Nuvio complement supplies three seasons.',
        'poster':ORIGIN+'/poster.png','background':ORIGIN+'/poster.png','runtime':'24 min','genres':['Animation','Action'],'imdbRating':'8.2'}

def complete(case):
    meta=primary(case);meta['type']='series';videos=[]
    for season,count in ((1,12),(2,11),(3,6)):
        for number in range(1,count+1):
            videos.append({'id':f'{TITLE}:{season}:{number}','season':season,'episode':number,
                'title':f'Fixture S{season:02d}E{number:02d}','overview':'A deterministic offline episode for the rendered season-row check.',
                'released':f'2024-{season:02d}-{number:02d}T00:00:00.000Z','thumbnail':ORIGIN+'/poster.png','runtime':'24 min'})
    meta['videos']=videos
    meta['app_extras']={'cast':[]}
    return meta

class State:
    def __init__(self,path):self.path=path;self.lock=threading.Lock();self.sequence=0;self.case=None;self.gate=threading.Event();self.pending=0
    def event(self,kind,**facts):
        with self.lock:
            self.sequence+=1
            row={'sequence':self.sequence,'kind':kind,'case':self.case,'host_time_ns':time.time_ns(),'monotonic_ns':time.monotonic_ns(),**facts}
            with self.path.open('a') as f:f.write(json.dumps(row,sort_keys=True)+'\n')
            return row

class Handler(BaseHTTPRequestHandler):
    def log_message(self,*_):pass
    def send(self,value,status=200,content_type='application/json'):
        body=value if isinstance(value,bytes) else json.dumps(value,separators=(',',':')).encode()
        self.send_response(status);self.send_header('Content-Type',content_type);self.send_header('Content-Length',str(len(body)));self.send_header('Access-Control-Allow-Origin','*');self.end_headers()
        if self.command!='HEAD':self.wfile.write(body)
    def do_HEAD(self):self.do_GET()
    def do_OPTIONS(self):self.send(b'',204)
    def do_POST(self):
        host=self.headers.get('Host','').split(':')[0]
        if host!='127.0.0.1':return self.send({'error':'host-local fixture controls only'},403)
        path=urlsplit(self.path).path
        if path.startswith('/control/activate/'):
            case=path.rsplit('/',1)[1]
            if case not in CASES:return self.send({},400)
            assert not self.server.state.pending,'Cannot change case while an old response remains pending'
            self.server.state.case=case;self.server.state.gate.clear();self.server.state.event('case_activated',catalog_type=CASES[case]);return self.send({'case':case})
        if path=='/control/release':
            self.server.state.event('complement_released');self.server.state.gate.set();return self.send({'released':True})
        return self.send({},404)
    def do_GET(self):
        s=self.server.state;target=urlsplit(self.path);path=unquote(target.path);host=self.headers.get('Host','').split(':')[0].lower()
        if path=='/ready':return self.send({'ready':True,'title_id':TITLE})
        if path=='/poster.png':return self.send(ART,content_type='image/png')
        parts=path.split('/')
        if len(parts)>=4 and parts[1]=='addon' and parts[2] in CASES:
            case=parts[2];route='/'.join(parts[3:]);tp=CASES[case]
            if case!=s.case:return self.send({'metas':[],'streams':[]})
            if route=='manifest.json':
                s.event('addon_manifest',request_path=path)
                return self.send({'id':'org.nuvio.fixture.series.'+case,'version':'1.0.0','name':'Primary series fixture',
                    'description':'Offline normal-protocol metadata fixture.','types':[tp,'series'] if tp!='series' else ['series'],
                    'resources':['catalog','meta','stream'],'catalogs':[{'type':tp,'id':'series-frame','name':'Series fixture','extra':[{'name':'search','isRequired':False}]}], 'idPrefixes':['tt21975436']})
            if route.startswith('catalog/'+tp+'/series-frame'):
                s.event('addon_search' if '/search=' in path else 'addon_catalog',request_path=path,catalog_type=tp,episode_count=0)
                return self.send({'metas':[primary(case)]})
            if route in ('meta/'+tp+'/'+TITLE+'.json','meta/series/'+TITLE+'.json'):
                s.event('primary_metadata',request_path=path,catalog_type=tp,episode_count=0)
                return self.send({'meta':primary(case)})
            if route.startswith('stream/'):
                s.event('addon_streams_empty',request_path=path);return self.send({'streams':[]})
            if route.startswith('subtitles/'):return self.send({'subtitles':[]})
            s.event('unrecognized_addon_route',request_path=path);return self.send({'metas':[],'streams':[],'meta':None})
        if host in ('catalog.nuvio.tv','v3-cinemeta.strem.io') and path.endswith('/meta/series/'+TITLE+'.json'):
            if not s.case:return self.send({'meta':None},404)
            tls=isinstance(self.connection,ssl.SSLSocket)
            s.event('nuvio_pending' if host=='catalog.nuvio.tv' else 'cinemeta_fallback',request_host=host,request_path=path,tls=tls,tls_sni=getattr(self.connection,'fixture_sni',None))
            s.pending+=1
            try:
                if not s.gate.wait(20):s.event('fixture_gate_timeout');return self.send({'meta':None},504)
                value={'meta':complete(s.case)};body=json.dumps(value,separators=(',',':')).encode()
                self.send(body)
                s.event('complement_delivered',request_host=host,request_path=path,episode_count=29,season_counts={'1':12,'2':11,'3':6},body_sha256=hashlib.sha256(body).hexdigest())
            except (BrokenPipeError,ConnectionResetError):s.event('client_disconnected_before_delivery',request_host=host)
            finally:s.pending-=1
            return
        # Official catalog searches are unrelated Home discovery, not metadata.
        if host in ('catalog.nuvio.tv','v3-cinemeta.strem.io') and '/catalog/' in path:return self.send({'metas':[]})
        s.event('unrecognized_request',request_host=host,request_path=path);return self.send({'meta':None,'metas':[],'streams':[]},404)

def main():
    p=argparse.ArgumentParser();p.add_argument('--events',type=Path,required=True);p.add_argument('--cert',type=Path,required=True);p.add_argument('--key',type=Path,required=True);a=p.parse_args();a.events.parent.mkdir(parents=True,exist_ok=True)
    state=State(a.events);servers=[]
    for port in (8765,18443):
        server=ThreadingHTTPServer(('127.0.0.1',port),Handler);server.daemon_threads=True;server.state=state
        if port==18443:
            context=ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER);context.load_cert_chain(a.cert,a.key)
            context.set_servername_callback(lambda sock,name,ctx:setattr(sock,'fixture_sni',name));server.socket=context.wrap_socket(server.socket,server_side=True)
        threading.Thread(target=server.serve_forever,daemon=True).start();servers.append(server)
    state.event('fixture_started')
    while True:time.sleep(1)
if __name__=='__main__':main()
