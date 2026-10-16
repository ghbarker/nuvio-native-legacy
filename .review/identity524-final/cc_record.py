#!/usr/bin/env python3
"""Record real source graphs, compile phases, ELF outputs and executions."""
import hashlib,json,os,resource,shlex,subprocess,sys,time
from pathlib import Path

def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def save(path,value):Path(path).write_text(json.dumps(value,indent=2)+'\n')
def git(*args):return subprocess.check_output(['git',*args],text=True).strip()

def execute(binary,args):
    out=Path(os.environ['IDENTITY524_CC_PROOF'])
    number=len(list(out.glob('runtime-*.json')))+1;stem='runtime-%02d'%number
    limits={name:list(resource.getrlimit(value)) for name,value in
        [('address_space',resource.RLIMIT_AS),('core',resource.RLIMIT_CORE)]}
    assert limits['address_space']==[-1,-1] and limits['core']==[0,0]
    started=time.time_ns()
    got=subprocess.run([binary,*args],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=90)
    log=out/(stem+'.log');log.write_bytes(got.stdout)
    save(out/(stem+'.json'),{'command':[binary,*args],'arguments':args,'returncode':got.returncode,
        'binary_sha256':sha(binary),'started_ns':started,'finished_ns':time.time_ns(),
        'runtime_limits':limits,'log_sha256':sha(log),'data_directory':os.environ.get('NUVIO_DADOS')})
    sys.stdout.buffer.write(got.stdout);return got.returncode

def compile_actual(args):
    root=Path(os.environ['IDENTITY524_SOURCE']).resolve();out=Path(os.environ['IDENTITY524_CC_PROOF'])
    out.mkdir(parents=True,exist_ok=True)
    phase=len(list(out.glob('compiler-*.json')))+1;phases=int(os.environ['IDENTITY524_CC_PHASES'])
    assert 1<=phase<=phases and phases in (1,3)
    assert ('-c' in args)==(phase<phases)
    assert '-o' in args and not any(arg in ('-E','-S') for arg in args)
    assert not any(arg.startswith('-D') and 'NDEBUG' in arg for arg in args)
    compiler=os.environ['IDENTITY524_REAL_CC'];index=args.index('-o');binary=Path(args[index+1]).resolve()
    sdl=shlex.split(subprocess.check_output(['pkg-config','--cflags','sdl2'],text=True))
    command=[compiler,*args,*sdl]
    pre=[compiler,*args[:index],*args[index+2:],*sdl,'-MM']
    direct={str(Path(arg).resolve()):sha(arg) for arg in args
        if arg.endswith(('.c','.o')) and Path(arg).is_file()}
    facts={'phase':phase,'phase_count':phases,'command':command,'dependency_command':pre,
        'original_arguments':args,'direct_inputs':direct,'source_review':git('rev-parse','HEAD'),
        'real_compiler':str(Path(compiler).resolve()),'real_compiler_sha256':sha(compiler),
        'compiler_version':subprocess.check_output([compiler,'--version'],text=True),'sdl_pkg_config':sdl}
    dependency_log=out/('compiler-%d.dependencies.log'%phase)
    got=subprocess.run(pre,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
    dependency_log.write_text(got.stdout);facts['dependency_exit_code']=got.returncode
    assert got.returncode==0,got.stdout
    paths=set()
    for line in got.stdout.replace('\\\n',' ').splitlines():
        if ':' not in line:continue
        for token in shlex.split(line.split(':',1)[1]):
            path=Path(token);path=(path if path.is_absolute() else root/path).resolve()
            if path.is_file() and path.is_relative_to(root):paths.add(path.relative_to(root).as_posix())
    assert any(path.endswith('.c') for path in paths),'actual source graph missing'
    facts['dependencies']={}
    for name in sorted(paths):
        blob=git('rev-parse','HEAD:'+name)
        assert git('hash-object',name)==blob,'dirty compiler dependency: '+name
        facts['dependencies'][name]={'git_blob':blob,'sha256':sha(root/name)}
    facts['dependency_log_sha256']=sha(dependency_log)
    started=time.time_ns();got=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
    compile_log=out/('compiler-%d.log'%phase);compile_log.write_text(got.stdout)
    facts.update(returncode=got.returncode,started_ns=started,finished_ns=time.time_ns(),compile_log_sha256=sha(compile_log))
    if got.returncode==0:
        assert binary.is_file() and binary.read_bytes()[:4]==b'\x7fELF'
        facts.update(output_sha256=sha(binary),output_bytes=binary.stat().st_size,elf=True)
        if phase==phases:
            actual=binary.with_name(binary.name+'.actual');binary.rename(actual)
            binary.write_text('#!/bin/sh\nexec '+shlex.quote(sys.executable)+' '+
                shlex.quote(str(Path(__file__).resolve()))+' --run '+shlex.quote(str(actual))+' "$@"\n')
            binary.chmod(0o755)
    save(out/('compiler-%d.json'%phase),facts);sys.stdout.write(got.stdout);return got.returncode

if __name__=='__main__':
    if len(sys.argv)>2 and sys.argv[1]=='--run':raise SystemExit(execute(sys.argv[2],sys.argv[3:]))
    raise SystemExit(compile_actual(sys.argv[1:]))
