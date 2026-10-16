"""Run the released ARM APK in a real emulator; capture genuine D-pad screenshots."""
from pathlib import Path
import argparse,base64,hashlib,json,os,re,subprocess,time,traceback,zipfile
P=argparse.ArgumentParser();P.add_argument('--apk',type=Path,required=True);P.add_argument('--output',type=Path,required=True);a=P.parse_args();out=a.output;out.mkdir(parents=True,exist_ok=True)
PKG='space.nuvio.nativelegacy.touch';APK_SHA='8a826174b2035a65a0c280500af79b2345ac73ef464c5a251dc357b625df7c8c';CERT='862d7ef63a0dbde2929f6f231cae295510726d9ac28f9292496d2a38304be5e6'
r={'execution_passed':False,'ui_result':'Screenshots require review; not an automated visual pass','source':'be1183f7dde83d8c8b5b9c0881537d0e4b814f0f','apk_sha256':APK_SHA,'real_emulator':True,'app_rebuilt':False,'screenshots':[],'steps':[],'expected_initial_page':'Home (single unlocked seeded profile auto-selection; actual frame requires visual review)'}
def run(*args,check=True,input=None,timeout=45):
 p=subprocess.run(args,input=input,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=timeout)
 if check and p.returncode:raise RuntimeError('Command failed: '+str(args[:4])+' '+p.stderr.decode(errors='replace')[-1000:])
 return p.stdout
adb=lambda *args,**kwargs:run('adb',*args,**kwargs)
def step(name):r['steps'].append({'name':name,'time':time.time()})
def shell(command,**kwargs):return adb('shell',command,**kwargs)
def shot(name):
 data=adb('exec-out','screencap','-p');assert data.startswith(b'\x89PNG\r\n\x1a\n');p=out/(name+'.png');p.write_bytes(data);r['screenshots'].append({'file':p.name,'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()})
def seed(name,text):
 assert re.fullmatch('[a-zA-Z0-9_.-]+',name)
 shell('run-as '+PKG+" sh -c 'cat > files/dados/"+name+"'",input=text.encode())
def key(code):shell('input keyevent '+str(code));time.sleep(1)
def foreground():
 text=shell('dumpsys window windows').decode(errors='replace')
 assert any(PKG in line for line in text.splitlines() if 'mCurrentFocus=' in line),'App is no longer the focused Android window'
try:
 data=a.apk.read_bytes();assert len(data)==59110672 and hashlib.sha256(data).hexdigest()==APK_SHA
 tools=Path(os.environ['ANDROID_HOME'])/'build-tools/35.0.0';sign=run(str(tools/'apksigner'),'verify','--print-certs',str(a.apk)).decode();assert 'Signer #1 certificate SHA-256 digest: '+CERT in sign
 badge=run(str(tools/'aapt'),'dump','badging',str(a.apk)).decode();assert "package: name='"+PKG+"'" in badge and "versionCode='20030'" in badge and "versionName='2.0.30-test-logs20'" in badge
 with zipfile.ZipFile(a.apk) as z:assert {p.split('/')[1] for p in z.namelist() if p.startswith('lib/') and p.endswith('/libmain.so')}=={'arm64-v8a','armeabi-v7a'}
 serial=adb('get-serialno').decode().strip();assert serial.startswith('emulator-');assert shell('getprop ro.kernel.qemu').decode().strip()=='1'
 props={k:shell('getprop '+k).decode().strip() for k in ['ro.product.cpu.abilist','ro.product.cpu.abilist64','ro.product.cpu.abilist32','ro.dalvik.vm.native.bridge','ro.build.fingerprint','ro.build.version.sdk']};r['emulator_properties']=props;assert props['ro.build.version.sdk']=='30'
 assert set(props['ro.product.cpu.abilist'].split(','))&{'arm64-v8a','armeabi-v7a'},'Image lacks advertised ARM ABI translation'
 assert props['ro.dalvik.vm.native.bridge'] not in ('','0'),'Native bridge unavailable'
 step('Verified exact released APK and real ARM-capable emulator')
 adb('root');adb('wait-for-device');assert shell('id -u').decode().strip()=='0'
 adb('install','-r',str(a.apk),timeout=120);uidtext=shell('cmd package list packages -U '+PKG).decode();uid=re.search(r'uid:(\d+)',uidtext);assert uid;uid=uid.group(1)
 # Synthetic local session must never reach a backend. Block both IP families first.
 for tool in ['iptables','ip6tables']:
  shell(tool+' -I OUTPUT 1 -m owner --uid-owner '+uid+' -j REJECT');shell(tool+' -C OUTPUT -m owner --uid-owner '+uid+' -j REJECT')
 r['app_uid_network_blocked_ipv4_ipv6']=True;step('Installed APK and verified app-UID network rejection')
 shell('am force-stop '+PKG);shell('run-as '+PKG+' mkdir -p files/dados')
 payload=base64.urlsafe_b64encode(b'{"sub":"00000000-0000-4000-8000-000000000495","exp":2000000000}').decode().rstrip('=')
 seed('sessao.txt','e30.'+payload+'.offline\noffline-refresh\n0\n');seed('perfil.txt','1\n');seed('perfis.txt','1\t0\t1\t0\t#3B82F6\tEmulator Test\t\t\n')
 seed('ajustes.txt','interfaceModoLocal 0\nidioma 1\nidiomaAutoLocal 0\nrelogioTelaLocal 0\ntrailerAuto 1\ntrailerHero 1\nheroSectionEnabled 0\nhomeLayoutLocal 0\nenvioAuto 1\nmenuExplorarLocal 1\nmenuGuiaLocal 1\nmenuAgendaLocal 1\nmenuPerfilLocal 1\n')
 marks=['novidades-guia','novidades-11','novidades-12','novidades-13','novidades-131','novidades-132','novidades-133','novidades-134','novidades-139','novidades-14-celebracao','novidades-142','novidades-150','novidades-152-ui','novidades-160-ui','novidades-170-ui','novidades-172-ui','novidades-174-ui','salvos-intro','aviso-log','recintro-social','novidades-180-ui','novidades-20-guia','novidades-201','novidades-202','novidades-205','trailer-1310']
 for name in marks:seed(name+'.txt','1\n')
 seed('diagnostico-otimizacao-intro.cfg','versao=1\n');seed('telemetria-perguntado.txt','0\n');seed('pip-modo.txt','0\n');step('Seeded synthetic offline local profile and actual saved TV preferences')
 adb('logcat','-c');shell('wm size 1280x720');shell('settings put system accelerometer_rotation 0');shell('settings put system user_rotation 0')
 resolved=shell('cmd package resolve-activity --brief -a android.intent.action.MAIN -c android.intent.category.LAUNCHER '+PKG).decode().strip().splitlines()[-1]
 assert resolved.startswith(PKG+'/') and resolved.count('/')==1 and re.fullmatch(r'[A-Za-z0-9_.$/]+',resolved),'Unexpected launcher component'
 r['resolved_launcher_component']=resolved
 launch=shell('am start -W -n '+resolved).decode();assert 'Error' not in launch;r['launch']=launch;time.sleep(15)
 pid=shell('pidof '+PKG).decode().strip();assert pid;r['pid']=pid;foreground();shot('01-home-before');step('Captured real initial app frame')
 for _ in range(3):key(19)
 shot('02-clock-up-focus');step('Sent three real D-pad Up key presses')
 key(23);shot('03-clock-ok-panel');step('Sent real D-pad Center press and release')
 key(4);foreground();shot('04-after-back');step('Sent real Android Back press and release')
 key(20);shot('05-home-after-down');step('Returned from island to Home focus using Down')
 key(21);key(20);key(23);foreground();shot('06-search-before');step('Opened real Search through sidebar')
 for _ in range(4):key(19)
 shot('07-search-clock-focus');key(23);shot('08-search-clock-ok-panel');key(4);foreground();key(20);shot('09-search-after-down');step('Captured Search clock focus, activation and return')
 key(21);key(20);key(20);key(23);foreground();shot('10-settings-before');step('Opened real Settings through sidebar')
 for _ in range(6):key(19)
 shot('11-settings-clock-focus');key(23);shot('12-settings-clock-ok-panel');key(4);foreground();key(20);shot('13-settings-after-down');step('Captured Settings clock focus, activation and return')
 assert shell('pidof '+PKG).decode().strip()==pid,'App process restarted during navigation'
 r['execution_passed']=True
except Exception as exc:
 r['error']={'type':type(exc).__name__,'message':str(exc)}
 try:shot('failure-real-screen')
 except Exception:pass
finally:
 try:
  raw=adb('logcat','-d','-v','threadtime',check=False).decode(errors='replace');r['raw_logcat_sha256']=hashlib.sha256(raw.encode()).hexdigest();raw=re.sub(r'https?://\S+','<url>',raw);raw=re.sub(r'(?i)(bearer\s+)\S+',r'\1<redacted>',raw);(out/'adb-logcat-redacted.txt').write_text(raw)
  if re.search(r'FATAL EXCEPTION|Fatal signal \d+',raw):r['execution_passed']=False;r['native_or_java_crash_seen']=True
 except Exception as exc:r['log_error']=str(exc)
 (out/'receipt.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r,indent=2))
if not r['execution_passed']:raise SystemExit(1)
