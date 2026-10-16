#!/usr/bin/env python3
"""Record real compiler, Git dependencies and binary before public-script cleanup."""
import hashlib,json,os,pathlib,shlex,subprocess,sys,time

def sha(p): return hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest()
def git(*a): return subprocess.check_output(['git',*a],text=True).strip()
root=pathlib.Path(os.environ['REPRO_SOURCE']).resolve()
out=pathlib.Path(os.environ['REPRO_CC_PROOF']); out.mkdir(parents=True,exist_ok=True)
args=sys.argv[1:]
assert '-o' in args, 'public compile/link output missing'
assert not any(a in ('-c','-E','-S') for a in args), 'unexpected compiler phase'
assert not any(a.startswith('-D') and ('NDEBUG' in a) for a in args)
compiler=os.environ['REPRO_REAL_CC']
sdl=shlex.split(subprocess.check_output(['pkg-config','--cflags','sdl2'],text=True))
# The unchanged #524 public script has Homebrew include paths; use the real
# Linux SDL installation at the compiler boundary, without editing its source.
command=[compiler,*args,*sdl]
idx=args.index('-o'); binary=pathlib.Path(args[idx+1])
pre=[compiler,*args[:idx],*args[idx+2:],*sdl,'-MM']
record={'original_arguments':args,'command':command,'dependency_command':pre,
        'real_compiler':str(pathlib.Path(compiler).resolve()),'real_compiler_sha256':sha(compiler),
        'compiler_version':subprocess.check_output([compiler,'--version'],text=True),
        'sdl_pkg_config':sdl,'source_review':git('rev-parse','HEAD')}
p=subprocess.run(pre,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
(out/'dependencies.log').write_text(p.stdout)
record['dependency_exit_code']=p.returncode
assert p.returncode==0, p.stdout
text=p.stdout.replace('\\\n',' ')
deps=set()
for line in text.splitlines():
    if ':' in line:
        for token in shlex.split(line.split(':',1)[1]):
            f=pathlib.Path(token)
            if not f.is_absolute(): f=root/f
            f=f.resolve()
            if f.is_file() and f.is_relative_to(root): deps.add(f.relative_to(root).as_posix())
assert 'src/descoberta.c' in deps or 'src/trakt.c' in deps, 'actual metadata implementation missing'
assert any(x.startswith('tests/') and x.endswith('.c') for x in deps)
record['dependencies']={}
for name in sorted(deps):
    blob=git('rev-parse','HEAD:'+name)
    assert git('hash-object',name)==blob, 'dirty actual compiler dependency: '+name
    record['dependencies'][name]={'git_blob':blob,'sha256':sha(root/name)}
record['dependency_log_sha256']=sha(out/'dependencies.log')
start=time.monotonic()
p=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
(out/'compile.log').write_text(p.stdout)
record.update(exit_code=p.returncode,seconds=time.monotonic()-start,compile_log_sha256=sha(out/'compile.log'))
if p.returncode==0:
    assert binary.is_file()
    record.update(binary_sha256=sha(binary),binary_bytes=binary.stat().st_size)
(out/'receipt.json').write_text(json.dumps(record,indent=2)+'\n')
sys.stdout.write(p.stdout)
sys.exit(p.returncode)
