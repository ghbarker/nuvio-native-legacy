"""Actual row store and verbatim Settings/discovery code,21 retained cases."""
from pathlib import Path
import argparse,hashlib,json,os,platform,re,shlex,subprocess,tempfile,time

HERE=Path(__file__).resolve().parent
CASES=['full-tmdb','removed','reinstall','unknown-disabled','unknown-reinstalled',
       'identity-complete-after-unknown','cached-home','failed-after-removal','pending',
       'failed-manifest','changed-list','other-profile','package-list','disabled','empty-list',
       'raw-base','prefix-boundary','same-profile-reopen','profile-switch','full-home','full-outside']
def sha(path): return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def git(source,*args): return subprocess.check_output(['git','-C',str(source),*args],text=True).strip()
p=argparse.ArgumentParser()
p.add_argument('--source',type=Path,required=True)
p.add_argument('--out',type=Path,required=True)
p.add_argument('--cc',default='/usr/bin/cc')
p.add_argument('--sanitize',action='store_true')
a=p.parse_args();src=a.source.resolve();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False)
head=git(src,'rev-parse','HEAD');tree=git(src,'rev-parse','HEAD^{tree}')
assert not git(src,'status','--porcelain')
def source_bytes(path):
    data=subprocess.check_output(['git','-C',str(src),'show',head+':'+path])
    if (src/path).exists(): assert (src/path).read_bytes()==data,path
    return data
settings=source_bytes('src/ajustes.c').decode();discovery=source_bytes('src/descoberta.c').decode()
def function(text,needle):
    start=text.index(needle);end=text.index('\n}',start)+2
    return text[start:end]+'\n'
def block(text,needle):
    start=text.index(needle);start=text.index('{',start);level=0;state='code';i=start
    while i<len(text):
        c=text[i];n=text[i+1]if i+1<len(text)else''
        if state=='line':
            if c=='\n':state='code'
        elif state=='comment':
            if c=='*'and n=='/':state='code';i+=1
        elif state in ['"',"'"]:
            if c=='\\':i+=1
            elif c==state:state='code'
        elif c=='/'and n=='/':state='line';i+=1
        elif c=='/'and n=='*':state='comment';i+=1
        elif c in ['"',"'"]:state=c
        elif c=='{':level+=1
        elif c=='}':
            level-=1
            if not level:return text[start:i+1]+'\n'
        i+=1
    raise ValueError('unclosed production block')
parts={'settings-projection.inc':function(settings,'static int filCmpFora(')+function(settings,'static void filMontarLista('),
       'discovery-guards.inc':block(discovery,'{ const char *ids[16], *bases[16];')}
for name,text in parts.items(): (out/name).write_text(text)
origin=json.loads((HERE/'provenance.json').read_text())
assert origin['candidate']==head and origin['tree']==tree
for name,digest in origin['assertion_fixtures_unchanged'].items(): assert sha(HERE/name)==digest,name
for name,digest in origin['extracted_sha256'].items(): assert sha(out/name)==digest,name
# A local sparse worktree may omit this unchanged, real project header. Copy
# its exact HEAD blob outside the worktree; full CI sources use their own file.
headers=out/'source-headers';headers.mkdir()
sparse_headers={}
if not (src/'src/dados.h').exists():
    data=source_bytes('src/dados.h');(headers/'dados.h').write_bytes(data)
    sparse_headers['src/dados.h']={'copy':'source-headers/dados.h','sha256':sha(headers/'dados.h')}
flags=['-O1','-g','-I'+str(src/'src'),'-I'+str(out),'-I'+str(headers),'-pthread',
       '-ffunction-sections','-fdata-sections','-DFIL_TESTE']
if a.sanitize:
    flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-sanitize-recover=all']
    if platform.system()=='Linux':flags+=['-fno-pie']
env=os.environ.copy()
if a.sanitize:
    env.update(ASAN_OPTIONS='detect_leaks='+('0'if platform.system()=='Darwin'else'1')+':halt_on_error=1:abort_on_error=1',
               UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
records=[]
def run(cmd,name,timeout=60):
    start=time.monotonic()
    result=subprocess.run(cmd,cwd=src,env=env,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=timeout)
    log=out/(name+'.log');log.write_text(result.stdout)
    records.append({'name':name,'command':cmd,'exit_code':result.returncode,'expectation_met':result.returncode==0,
                    'seconds':round(time.monotonic()-start,3),'log':log.name,'log_sha256':sha(log)})
    if result.returncode: raise RuntimeError(name+' failed:\n'+result.stdout[-6000:])
run([a.cc,'--version'],'compiler-version')
objects=[];depfiles=[]
for name,path in [('fixture',HERE/'rows.c'),('fileiras',src/'src/fileiras.c')]:
    obj=out/(name+'.o');dep=out/(name+'.d');objects.append(str(obj));depfiles.append(dep)
    run([a.cc,*flags,'-MMD','-MF',str(dep),'-c',str(path),'-o',str(obj)],name+'-compile')
binary=out/'removed-addon-rows'
link=[a.cc,*objects,'-pthread','-o',str(binary)]
link+=['-Wl,-dead_strip']if platform.system()=='Darwin'else['-Wl,--gc-sections']
if a.sanitize:
    link+=['-fsanitize=address,undefined','-fno-sanitize-recover=all']
    if platform.system()=='Linux':link+=['-no-pie']
run(link,'link')
for case in CASES:
    with tempfile.TemporaryDirectory(prefix='rows-data-',dir=out)as folder:
        run([str(binary),case,folder],case)
deps=set()
for dep in depfiles:
    text=dep.read_text().replace('\\\n',' ')
    for name in shlex.split(text.split(':',1)[1]):
        path=Path(name);deps.add(path.resolve()if path.is_absolute()else(src/path).resolve())
assert all(path.is_relative_to(src)or path.is_relative_to(HERE)or path.is_relative_to(out)for path in deps)
source_hashes={path:hashlib.sha256(source_bytes(path)).hexdigest()for path in
               ['src/fileiras.c','src/fileiras.h','src/ajustes.c','src/descoberta.c','src/dados.h']}
assert head==git(src,'rev-parse','HEAD')and tree==git(src,'rev-parse','HEAD^{tree}')
assert not git(src,'status','--porcelain')
raw={str(path.relative_to(out)):sha(path)for path in sorted(out.rglob('*'))if path.is_file()and path.suffix!='.o'}
receipt={'all_passed':True,'passed':True,'source_commit':head,'source_tree':tree,'source_clean':True,
         'source':str(src),'source_sha256':source_hashes,'sanitized':a.sanitize,
         'sanitizers':['address','undefined']if a.sanitize else[],'platform':platform.system(),
         'case_count':21,'cases':CASES,'records':records,
         'fixture_sha256':{path.name:sha(path)for path in HERE.iterdir()if path.is_file()},
         'extracted_sha256':{name:sha(out/name)for name in parts},
         'dependency_sha256':{str(path):sha(path)for path in sorted(deps)},
         'sparse_source_headers':sparse_headers,'binary':binary.name,'binary_sha256':sha(binary),
         'raw_files_sha256':raw,
         'scope':'Actual fileiras.c/h and verbatim Settings two-list projection/discovery guard. Original21cases and full768row registry. Only external storage/addon snapshot boundaries; no production predicate replacement, SDK, GUI or network.'}
(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
print('PASS actual removed-addon rows:21retained cases; sanitize='+str(a.sanitize),flush=True)
