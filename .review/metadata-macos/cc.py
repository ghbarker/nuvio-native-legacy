#!/usr/bin/env python3
"""Record the real Apple compiler and execute the linked, hashed Mach-O unchanged."""
from pathlib import Path
import hashlib,json,os,shlex,subprocess,sys,time

def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def save(path,value):Path(path).write_text(json.dumps(value,indent=2)+'\n')
def main():
    proof=Path(os.environ['METADATA_CC_PROOF']);proof.mkdir(parents=True,exist_ok=True)
    stem=str(time.time_ns())+'-'+str(os.getpid())
    if sys.argv[1:2]==['--run']:
        binary=Path(sys.argv[2]);args=sys.argv[3:];started=time.time_ns()
        try:
            got=subprocess.run([str(binary),*args],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=180)
            code,output,timed_out=got.returncode,got.stdout,False
        except subprocess.TimeoutExpired as exc:
            code,output,timed_out=124,exc.stdout or b'',True
        log=proof/(stem+'-runtime.log');log.write_bytes(output)
        save(proof/(stem+'-runtime.json'),{'binary_sha256':sha(binary),'arguments':args,'returncode':code,
            'timed_out':timed_out,'started_ns':started,'finished_ns':time.time_ns(),'log_sha256':sha(log)})
        sys.stdout.buffer.write(output);return code
    args=sys.argv[1:];compiler='/usr/bin/cc'
    assert os.environ['METADATA_REAL_CC']==compiler
    assert not any(a.startswith('-D') and 'NDEBUG' in a for a in args)
    query=any(a in args for a in ['--version','-dumpmachine','-dumpversion'])
    compile_only=any(a in args for a in ['-c','-E','-S','-fsyntax-only','-M','-MM'])
    sources=[a for a in args if Path(a).suffix in ('.c','.m','.mm','.cc','.cpp')]
    effective=list(args);rpath=[]
    if not compile_only and not query:
        for value in os.environ['METADATA_RPATHS'].split(os.pathsep):
            assert Path(value).is_dir();rpath.append('-Wl,-rpath,'+value)
        effective+=rpath
    fact={'compiler':compiler,'source_review':os.environ['GITHUB_SHA'],'arguments':args,
          'effective_arguments':effective,'added_link_rpaths':rpath,'compile_only':compile_only,'query':query,
          'cwd':str(Path.cwd()),'dependencies':{}}
    if sources:
        dep_args=[];skip=False
        for value in args:
            if skip:skip=False;continue
            if value=='-o':skip=True;continue
            if value in ('-c','-S','-E'):continue
            dep_args.append(value)
        dep=subprocess.run([compiler,*dep_args,'-MM'],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
        (proof/(stem+'-dependencies.log')).write_text(dep.stdout)
        fact['dependency_returncode']=dep.returncode
        assert dep.returncode==0,dep.stdout
        root=Path(os.environ['GITHUB_WORKSPACE']).resolve()
        for line in dep.stdout.replace('\\\n',' ').splitlines():
            if ':' not in line:continue
            for word in shlex.split(line.split(':',1)[1]):
                path=Path(word).resolve()
                if path.is_file() and path.is_relative_to(root):
                    rel=path.relative_to(root).as_posix()
                    blob=subprocess.check_output(['git','hash-object',str(path)],text=True).strip()
                    expected=subprocess.check_output(['git','rev-parse','HEAD:'+rel],text=True).strip()
                    assert blob==expected,rel
                    fact['dependencies'][rel]={'git_blob':blob,'sha256':sha(path)}
    started=time.time_ns();got=subprocess.run([compiler,*effective],stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    log=proof/(stem+'-compile.log');log.write_bytes(got.stdout)
    fact.update(returncode=got.returncode,started_ns=started,finished_ns=time.time_ns(),log_sha256=sha(log))
    if got.returncode==0 and rpath and '-o' in args:
        binary=Path(args[args.index('-o')+1]).resolve()
        assert binary.is_file()
        kind=subprocess.check_output(['/usr/bin/file','-b',str(binary)],text=True).strip();assert 'Mach-O' in kind
        fact.update(binary_sha256=sha(binary),binary_bytes=binary.stat().st_size,macho=kind)
        for opt,name in [('-l','load-commands'),('-L','linked-libraries')]:
            result=subprocess.run(['otool',opt,str(binary)],stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
            (proof/(stem+'-'+name+'.log')).write_bytes(result.stdout);assert result.returncode==0
        if os.environ.get('METADATA_WRAP_RUNTIME')=='1':
            actual=binary.with_name(binary.name+'.actual');binary.rename(actual)
            binary.write_text('#!/bin/sh\nexec '+shlex.quote(sys.executable)+' '+shlex.quote(str(Path(__file__).resolve()))+' --run '+shlex.quote(str(actual))+' "$@"\n')
            binary.chmod(0o755)
    save(proof/(stem+'-compiler.json'),fact);sys.stdout.buffer.write(got.stdout);return got.returncode
if __name__=='__main__':raise SystemExit(main())
