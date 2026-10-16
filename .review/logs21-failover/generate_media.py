"""Hosted-runner-only generation of valid controls and genuine unsupported MKV encoding."""
from pathlib import Path
import argparse,hashlib,json,os,re,subprocess,time
from inspect_mkv import inspect,explicit_zlib_algorithm

p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);p.add_argument('--proof',type=Path,required=True);a=p.parse_args()
if os.environ.get('GITHUB_ACTIONS')!='true' or os.environ.get('RUNNER_OS')!='Linux':
    p.error('Run only in the reviewed hosted Linux workflow; do not generate local media')
media=a.output.resolve();proof=a.proof.resolve();media.mkdir(parents=True,exist_ok=False);proof.mkdir(parents=True,exist_ok=False)
assert media!=proof and media not in proof.parents and proof not in media.parents
records=[]
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def run(label,command,timeout=180):
    started=time.monotonic();r=subprocess.run([str(x)for x in command],stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=timeout)
    out=proof/(label+'.stdout');err=proof/(label+'.stderr');out.write_bytes(r.stdout);err.write_bytes(r.stderr)
    records.append({'name':label,'command':[str(x)for x in command],'returncode':r.returncode,'seconds':round(time.monotonic()-started,3),'stdout':out.name,'stdout_sha256':sha(out),'stderr':err.name,'stderr_sha256':sha(err)})
    if r.returncode:raise RuntimeError(label+' failed: '+r.stderr.decode(errors='replace')[-3000:])
    return r.stdout
for executable in ['ffmpeg','ffprobe','mkvmerge','mkvextract','mkvinfo']:
    run(executable+'-version',[executable,'-version'if executable in ['ffmpeg','ffprobe']else'--version'],30)
subtitles=media/'english.srt';subtitles.write_text('''1
00:00:00,000 --> 00:00:15,000
ENGLISH_FIXTURE_START: real embedded subtitle metadata and text.

2
00:00:30,000 --> 00:00:45,000
ENGLISH_FIXTURE_SECOND: generated locally on the CI runner.

3
00:01:30,000 --> 00:01:45,000
ENGLISH_FIXTURE_THIRD: no external account or media provider.

4
00:02:40,000 --> 00:02:55,000
ENGLISH_FIXTURE_END: deterministic synthetic failover control.
''')
mp4=media/'healthy-control.mp4';healthy=media/'healthy-english.mkv';bad=media/'unsupported-contentcompalgo0.mkv'
run('generate-h264-aac',['ffmpeg','-hide_banner','-loglevel','error','-filter_threads','1','-f','lavfi','-i','testsrc2=size=640x360:rate=24','-f','lavfi','-i','sine=frequency=440:sample_rate=48000','-t','180','-map','0:v:0','-map','1:a:0','-c:v','libx264','-threads','1','-preset','ultrafast','-profile:v','baseline','-level','3.0','-crf','28','-pix_fmt','yuv420p','-c:a','aac','-b:a','64k','-metadata:s:a:0','language=und','-movflags','+faststart',mp4])
control_identity=json.loads(run('identify-control',['mkvmerge','-J',mp4]))
assert sorted(t['type']for t in control_identity['tracks'])==['audio','video']
source_options=[]
for track in control_identity['tracks']:
    source_options+=['--compression',str(track['id'])+':none','--language',str(track['id'])+':und']
for label,path,compression in [('healthy',healthy,'none'),('unsupported',bad,'zlib')]:
    # Track IDs apply to the following input. Explicit none is important:
    # mkvmerge defaults to zlib for some subtitle types.
    run('mkvmerge-'+label,['mkvmerge','--disable-track-statistics-tags','--output',path,*source_options,mp4,'--compression','0:'+compression,'--language','0:eng','--track-name','0:English','--default-track-flag','0:yes','--forced-display-flag','0:no',subtitles])
normalization=explicit_zlib_algorithm(bad)
(proof/'explicit-algorithm-normalization.json').write_text(json.dumps(normalization,indent=2)+'\n')
run('poster',['ffmpeg','-hide_banner','-loglevel','error','-threads','1','-i',mp4,'-frames:v','1','-update','1',media/'poster.jpg'])
validation={}
for path in [mp4,healthy,bad]:
    blob=run('ffprobe-'+path.stem,['ffprobe','-v','error','-show_format','-show_streams','-of','json',path]);meta=json.loads(blob)
    assert float(meta['format']['duration'])>=179 and len([s for s in meta['streams']if s['codec_type']=='video'])==1
    video=next(s for s in meta['streams']if s['codec_type']=='video');audio=next(s for s in meta['streams']if s['codec_type']=='audio')
    assert video['codec_name']=='h264'and video['width']==640 and video['height']==360 and audio['codec_name']=='aac'
    if path.suffix=='.mkv':
        subtitle=next(s for s in meta['streams']if s['codec_type']=='subtitle')
        assert subtitle['codec_name']=='subrip'and subtitle['tags']['language']=='eng'
    # Decode actual full audio/video with FFmpeg; a valid unsupported encoding
    # fixture is not an arbitrary corrupt header or an HTTP error body.
    decoded=run('decode-'+path.stem,['ffmpeg','-hide_banner','-loglevel','error','-xerror','-err_detect','explode','-threads','1','-i',path,'-map','0:v:0','-map','0:a:0','-f','null','-'])
    row={'file':path.name,'sha256':sha(path),'bytes':path.stat().st_size,'ffprobe_validated':True,'full_audio_video_decoded_by_ffmpeg':True,'duration_seconds':float(meta['format']['duration'])}
    if path.suffix=='.mkv':
        ebml=inspect(path);ep=proof/('ebml-'+path.stem+'.json');ep.write_text(json.dumps(ebml,indent=2)+'\n');row['ebml_receipt']=ep.name;row['ebml_sha256']=sha(ep)
        st=[t for t in ebml['tracks']if t['type']==17];assert len(st)==1 and st[0]['codec']=='S_TEXT/UTF8'and st[0]['language']=='eng'
        encodings=[e for t in ebml['tracks']for e in t['content_encodings']]
        if path==bad:
            assert len(encodings)==1 and len(st[0]['content_encodings'])==1
            enc=encodings[0];assert enc['type']==0 and enc['scope']==1 and enc['order']==0
            assert enc['compression']['algorithm']==0 and enc['compression']['algorithm_explicit']
            row['expected_media3_error']='ContentCompAlgo 0 not supported'
        else:assert not encodings,'Healthy control must have no content encodings'
        identified=json.loads(run('mkv-identify-'+path.stem,['mkvmerge','-J',path]));subtitle_id=next(t['id']for t in identified['tracks']if t['type']=='subtitles')
        extracted=media/(path.stem+'-extracted.srt');run('subtitle-extract-'+path.stem,['mkvextract',path,'tracks',str(subtitle_id)+':'+str(extracted)])
        text=extracted.read_text(encoding='utf-8-sig');assert all(marker in text for marker in ['ENGLISH_FIXTURE_START','ENGLISH_FIXTURE_SECOND','ENGLISH_FIXTURE_THIRD','ENGLISH_FIXTURE_END'])
        row['actual_english_subtitle_decompression_verified']=True;row['extracted_subtitle_sha256']=sha(extracted)
        run('mkvinfo-'+path.stem,['mkvinfo',path])
    validation[path.name]=row
reference=json.loads((Path(__file__).parent/'media3-reference.json').read_text())
assert reference['version']=='1.8.0'and reference['expected_log_fragment']=='ContentCompAlgo 0 not supported'
result={'passed':True,'hosted_runner_only':True,'ffprobe_ffmpeg_mkvextract_ebml_validations_passed':True,'media':validation,'explicit_algorithm_normalization':normalization,'poster':{'file':'poster.jpg','bytes':(media/'poster.jpg').stat().st_size,'sha256':sha(media/'poster.jpg')},'commands':records,'script_sha256':{n:sha(Path(__file__).parent/n)for n in ['generate_media.sh','generate_media.py','inspect_mkv.py','media3-reference.json']},'media3_reference':reference,'media3_runtime_validation_passed':False,'media3_runtime_validation_status':'PENDING: the unchanged released Logs21 APK must actually emit the expected parser error and play the healthy English MKV. The MP4 is a FFmpeg-only construction control, not an additional claimed APK playback case. Generator/source inspection is not a Media3 runtime pass.','scope':'Synthetic valid Matroska unsupported track compression, healthy embedded-English Matroska and MP4 control. This receipt validates media, not failover behavior, Android playback, HTTP timing, or exact Media3 error execution.'}
(proof/'media-receipt.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({'passed':True,'files':{n:r['bytes']for n,r in validation.items()},'expected_media3_error':'ContentCompAlgo 0 not supported','actual_media3_error_observed':False,'receipt':str(proof/'media-receipt.json')}))
