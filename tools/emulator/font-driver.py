"""Isolated emulator-only UI driver; never included in a product PR."""
import argparse,base64,hashlib,json,os,re,subprocess,time,urllib.request
from pathlib import Path
P='space.nuvio.nativelegacy.touch'; ROOT=Path('emulator-results');ROOT.mkdir(exist_ok=True)
def adb(*args, binary=False, check=True):
 r=subprocess.run(['adb',*map(str,args)],stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=60)
 if check and r.returncode:raise RuntimeError('adb '+str(args[:4])+': '+r.stderr.decode(errors='replace'))
 return r.stdout if binary else r.stdout.decode(errors='replace').strip()
def private_write(name,data):
 assert re.fullmatch(r'[a-z0-9/.-]+',name)
 if isinstance(data,str):data=data.encode()
 r=subprocess.run(['adb','shell','run-as',P,'sh','-c',f"'cat > files/dados/{name}'"],input=data,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=30)
 if r.returncode:raise RuntimeError(r.stderr.decode())
def shot(name,phase):
 assert re.fullmatch(r'[a-zA-Z0-9_-]{1,64}',name)
 out=ROOT/f'phase-{phase}';out.mkdir(exist_ok=True)
 (out/(name+'.png')).write_bytes(adb('exec-out','screencap','-p',binary=True))
 return out

def snapshot(phase):
 out=ROOT/f'phase-{phase}';out.mkdir(exist_ok=True)
 for f in ['ajustes.txt','fontes-legenda.txt','player.txt']:
  text=adb('shell','run-as',P,'cat',f'files/dados/{f}',check=False)
  (out/f).write_text(text)
 (out/'activity.txt').write_text(adb('shell','dumpsys','activity','activities'))
 (out/'pid.txt').write_text(adb('shell','pidof',P,check=False))
 (out/'app-log-tail.txt').write_text(adb('shell','run-as',P,'tail','-160','files/dados/nuvio.log',check=False))

def bootstrap():
 adb('shell','settings','put','global','window_animation_scale','0')
 adb('shell','settings','put','global','transition_animation_scale','0')
 adb('shell','settings','put','system','accelerometer_rotation','0')
 adb('shell','settings','put','system','user_rotation','0')
 adb('shell','svc','wifi','disable');adb('shell','svc','data','disable')
 routes=adb('shell','ip','route'); (ROOT/'routes.txt').write_text(routes)
 assert 'default via' not in routes, 'offline fixture requires network disabled'
 adb('shell','input','keyevent','82')
 adb('shell','run-as',P,'mkdir','-p','files/dados/fontes-importadas')
 enc=lambda obj:base64.urlsafe_b64encode(json.dumps(obj,separators=(',',':')).encode()).decode().rstrip('=')
 token=enc({'alg':'none','typ':'JWT'})+'.'+enc({'sub':'emulator-font-test','exp':4102444800})+'.offline-fixture'
 private_write('sessao.txt',token+'\noffline-fixture\n0\n')
 private_write('perfis.txt','1\t0\t1\t0\t#4477aa\tEmulator font test\t\t\n')
 private_write('perfil.txt','1\n')
 private_write('envio-151.txt','1\n')
 private_write('ajustes.txt','interfaceModoLocal 1\nfonteInterface 0\n')
 selected=[]
 for file in ['Montserrat-Regular.ttf','Roboto-Regular.ttf']:
  b=Path('deploy/app/fonts',file).read_bytes();h=1469598103934665603
  for v in b:h=((h^v)*1099511628211)&0xffffffffffffffff
  name=f'{h:016x}.ttf';private_write('fontes-importadas/'+name,b);selected.append(name)
 private_write('fontes-legenda.txt','\n'.join(selected)+'\n\n')
 Path('subtitle-selections-before.json').write_text(json.dumps(selected))
 adb('shell','mkdir','-p','/sdcard/Download')
 adb('push','deploy/app/fonts/JetBrainsMonoNL-Regular.ttf','/sdcard/Download/Interface-Test.ttf')
 adb('shell','am','start','-n',P+'/space.nuvio.nativelegacy.NuvioActivity')
 time.sleep(20)
 shot('01-startup',1);snapshot(1)

def command(c,phase):
 op=c['op']
 if op=='tap':adb('shell','input','tap',int(c['x']),int(c['y']))
 elif op=='key':adb('shell','input','keyevent',int(c['code']))
 elif op=='swipe':adb('shell','input','swipe',int(c['x1']),int(c['y1']),int(c['x2']),int(c['y2']),int(c.get('ms',350)))
 elif op=='text':
  assert re.fullmatch(r'[A-Za-z0-9._/-]{1,120}',c['text']);adb('shell','input','text',c['text'])
 elif op=='wait':time.sleep(min(30,max(0,float(c['seconds']))))
 elif op=='shot':shot(c['name'],phase)
 elif op=='rotate':adb('shell','settings','put','system','user_rotation',int(c['rotation']))
 elif op=='hierarchy':
  adb('shell','uiautomator','dump','/sdcard/font-ui.xml');(ROOT/f'phase-{phase}'/'ui.xml').write_text(adb('shell','cat','/sdcard/font-ui.xml'))
 elif op=='restart':
  adb('shell','am','force-stop',P);adb('shell','am','start','-n',P+'/space.nuvio.nativelegacy.NuvioActivity')
 elif op=='assert_font':
  lines=adb('shell','run-as',P,'cat','files/dados/fontes-legenda.txt').splitlines()
  assert lines[:2]==json.loads(Path('subtitle-selections-before.json').read_text()),'subtitle selections changed'
  assert len(lines)>=3 and bool(lines[2])==bool(c['imported']),'wrong interface font state'
  assert adb('shell','pidof',P),'app stopped'
 elif op=='done':Path('emulator-done').touch()
 else:raise ValueError('Unknown test operation '+op)
 time.sleep(.8)

def phase(n):
 if Path('emulator-done').exists():return
 out=ROOT/f'phase-{n}';out.mkdir(exist_ok=True)
 deadline=time.monotonic()+420
 while time.monotonic()<deadline:
  req=urllib.request.Request('https://api.github.com/repos/ghbarker/nuvio-native-legacy/contents/tools/emulator/font-control.json?ref=review%2Flogs34-integration-20261017&v='+str(int(time.time())),headers={'Authorization':'Bearer '+os.environ['GH_TOKEN'],'Accept':'application/vnd.github+json','User-Agent':'Nuvio-Emulator-Test'})
  with urllib.request.urlopen(req,timeout=25) as r:control=json.loads(r.read())
  payload=json.loads(base64.b64decode(control['content']))
  commands=payload.get('phases',{}).get(str(n))
  if commands is not None:
   for c in commands:command(c,n)
   snapshot(n);(out/'commands.json').write_text(json.dumps(commands,indent=2));return
  time.sleep(8)
 raise TimeoutError('No commands supplied for emulator test phase '+str(n))

p=argparse.ArgumentParser();p.add_argument('--bootstrap',action='store_true');p.add_argument('--phase',type=int);a=p.parse_args()
if a.bootstrap:bootstrap()
else:phase(a.phase)
