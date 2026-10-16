"""Verify raw cold-detail frame evidence; never infer a visual verdict from counts.

This is an offline artifact consumer. It starts no application, emulator, server,
network request, SDK tool, or compiler. Root must inspect the actual PNG frames.
"""
from pathlib import Path, PurePosixPath
import argparse,hashlib,importlib.util,json,re,struct,zlib

HERE=Path(__file__).resolve().parent
SOURCE='f07278cddd24fbf10a59ce00b7366d7e18a3898c'
APK='793a0235482a0457a07ebd8191784ef9291ae5f544385036b43df366d036f360'
TITLE='tt21975436'
CASES={'known_swipe':'series','known_search':'series'}
REQUIRED=('00-home','01-before-complement','02-after-complement',
          '03-after-five-seconds','04-after-back','05-after-reopen')

def sha(data):return hashlib.sha256(data).hexdigest()
def file_sha(path):return sha(path.read_bytes())
def load(path):return json.loads(path.read_text())
def safe_file(root,name):
    rel=PurePosixPath(name)
    assert isinstance(name,str) and name and not rel.is_absolute() and '..' not in rel.parts,name
    path=root.joinpath(*rel.parts)
    assert not path.is_symlink() and path.is_file() and path.resolve().is_relative_to(root.resolve()),name
    return path

def png_dimensions(data):
    assert data.startswith(b'\x89PNG\r\n\x1a\n')
    pos=8;dimensions=None;ended=False
    while pos<len(data):
        assert pos+12<=len(data)
        size=struct.unpack('!I',data[pos:pos+4])[0];kind=data[pos+4:pos+8]
        end=pos+8+size;assert end+4<=len(data)
        body=data[pos+8:end]
        assert zlib.crc32(kind+body)&0xffffffff==struct.unpack('!I',data[end:end+4])[0]
        if kind==b'IHDR':
            assert dimensions is None and size==13
            dimensions=list(struct.unpack('!II',body[:8]));assert body[8]==8 and body[9] in (2,6)
        if kind==b'IEND':assert size==0;ended=True;assert end+4==len(data)
        pos=end+4
    assert ended and dimensions and dimensions[0]>0 and dimensions[1]>dimensions[0]
    return dimensions

def consume(root,output,review):
    receipt_path=safe_file(root,'receipt.json');receipt=load(receipt_path)
    assert receipt['execution_passed'] is True and not receipt['errors']
    assert receipt['source']==SOURCE and receipt['apk_sha256']==APK
    assert receipt['real_emulator'] is True and receipt['app_rebuilt'] is False
    assert receipt['source_or_backend_test_hooks'] is False and receipt['synthetic_offline_profile'] is True
    assert receipt['sdk_or_physical_device_validated'] is False
    assert receipt['requested_mobile_mode']==1
    assert receipt['requested_portrait_size']==[720,1560] and receipt['expected_logical_canvas']==[1080,2340]
    assert re.fullmatch('[0-9a-f]{40}',receipt['actual_review'])
    if review:assert receipt['actual_review']==review
    assert str(receipt['visual_verdict']).startswith('UNASSIGNED'), 'Driver must not assign visual success from metadata'
    raw=receipt['raw_files_sha256'];assert isinstance(raw,dict) and raw
    assert len(raw)<2000
    for rel,digest in raw.items():
        f=safe_file(root,rel);assert f.stat().st_size<50*1024*1024
        assert re.fullmatch('[0-9a-f]{64}',digest) and file_sha(f)==digest,rel
        assert f.suffix.lower() not in ('.apk','.aab','.keystore','.jks'),rel
    observed={str(f.relative_to(root)) for f in root.rglob('*') if f.is_file()
              and f.name!='receipt.json' and f.resolve()!=output.resolve()}
    assert observed==set(raw), 'Raw evidence manifest must cover every uploaded result file'

    rows=[json.loads(line) for line in safe_file(root,'fixture-events.jsonl').read_text().splitlines() if line.strip()]
    assert rows and all(isinstance(e['sequence'],int) and e['sequence']>0 for e in rows)
    assert [e['sequence'] for e in rows]==list(range(1,len(rows)+1))
    assert all(isinstance(e['host_time_ns'],int) and e['host_time_ns']>0 for e in rows)
    assert all(a['host_time_ns']<=b['host_time_ns'] for a,b in zip(rows,rows[1:]))
    assert all(a['monotonic_ns']<=b['monotonic_ns'] for a,b in zip(rows,rows[1:]))
    spec=importlib.util.spec_from_file_location('series_fixture_contract',HERE/'series_fixture.py')
    fixture=importlib.util.module_from_spec(spec);spec.loader.exec_module(fixture)
    assert fixture.TITLE==TITLE and fixture.CASES==CASES
    items={x['case']:x for x in receipt['scenarios']}
    assert len(items)==len(receipt['scenarios'])==2 and set(items)==set(CASES)
    frames={};all_frame_files=[]
    for frame in receipt['screenshots']:
        name=frame['file'];assert name not in all_frame_files;all_frame_files.append(name)
        case=next((c for c in CASES if name.startswith(c+'-')),None);assert case,name
        label=name[len(case)+1:];assert label.endswith('.png');label=label[:-4]
        assert frame['case']==case and frame['label']==label
        assert label not in frames.setdefault(case,{})
        f=safe_file(root,name);assert raw[name]==frame['sha256']==file_sha(f) and f.stat().st_size==frame['bytes']
        assert png_dimensions(f.read_bytes())==frame['dimensions']
        assert frame['dimensions']==receipt['requested_portrait_size']
        assert frame['capture_host_before_ns']<=frame['capture_host_after_ns']
        logpath=safe_file(root,frame['native_log']);assert raw[frame['native_log']]==frame['native_log_sha256']==file_sha(logpath)
        log=logpath.read_text()
        expressions={'page_episode_counts':r'\[detail\] episodios na pagina: '+TITLE+r' (\d+)',
            'published_episode_counts':r'\[desc\] [^\n]*: (\d+) episodios publicados antes dos extras',
            'published_season_counts':r'\[desc\] [^\n]*: [^\n]*, (\d+) temporadas'}
        for field,pattern in expressions.items():assert frame[field]==re.findall(pattern,log),(name,field)
        assert not re.search(r'FATAL EXCEPTION|Fatal signal \d+',log),name
        assert frame['fixture_last_sequence'] in {0,*[e['sequence'] for e in rows]}
        frames[case][label]=(frame,log)

    findings={}
    for case,tp in CASES.items():
        item=items[case];assert item['catalog_type']==tp and item['normal_route_assertions_passed'] is True
        assert item['visual_pass_claimed'] is False and re.fullmatch('[0-9]+',item['pid'])
        assert item['parent_screen']==('Search' if case=='known_search' else 'Home')
        assert item['early_vertical_swipe'] is (case=='known_swipe')
        case_frames=frames[case];expected_labels=set(REQUIRED)
        if case=='known_swipe':expected_labels.update(['01b-early-swipe-before-complement','03b-swipe-back-to-top'])
        if case=='known_search':expected_labels.update(['00b-search-before-query','00c-search-results'])
        assert set(case_frames)==expected_labels
        series=[e for e in rows if e.get('case')==case]
        assert all(e['response_status']==404 and e['episode_count']==0 for e in series if e['kind']=='cinemeta_fallback')
        primary=[e for e in series if e['kind']=='primary_metadata'];pending=[e for e in series if e['kind']=='nuvio_pending'];delivered=[e for e in series if e['kind']=='complement_delivered']
        assert primary and pending and delivered
        first_primary=primary[0];first_pending=next(e for e in pending if e['sequence']==item['nuvio_pending_sequence']);selected=[e for e in delivered if e['pending_request_sequence']==first_pending['sequence']];assert len(selected)==1;delivery=selected[0]
        assert item['primary_sequence']==first_primary['sequence'] and item['nuvio_pending_sequence']==first_pending['sequence']
        assert first_primary['sequence']<first_pending['sequence']<delivery['sequence']
        assert delivery['response_status']==200 and delivery['host_time_ns']-first_pending['host_time_ns']<5_000_000_000
        assert not any(e['kind'] in ('client_disconnected_before_delivery','fixture_gate_timeout','expired_nuvio_request') and e.get('pending_request_sequence')==first_pending['sequence'] for e in series)
        assert first_primary['episode_count']==0 and 'videos' not in fixture.primary(case)
        assert first_primary['request_path'] in ('/addon/'+case+'/meta/'+tp+'/'+TITLE+'.json','/addon/'+case+'/meta/series/'+TITLE+'.json')
        assert first_pending['request_host']=='catalog.nuvio.tv' and first_pending['tls'] is True and first_pending['tls_sni']=='catalog.nuvio.tv'
        assert first_pending['request_path'].endswith('/meta/series/'+TITLE+'.json')
        expected_body=json.dumps({'meta':fixture.complete(case)},separators=(',',':')).encode()
        assert delivery['request_host']=='catalog.nuvio.tv' and delivery['episode_count']==29
        assert delivery['season_counts']=={'1':12,'2':11,'3':6} and delivery['body_sha256']==sha(expected_body)
        assert item['complement_delivery']==delivery
        before,before_log=case_frames['01-before-complement'];late,late_log=case_frames['03-after-five-seconds'];reopen,reopen_log=case_frames['05-after-reopen']
        assert before['capture_host_after_ns']<delivery['host_time_ns']
        assert late['capture_host_before_ns']>=delivery['host_time_ns']+5_000_000_000
        assert before['page_episode_counts'] and before['page_episode_counts'][-1]=='0'
        assert not before['published_episode_counts'] and not before['published_season_counts']
        assert late['page_episode_counts'][-1:]==['29'] and late['published_episode_counts'][-1:]==['29'] and late['published_season_counts'][-1:]==['3']
        opening=r'detalhe: foco/abertura id='+TITLE+r'\b'
        assert len(re.findall(opening,before_log))==1 and len(re.findall(opening,late_log))==1
        assert len(re.findall(opening,reopen_log))>=2
        ordered=[case_frames[k][0] for k in REQUIRED]
        assert all(a['capture_host_after_ns']<=b['capture_host_before_ns'] for a,b in zip(ordered,ordered[1:]))
        assert item['five_second_frame']==late['file']
        frame_pids=[x[0].get('pid') for x in case_frames.values()]
        assert all(str(pid)==item['pid'] for pid in frame_pids)
        assert item['pid_after_reopen']==item['pid']
        assert re.search(r'\[interface\] modo=mobile[^\n]*canvas=1080x2340\b',late_log)
        if case=='known_swipe':
            early,_=case_frames['01b-early-swipe-before-complement'];top,_=case_frames['03b-swipe-back-to-top']
            assert before['capture_host_after_ns']<=early['capture_host_before_ns']
            assert early['capture_host_after_ns']<delivery['host_time_ns']
            assert late['capture_host_after_ns']<=top['capture_host_before_ns']
            assert top['capture_host_after_ns']<=case_frames['04-after-back'][0]['capture_host_before_ns']
        if case=='known_search':
            queries=[e for e in series if e['kind']=='addon_search'];assert queries
            assert any('/search=' in e['request_path'] for e in queries)
            assert re.search(r'\[transicao\] tela \d+ -> 5\b',before_log)
            search=case_frames['00b-search-before-query'][0];results=case_frames['00c-search-results'][0]
            assert case_frames['00-home'][0]['capture_host_after_ns']<=search['capture_host_before_ns']
            assert search['capture_host_after_ns']<=results['capture_host_before_ns']
            assert results['capture_host_after_ns']<=before['capture_host_before_ns']
        findings[case]={'catalog_type':tp,'metadata_before_delivery_frame':before['file'],
            'late_frame':late['file'],'reopen_frame':reopen['file'],
            'late_delay_ns':late['capture_host_before_ns']-delivery['host_time_ns'],
            'late_actual_episode_count':29,'late_actual_season_count':3,
            'selected_pending_sequence':first_pending['sequence'],'matching_response_sequence':delivery['sequence'],'precise_request_response_join':True,'original_host_tls_sni':'catalog.nuvio.tv','primary_has_videos':False,
            'pid':item['pid'],'pid_recorded_per_frame':True,
            'parent_screen':item['parent_screen'],'early_vertical_swipe':item['early_vertical_swipe'],
            'late_equals_reopen_raw_png':late['sha256']==reopen['sha256'],
            'raw_frames':{k:v[0]['sha256'] for k,v in case_frames.items()},
            'visual_verdict':'UNASSIGNED'}

    return {'execution_evidence_verified':True,'visual_verdict':'UNASSIGNED',
        'visual_review_required':True,'bug_reproduced':None,'bug_fixed':None,
        'actual_review':receipt['actual_review'],'source':SOURCE,'apk_sha256':APK,
        'driver_receipt_sha256':file_sha(receipt_path),'raw_manifest_entries':len(raw),
        'raw_frame_count':len(all_frame_files),'fixture_event_count':len(rows),
        'consumer_sha256':file_sha(Path(__file__)),'fixture_contract_sha256':file_sha(HERE/'series_fixture.py'),
        'scenarios':findings,
        'scope':'Offline hash, timing, protocol, live native-count and cold/reopen evidence checks only. Counts cannot establish whether season controls or episode cards are visible. Root must inspect the original raw PNGs; no product fix or physical-device validation is claimed.'}

def main():
    p=argparse.ArgumentParser();p.add_argument('--root',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--review');a=p.parse_args()
    result=consume(a.root.resolve(),a.output.resolve(),a.review)
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(result,indent=2)+chr(10))
    print(json.dumps({'execution_evidence_verified':True,'visual_verdict':'UNASSIGNED','raw_frames':result['raw_frame_count'],'output_sha256':file_sha(a.output)}))
if __name__=='__main__':main()
