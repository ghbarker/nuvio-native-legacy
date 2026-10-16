"""Actual unchanged Logs22 rendered series screen; screenshots need visual review.

Only normal saved profile/preferences, addon resources and HTTP metadata are
synthetic. All input is real Android UI input; no app memory/source hooks.
"""
import argparse,base64,hashlib,json,os,re,subprocess,sys,time,traceback,urllib.request,zipfile
from pathlib import Path
from android_transport import install_system_ca,append_native_ca,restrict_fixture_egress,verify_system_ca_visible
from jdwp_environment import launch_with_guest_environment
from startup_diagnostics import StartupDiagnostics

PKG='space.nuvio.nativelegacy.touch'
SOURCE='f07278cddd24fbf10a59ce00b7366d7e18a3898c'
APK_SHA='793a0235482a0457a07ebd8191784ef9291ae5f544385036b43df366d036f360'
CERT='862d7ef63a0dbde2929f6f231cae295510726d9ac28f9292496d2a38304be5e6'
VALIDATION_SHA='20f3c08ce626b8871a807334ec7cafb80de4eebe50e3119a81d726cdf71f41c5'
TITLE='tt21975436'
HERE=Path(__file__).resolve().parent
p=argparse.ArgumentParser();p.add_argument('--apk',type=Path,required=True);p.add_argument('--validation',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--cert-dir',type=Path,required=True);p.add_argument('--dns-log',type=Path,required=True);a=p.parse_args();out=a.output;out.mkdir(parents=True,exist_ok=True)
receipt={'execution_passed':False,'visual_verdict':'UNASSIGNED: root must inspect raw before/after/reopen frames; metadata counts do not prove controls rendered',
    'real_emulator':True,'source':SOURCE,'apk_sha256':APK_SHA,'source_or_backend_test_hooks':False,'app_rebuilt':False,
    'synthetic_offline_profile':True,'normal_metadata_sequence':'primary addon with NO videos, then gated HTTPS Nuvio complement29 episodes/3 seasons',
    'actual_review':os.environ.get('GITHUB_SHA'),'requested_mobile_mode':1,'requested_portrait_size':[720,1560],'expected_logical_canvas':[1080,2340],
    'scenarios':[],'screenshots':[],'errors':[],'sdk_or_physical_device_validated':False}
server=None;server_log=None

def run(*args,data=None,check=True,timeout=45):
    r=subprocess.run(args,input=data,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=timeout)
    if check and r.returncode:raise RuntimeError('Command failed: '+str(args[:3])+': '+r.stderr.decode(errors='replace')[-700:])
    return r.stdout

def adb(*args,**kw):return run('adb',*args,**kw)
def shell(text,**kw):return adb('shell',text,**kw)
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def native():return shell('run-as '+PKG+' cat files/dados/nuvio.log',check=False).decode(errors='replace')
def java():return adb('logcat','-d','-v','epoch',check=False).decode(errors='replace')
def wait(name,predicate,timeout=30,interval=.1):
    until=time.monotonic()+timeout
    while time.monotonic()<until:
        if predicate():return
        time.sleep(interval)
    raise AssertionError('Timed out: '+name)

def events(case=None):
    rows=[];path=out/'fixture-events.jsonl'
    if path.exists():
        for line in path.read_text().splitlines():
            try:r=json.loads(line)
            except json.JSONDecodeError:continue
            if case is None or r.get('case')==case:rows.append(r)
    return rows

def control(path):
    req=urllib.request.Request('http://127.0.0.1:8765/control/'+path,data=b'',method='POST')
    with urllib.request.urlopen(req,timeout=3) as response:return json.load(response)

def write_asset(name,text):
    assert name in ('addons.txt','catalogo.txt','ids.txt')
    data=text.encode();relative='files/res/art/'+name;enc=base64.b64encode(data).decode()
    shell("run-as "+PKG+" sh -c 'printf %s "+enc+" | base64 -d > "+relative+"'")
    assert adb('exec-out','run-as',PKG,'cat',relative)==data

def foreground():
    assert shell('pidof '+PKG,check=False).strip(),'App process stopped'
    w=shell('dumpsys window').decode(errors='replace');ac=shell('dumpsys activity activities').decode(errors='replace')
    (out/'latest-window.txt').write_text(w);(out/'latest-activity.txt').write_text(ac)
    assert any(PKG in ln for ln in (w+'\n'+ac).splitlines() if any(x in ln for x in ('mCurrentFocus=','mFocusedApp=','mResumedActivity','topResumedActivity'))),'App not foreground'

def snapshot(case,label):
    foreground();before=time.time_ns();image=adb('exec-out','screencap','-p',timeout=15);after=time.time_ns()
    assert image.startswith(b'\x89PNG\r\n\x1a\n')
    path=out/(case+'-'+label+'.png');path.write_bytes(image)
    log=native();logpath=path.with_suffix('.native.txt');logpath.write_text(StartupDiagnostics._redact(log))
    dimensions=[int.from_bytes(image[16:20],'big'),int.from_bytes(image[20:24],'big')]
    assert dimensions==[720,1560],dimensions
    fact={'case':case,'label':label,'pid':shell('pidof '+PKG).decode().strip(),'file':path.name,'sha256':sha(path),'bytes':len(image),'capture_host_before_ns':before,'capture_host_after_ns':after,'dimensions':dimensions,
        'native_log':logpath.name,'native_log_sha256':sha(logpath),
        'page_episode_counts':re.findall(r'\[detail\] episodios na pagina: '+TITLE+r' (\d+)',log),
        'published_episode_counts':re.findall(r'\[desc\] [^\n]*: (\d+) episodios publicados antes dos extras',log),
        'published_season_counts':re.findall(r'\[desc\] [^\n]*: [^\n]*, (\d+) temporadas',log),
        'fixture_last_sequence':max((e['sequence'] for e in events()),default=0)}
    receipt['screenshots'].append(fact);return fact

def cold(component,label):
    pid=launch_with_guest_environment(PKG,component,out/'guest-env',label)
    wait('saved offline session restored',lambda:'[sessao] sessao restaurada' in native())
    assert 'sessao gravada invalida' not in native()
    verify_system_ca_visible(pid,out/'transport')
    return pid

def seed_case(case):
    assert shell('pm clear '+PKG).strip()==b'Success'
    shell('run-as '+PKG+' mkdir -p files/dados')
    d=StartupDiagnostics(PKG,out/'startup'/case)
    token=base64.urlsafe_b64encode(b'{"sub":"00000000-0000-4000-8000-000000005524","exp":2000000000}').decode().rstrip('=')
    d.checked_seed('sessao.txt','e30.'+token+'.offline\noffline-refresh\n0\n')
    d.checked_seed('perfil.txt','1\n');d.checked_seed('perfis.txt','1\t0\t1\t0\t#3B82F6\tFrame Test\t\t\n')
    prefs='interfaceModoLocal 1\nidioma 1\nidiomaAutoLocal 0\nrelogioTelaLocal 0\ntrailerAuto 1\ntrailerHero 1\nheroSectionEnabled 0\nhomeLayoutLocal 0\nselected_theme 12\nvidroLocal 1\nenvioAuto 1\nmenuExplorarLocal 1\nmenuGuiaLocal 1\nmenuAgendaLocal 1\nmenuPerfilLocal 1\nescolherFonteManual 1\nfontePrepararLocal 0\nfontePrecarregarLocal 0\nfontePrioridadeLocal 0\n'
    d.checked_seed('ajustes.txt',prefs)
    marks=['novidades-guia','novidades-11','novidades-12','novidades-13','novidades-131','novidades-132','novidades-133','novidades-134','novidades-139','novidades-14-celebracao','novidades-142','novidades-150','novidades-152-ui','novidades-160-ui','novidades-170-ui','novidades-172-ui','novidades-174-ui','salvos-intro','aviso-log','recintro-social','novidades-180-ui','novidades-20-guia','novidades-201','novidades-202','novidades-205','trailer-1310']
    for mark in marks:d.checked_seed(mark+'.txt','1\n')
    for name,value in (('diagnostico-otimizacao-intro.cfg','versao=1\n'),('telemetria-perguntado.txt','0\n'),('pip-modo.txt','0\n')):d.checked_seed(name,value)
    return d

try:
    assert a.apk.stat().st_size==59141052 and sha(a.apk)==APK_SHA
    assert sha(a.validation)==VALIDATION_SHA
    validation=json.loads(a.validation.read_text());assert validation['source_commit']==SOURCE and validation['apk_sha256']==APK_SHA and validation['android_build_and_apk_checks_passed']
    tools=Path(os.environ['ANDROID_HOME'])/'build-tools/35.0.0'
    sig=run(str(tools/'apksigner'),'verify','--print-certs',str(a.apk)).decode();assert 'Signer #1 certificate SHA-256 digest: '+CERT in sig
    badge=run(str(tools/'aapt'),'dump','badging',str(a.apk)).decode();assert "package: name='"+PKG+"'" in badge and "versionCode='20032'" in badge
    with zipfile.ZipFile(a.apk) as z:assert {n.split('/')[1] for n in z.namelist() if n.endswith('/libmain.so')}=={'arm64-v8a','armeabi-v7a'}
    assert adb('get-serialno').decode().strip().startswith('emulator-') and shell('getprop ro.kernel.qemu').strip()==b'1'
    assert shell('getprop ro.build.version.sdk').strip()==b'30'
    assert 'armeabi-v7a' in shell('getprop ro.product.cpu.abilist').decode().strip().split(',')
    receipt['system_ca']=install_system_ca(a.cert_dir/'ca.pem',out/'transport')
    adb('install','-r','--abi','armeabi-v7a',str(a.apk),timeout=120)
    package=shell('dumpsys package '+PKG).decode();assert re.search(r'\bprimaryCpuAbi=([^\s]+)',package)[1]=='armeabi-v7a'
    uid=re.search(r'uid:(\d+)',shell('cmd package list packages -U '+PKG).decode())[1]
    receipt['transport']=restrict_fixture_egress(uid,out/'transport')
    server_log=(out/'fixture-server.log').open('wb')
    server=subprocess.Popen([sys.executable,str(HERE/'series_fixture.py'),'--events',str(out/'fixture-events.jsonl'),'--cert',str(a.cert_dir/'server.pem'),'--key',str(a.cert_dir/'server.key')],stdout=server_log,stderr=subprocess.STDOUT)
    def ready():
        try:
            with urllib.request.urlopen('http://127.0.0.1:8765/ready',timeout=1) as response:return json.load(response)['ready']
        except (OSError,ValueError):return False
    wait('fixture ready',ready,timeout=10)
    run(sys.executable,str(HERE/'transport_preflight.py'),'--ca',str(a.cert_dir/'ca.pem'),'--output',str(out/'transport'))
    shell('wm size 720x1560');shell('settings put system accelerometer_rotation 0');shell('settings put system user_rotation 0')
    component=shell('cmd package resolve-activity --brief -a android.intent.action.MAIN -c android.intent.category.LAUNCHER '+PKG).decode().strip().splitlines()[-1]
    assert component.startswith(PKG+'/') and re.fullmatch(r'[A-Za-z0-9_.$/]+',component)
    for case,tp in (('known_series','series'),('known_swipe','series'),('known_search','series')):
        item={'case':case,'catalog_type':tp,'parent_screen':'Search' if case=='known_search' else 'Home','early_vertical_swipe':False,'normal_route_assertions_passed':False};receipt['scenarios'].append(item)
        try:
            diagnostics=seed_case(case);cold(component,case+'-bootstrap')
            wait('APK ordinary assets extracted',lambda:bool(shell('run-as '+PKG+' cat files/res/.versao',check=False).strip()))
            shell('am force-stop '+PKG)
            append_native_ca(a.cert_dir/'ca.pem',out/'transport'/case)
            # Only the normal network addon owns the title; no local origin-less duplicate.
            write_asset('addons.txt','Primary series fixture\thttp://10.0.2.2:8765/addon/'+case+'/manifest.json\t1\t1\t0\n')
            write_asset('catalogo.txt','# Titles arrive from the normal addon catalog.\n');write_asset('ids.txt','')
            control('activate/'+case);adb('logcat','-c');item['pid']=cold(component,case+'-cold')
            wait('normal addon catalog loaded',lambda:any(e['kind']=='addon_catalog' for e in events(case)))
            time.sleep(2);snapshot(case,'00-home')
            if case=='known_search':
                for code in (21,20,23):shell('input keyevent '+str(code));time.sleep(.75)
                snapshot(case,'00b-search-before-query')
                shell('input text Kaiju')
                wait('normal addon Search route used',lambda:any(e['kind']=='addon_search' for e in events(case)),timeout=15)
                time.sleep(1);snapshot(case,'00c-search-results')
                shell('input keyevent 20');time.sleep(.4)
                assert re.search(r'\[transicao\] tela \d+ -> 5\b',native()),'Search parent not reached'
            shell('input keyevent 23')
            wait('intended title opened',lambda:bool(re.search(r'detalhe: foco/abertura id='+TITLE+r'\b',native())),timeout=15)
            wait('primary addon metadata has no videos',lambda:any(e['kind']=='primary_metadata' and e['episode_count']==0 for e in events(case)))
            wait('ordinary original-host Nuvio complement pending',lambda:any(e['kind']=='nuvio_pending' for e in events(case)),timeout=10,interval=.05)
            pending=[e for e in events(case) if e['kind']=='nuvio_pending'][-1]
            before=snapshot(case,'01-before-complement')
            if case=='known_swipe':
                shell('input swipe 360 1200 360 620 180');time.sleep(.15)
                before=snapshot(case,'01b-early-swipe-before-complement');item['early_vertical_swipe']=True
            assert not any(e['kind']=='complement_delivered' for e in events(case))
            assert not any(e['kind']=='fixture_gate_timeout' for e in events(case))
            control('release')
            wait('Nuvio29 actual episodes published',lambda:bool(re.search(r'\[desc\] [^\n]*: 29 episodios publicados antes dos extras',native())),timeout=15)
            wait('three actual catalog seasons published',lambda:bool(re.search(r'\[desc\] [^\n]*: [^\n]*, 3 temporadas',native())),timeout=15)
            wait('fixture completed ordinary metadata delivery',lambda:any(e['kind']=='complement_delivered' for e in events(case)))
            delivery=[e for e in events(case) if e['kind']=='complement_delivered'][-1]
            assert delivery['request_host']=='catalog.nuvio.tv' and pending['tls'] and pending['tls_sni']=='catalog.nuvio.tv'
            assert before['capture_host_after_ns']<delivery['host_time_ns']
            assert not any(e['kind'] in ('cinemeta_fallback','client_disconnected_before_delivery','fixture_gate_timeout') for e in events(case))
            snapshot(case,'02-after-complement')
            until=delivery['host_time_ns']+5_000_000_000
            while time.time_ns()<until:time.sleep(max(0,min(.2,(until-time.time_ns())/1e9)))
            later=snapshot(case,'03-after-five-seconds');assert later['capture_host_before_ns']-delivery['host_time_ns']>=5_000_000_000
            assert later['published_episode_counts'] and later['published_episode_counts'][-1]=='29'
            if case=='known_swipe':
                shell('input swipe 360 450 360 1400 250');time.sleep(.5);snapshot(case,'03b-swipe-back-to-top')
            shell('input keyevent 4');time.sleep(1);snapshot(case,'04-after-back')
            shell('input keyevent 23')
            wait('same title reopened',lambda:len(re.findall(r'detalhe: foco/abertura id='+TITLE+r'\b',native()))>=2,timeout=15)
            time.sleep(2);snapshot(case,'05-after-reopen')
            j=java();assert 'FATAL EXCEPTION' not in j and not re.search(r'Fatal signal \d+',j+native())
            item['pid_after_reopen']=shell('pidof '+PKG).decode().strip();assert item['pid_after_reopen']==item['pid'],'Unexpected process restart'
            (out/(case+'-current-logcat.txt')).write_text(StartupDiagnostics._redact(j))
            (out/(case+'-events.json')).write_text(json.dumps(events(case),indent=2)+'\n')
            item.update({'normal_route_assertions_passed':True,'primary_sequence':next(e['sequence'] for e in events(case) if e['kind']=='primary_metadata'),'nuvio_pending_sequence':pending['sequence'],'complement_delivery':delivery,'five_second_frame':later['file'],'visual_pass_claimed':False})
        except Exception as exc:
            error={'case':case,'type':type(exc).__name__,'message':str(exc),'traceback':traceback.format_exc()};item['error']=error;receipt['errors'].append(error)
            try:snapshot(case,'failure')
            except Exception:pass
        finally:
            try:control('release')
            except Exception:pass
            shell('am force-stop '+PKG,check=False);time.sleep(.3)
    receipt['execution_passed']=not receipt['errors'] and len(receipt['scenarios'])==3 and all(x['normal_route_assertions_passed'] for x in receipt['scenarios'])
except Exception as exc:
    receipt['errors'].append({'type':type(exc).__name__,'message':str(exc),'traceback':traceback.format_exc()})
finally:
    try:(out/'final-current-native.txt').write_text(StartupDiagnostics._redact(native()))
    except Exception:pass
    shell('am force-stop '+PKG,check=False)
    if server:server.terminate();server.wait(timeout=10)
    if server_log:server_log.close()
    if a.dns_log.exists():
        data=a.dns_log.read_bytes();assert len(data)<10*1024*1024
        (out/'fixture-dns.jsonl').write_bytes(data)
        receipt['dns_snapshot']={'after_app_force_stop':True,'host_time_ns':time.time_ns(),'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()}
        server_dns=a.dns_log.with_name('fixture-dns-server.log')
        if server_dns.exists():(out/'fixture-dns-server.log').write_bytes(server_dns.read_bytes())
    receipt['raw_files_sha256']={str(x.relative_to(out)):sha(x) for x in out.rglob('*') if x.is_file() and x.name!='receipt.json'}
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
if not receipt['execution_passed']:raise SystemExit(1)
