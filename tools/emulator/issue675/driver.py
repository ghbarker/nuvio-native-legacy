"""Actual Android APK/Media3 completion tests; only local fixtures are installed."""
import json,subprocess,time,sys,hashlib
from pathlib import Path
P='space.nuvio.nativelegacy.touch';OUT=Path('emulator-results');OUT.mkdir(exist_ok=True)
def adb(*args,raw=False,check=True):
 r=subprocess.run(['adb',*map(str,args)],capture_output=True,timeout=45)
 if check and r.returncode:raise RuntimeError('adb '+str(args[:4])+': '+r.stderr.decode(errors='replace'))
 return r.stdout if raw else r.stdout.decode(errors='replace').strip()
def write(name,data):
 if isinstance(data,str):data=data.encode()
 r=subprocess.run(['adb','shell','run-as',P,'sh','-c',f"'cat > files/dados/{name}'"],input=data,capture_output=True,timeout=60)
 if r.returncode:raise RuntimeError(r.stderr.decode())
def read(name):
 s=adb('shell','run-as',P,'cat','files/dados/'+name,check=False)
 try:return json.loads(s)
 except (ValueError,TypeError):return None
def shot(folder,name):
 (folder/(name+'.png')).write_bytes(adb('exec-out','screencap','-p',raw=True))
def wait_state(folder,predicate,limit=45):
 until=time.monotonic()+limit;latest=None
 while time.monotonic()<until:
  latest=read('issue675-state.json')
  if latest:
   with (folder/'states.jsonl').open('a') as f:f.write(json.dumps(latest)+'\n')
   if predicate(latest):return latest
  time.sleep(.4)
 raise TimeoutError('Native state condition not reached; latest='+str(latest))
def run(name,gap,apk,expect_exit):
 folder=OUT/name;folder.mkdir(exist_ok=True)
 adb('install','-r',apk)
 adb('shell','am','force-stop',P);adb('shell','pm','clear',P)
 adb('shell','run-as',P,'mkdir','-p','files/dados')
 write('eof-test.mp4',Path('eof-test.mp4').read_bytes())
 write('issue675-gap.txt',str(gap))
 write('ajustes.txt','interfaceModoLocal 1\nreacaoCreditosLocal 1\n')
 adb('logcat','-c')
 adb('shell','am','start','-n',P+'/space.nuvio.nativelegacy.NuvioActivity')
 try:
  start=wait_state(folder,lambda s:s['stage']==1 and s['detail_open']==1)
  shot(folder,'01-title')
  wait_state(folder,lambda s:s['stage']==3 and s['video_active']==1 and s['playing']==1)
  shot(folder,'02-playing')
  until=time.monotonic()+30;eos=None
  while time.monotonic()<until:
   eos=read('issue675-eos.json')
   if eos:break
   time.sleep(.25)
  assert eos,'Actual Media3 STATE_ENDED was not observed'
  time.sleep(3)
  state=read('issue675-state.json');shot(folder,'03-after-ended')
  (folder/'eos.json').write_text(json.dumps(eos,indent=2)+'\n')
  (folder/'final-state.json').write_text(json.dumps(state,indent=2)+'\n')
  assert eos['duration_ms']>=120000 and eos['actual_position_ms']>=eos['duration_ms']-1000,eos
  assert eos['injected_gap_ms']==gap,eos
  assert state is not None
  if expect_exit:
   assert state['player_open']==0 and state['detail_open']==1,state
  else:
   assert state['player_open']==1 and state['ended']==1 and not state['playing'],state
   assert 1.8<=state['duration']-state['position']<=2.2,state
  result={'scenario':name,'passed':True,'real_media3_end':eos,'native_final_state':state}
  (folder/'result.json').write_text(json.dumps(result,indent=2)+'\n')
  print('PASS '+name+' '+json.dumps(state),flush=True)
  return result
 finally:
  pid=adb('shell','pidof',P,check=False).split()
  args=['logcat','-d','-v','threadtime']
  if pid:args+=['--pid='+pid[0]]
  (folder/'logcat.txt').write_text(adb(*args,check=False))
  shot(folder,'04-final-diagnostic')
if __name__=='__main__':
 adb('shell','input','keyevent','82')
 adb('shell','settings','put','system','accelerometer_rotation','0')
 adb('shell','settings','put','system','user_rotation','1')
 adb('shell','wm','size','720x1280');adb('shell','wm','density','180')
 adb('shell','svc','wifi','disable');adb('shell','svc','data','disable')
 assert 'default via' not in adb('shell','ip','route'),'Fixture must remain offline'
 (OUT/'android-device.txt').write_text(adb('shell','getprop','ro.build.fingerprint')+'\nABI: '+adb('shell','getprop','ro.product.cpu.abilist')+'\n')
 results=[]
 try:
  results.append(run('normal-baseline',0,'baseline.apk',True))
  results.append(run('two-second-gap-baseline',2000,'baseline.apk',False))
  results.append(run('two-second-gap-guarded',2000,'guarded.apk',True))
 finally:
  (OUT/'summary.json').write_text(json.dumps({'source_commit':'7e40d5f0f25f1b8c605cea4c5d2322f061cf8ff1','platform':'Android 11 API 30 x86_64 emulator','real_android_apk':True,'real_media3_decoding':True,'injection':'Only after actual STATE_ENDED, offset the clock exposed to native UI by 2000 ms. Natural completion uses no offset.','product_code_published':False,'completed_cases':results},indent=2)+'\n')
 assert len(results)==3
