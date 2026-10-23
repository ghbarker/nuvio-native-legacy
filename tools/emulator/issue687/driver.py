"""Real Android/Media3 trailer tests. Preserve language caches between cases."""
import json,subprocess,time,hashlib,wave,sys
from pathlib import Path
P='space.nuvio.nativelegacy.touch';OUT=Path('emulator-results');OUT.mkdir(exist_ok=True)
def adb(*args,raw=False,check=True):
 r=subprocess.run(['adb',*map(str,args)],capture_output=True,timeout=50)
 if check and r.returncode:raise RuntimeError('adb '+str(args[:4])+': '+r.stderr.decode(errors='replace'))
 return r.stdout if raw else r.stdout.decode(errors='replace').strip()
def write(name,data):
 r=subprocess.run(['adb','shell','run-as',P,'sh','-c',f"'cat > files/dados/{name}'"],input=data.encode(),capture_output=True,timeout=30)
 if r.returncode:raise RuntimeError(r.stderr.decode())
def read(name):
 try:return json.loads(adb('shell','run-as',P,'cat','files/dados/'+name,check=False))
 except (ValueError,TypeError):return None
def shot(folder,name):
 (folder/(name+'.png')).write_bytes(adb('exec-out','screencap','-p',raw=True))
def wait_state(folder,predicate,limit=90):
 until=time.monotonic()+limit;native=media=None
 while time.monotonic()<until:
  native=read('issue687-state.json');media=read('issue687-media3.json')
  if native and media:
   with (folder/'states.jsonl').open('a') as f:f.write(json.dumps({'native':native,'media3':media})+'\n')
   if predicate(native,media):return native,media
  time.sleep(.4)
 raise TimeoutError('Playback condition not reached: '+str((native,media)))
def run(name,mode,override,primary,expected,short=False):
 folder=OUT/name;folder.mkdir(exist_ok=True)
 adb('shell','am','force-stop',P)
 for f in ['issue687-state.json','issue687-media3.json','issue687-pcm-format.json','issue687-audio.pcm']:
  adb('shell','run-as',P,'rm','-f','files/dados/'+f)
 write('issue687-case.txt',mode+' '+override+' '+primary+'\n')
 adb('logcat','-c')
 adb('shell','am','start','-n',P+'/space.nuvio.nativelegacy.NuvioActivity')
 try:
  n,m=wait_state(folder,lambda n,m:n['trailer_open'] and n['playing'] and n['visible'] and m['playing'] and m['first_frame'] and m['rendered_video_frames']>=10 and n['seeked'] and m['position_ms']>=21000)
  assert n['sound'] and m['volume']>0,(n,m)
  assert m['audio_mime']=='audio/mp4a-latm',m
  assert any(l.lower().split('-')[0]==expected for l in m['audio_languages']),m
  if mode=='manual':assert n['manual_calls']==1 and n['fullscreen']==1 and n['detail_open']==1,n
  else:
   assert n['manual_calls']==0 and n['fullscreen']==0,n
   assert n['detail_open']==(mode=='detail-autoplay'),n
  shot(folder,'01-playing')
  frames=m['rendered_video_frames'];pos=m['position_ms']
  # Capture real decoded audio while frames and the playback clock advance.
  n,m=wait_state(folder,lambda n,m:m['playing'] and m['position_ms']>=pos+(3500 if short else 14000) and m['rendered_video_frames']>frames+10 and m['pcm_bytes']>100000,limit=40)
  shot(folder,'02-playing-later')
  fmt=read('issue687-pcm-format.json');assert fmt and fmt['encoding'] in [2,4],fmt
  pcm=adb('exec-out','run-as',P,'cat','files/dados/issue687-audio.pcm',raw=True)
  assert len(pcm)>100000,'No decoded audio captured'
  (folder/'decoded-audio.pcm').write_bytes(pcm)
  (folder/'pcm-format.json').write_text(json.dumps(fmt,indent=2)+'\n')
  subprocess.run(['ffmpeg','-hide_banner','-loglevel','error','-f','s16le' if fmt['encoding']==2 else 'f32le','-ar',str(fmt['sample_rate']),'-ac',str(fmt['channels']),'-i',str(folder/'decoded-audio.pcm'),'-t','18','-ac','1','-ar','16000','-y',str(folder/'decoded-audio.wav')],check=True)
  with wave.open(str(folder/'decoded-audio.wav'),'rb') as w:
   samples=w.readframes(w.getnframes());seconds=w.getnframes()/w.getframerate()
  assert seconds>2 and len(set(samples))>20,'Audio capture is empty/silent'
  (folder/'decoded-audio.pcm').unlink()
  (folder/'final-state.json').write_text(json.dumps({'native':n,'media3':m},indent=2)+'\n')
  if mode=='manual':
   adb('shell','input','keyevent','4')
   deadline=time.monotonic()+10
   while time.monotonic()<deadline:
    back=read('issue687-state.json')
    if back and not back['trailer_open']:break
    time.sleep(.3)
   assert back and not back['trailer_open'] and back['detail_open'],back
   shot(folder,'03-back-to-title')
  result={'scenario':name,'mode':mode,'override':override,'primary_audio_preference':primary,'expected_language':expected,'passed':True,'real_media3':m,'native':n,'captured_audio_seconds':seconds,'audio_sha256':hashlib.sha256((folder/'decoded-audio.wav').read_bytes()).hexdigest()}
  (folder/'result.json').write_text(json.dumps(result,indent=2)+'\n')
  print('PASS '+name+' language='+str(m['audio_languages'])+' frames='+str(m['rendered_video_frames'])+' audio_seconds='+str(round(seconds,2)),flush=True)
  return result
 finally:
  pid=adb('shell','pidof',P,check=False).split()
  args=['logcat','-d','-v','threadtime']
  if pid:args+=['--pid='+pid[0]]
  (folder/'logcat.txt').write_text(adb(*args,check=False))
  shot(folder,'04-final-diagnostic')
if __name__=='__main__':
 adb('install','-r','trailers.apk');adb('shell','pm','clear',P)
 adb('shell','run-as',P,'mkdir','-p','files/dados')
 adb('shell','input','keyevent','82')
 adb('shell','settings','put','system','accelerometer_rotation','0');adb('shell','settings','put','system','user_rotation','1')
 adb('shell','wm','size','720x1280');adb('shell','wm','density','180')
 (OUT/'android-device.txt').write_text(adb('shell','getprop','ro.build.fingerprint')+'\nABI: '+adb('shell','getprop','ro.product.cpu.abilist')+'\n')
 cases=[('manual-german','manual','de','-','de',False),('manual-french','manual','fr','-','fr',False),('manual-original','manual','~','-','en',False),('automatic-primary-german','manual','auto','de','de',False),('detail-autoplay-german','detail-autoplay','de','-','de',False),('home-autoplay-french','home-autoplay','fr','-','fr',False),('turkish-english-fallback','manual','tr','-','en',False),('german-cache-reuse','manual','de','-','de',True)]
 results=[]
 try:
  for c in cases:results.append(run(*c))
 finally:
  (OUT/'summary.json').write_text(json.dumps({'source_commit':'6c3400b012d8d2c477221c7b6b513b73a4944f8d','platform':'Android 11 API30 x86_64 emulator','same_product_source_as':'Logs107','real_android_app':True,'real_media3_decoding':True,'real_apple_network_lookup':True,'test_only_fixture':'Seeds one public movie and Settings inputs, marks introductory screens already seen, bypasses account sign-in, invokes the real manual handler or leaves real autoplay to run; observational PCM tap does not alter samples. No trailer resolver, ranking or playback implementation is replaced. x86_64 test APK uses an isolated key and is not a user update.','completed_cases':results},indent=2)+'\n')
  (OUT/'trailer-cache-files.txt').write_text(adb('shell','run-as',P,'ls','-l','files/dados/trailer',check=False))
 assert len(results)==len(cases)
