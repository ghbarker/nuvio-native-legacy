#!/usr/bin/env python3
"""Exact-source CPU UI checks; declaration-only SDK and explicit service boundaries."""
from pathlib import Path
import argparse, hashlib, json, os, platform, re, subprocess
from common import HERE, sha, verify

def main():
    p=argparse.ArgumentParser()
    for key in ['source','proof','output']:p.add_argument('--'+key,type=Path,required=True)
    a=p.parse_args();source=a.source.resolve();out=a.output.resolve()
    config,proof,manifest=verify(source,a.proof)
    assert platform.system()=='Linux'
    out.mkdir(parents=True,exist_ok=True)
    fixtures=HERE/'runtime/fixtures';sdk=HERE/'runtime/sdk'
    env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
    compile_records=[];runs=[];leaf_runs=[];notice_runs=[]
    def command(args,log):
        r=subprocess.run(list(map(str,args)),capture_output=True,text=True,env=env,timeout=180)
        Path(log).write_text(r.stdout+r.stderr)
        assert not r.returncode,(list(map(str,args)),r.returncode,(r.stdout+r.stderr)[-2000:])
        assert 'UNEXPECTED dependency:' not in r.stderr
        return r
    def build(label,fixture,modules,touch=True,sanitize=False,android=False,review_source=None):
        directory=out/label;directory.mkdir()
        flags=['-std=gnu11','-O1','-g','-ffunction-sections','-fdata-sections',
               '-I'+str(sdk),'-I'+str(source/'src'),'-Wno-deprecated-declarations',
               '-DSDL_TICKS_PASSED(A,B)=((Sint32)((B)-(A))<=0)',
               '-DNV_TOUCH_UI=1' if touch else '-UNV_TOUCH_UI']
        if android:flags+=['-DNV_ANDROID=1']
        if sanitize:flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie']
        flags+=['-DSETTINGS_SOURCE='+json.dumps(str(source/'src/ajustes.c'))]
        if review_source:flags+=['-DREVIEW_SOURCE='+json.dumps(str(source/'src'/review_source))]
        objects=[];depfiles=[]
        inputs=[fixtures/fixture]+[source/'src'/name for name in modules]
        for i,item in enumerate(inputs):
            obj=directory/(str(i)+'.o');dep=directory/(str(i)+'.d')
            command(['cc',*flags,'-MMD','-MF',dep,'-c',item,'-o',obj],directory/(str(i)+'.build.log'))
            objects.append(obj);depfiles.append(dep)
        binary=directory/'fixture'
        link=['cc',*objects,'-Wl,--gc-sections','-lm','-o',binary]
        if sanitize:link+=['-fsanitize=address,undefined','-no-pie']
        trial=subprocess.run(list(map(str,link)),capture_output=True,text=True,env=env)
        traps=sorted(set(re.findall(r"undefined reference to [`']([^'`]+)['`]",trial.stderr)))
        if trial.returncode:
            assert traps,trial.stderr
            text='#include <stdio.h>\n#include <stdlib.h>\n'
            for i,name in enumerate(traps):
                text+=f'void trap_{i}(void) __asm__({json.dumps(name)});\nvoid trap_{i}(void) {{fputs({json.dumps("UNEXPECTED dependency: "+name+chr(10))},stderr);abort();}}\n'
            trap=directory/'unreached-boundaries.c';trap.write_text(text)
            trap_obj=directory/'unreached-boundaries.o'
            command(['cc',*flags,'-c',trap,'-o',trap_obj],directory/'traps.build.log')
            link.insert(1,trap_obj)
        command(link,directory/'link.log')
        compile_records.append({'label':label,'touch_support':touch,'sanitizers':['address','undefined'] if sanitize else [],
            'fixture_sha256':sha(fixtures/fixture),'compiled_source_sha256':{str(x.relative_to(source)):sha(x) for x in inputs if x.is_relative_to(source)},
            'dependency_files':[{ 'path':str(x.relative_to(out)),'sha256':sha(x)} for x in depfiles],
            'unreached_dependency_traps':traps,'binary_sha256':sha(binary)})
        return binary
    def run(binary,args,label):
        r=command([binary,*args],out/(label+'.log'))
        record={'name':label,'returncode':r.returncode,'log_sha256':sha(out/(label+'.log'))}
        runs.append(record);return r,record
    companions=['layout.c','ponteiro.c','fileiras.c','dados.c','linguas.c','fontefalha.c','fonteregra.c','regexjs.c','js.c']
    for sanitize in [False,True]:
        mode='asan-ubsan' if sanitize else 'normal';leaf_results={}
        for leaf,module in [('interface_layout',None),('agenda_runtime','agendaui.c'),('busca_runtime','busca.c'),('settings_runtime','ajustes.c'),('scroll_runtime',None)]:
            for touch in ([True] if leaf=='scroll_runtime' else [False,True]):
                label=mode+'-'+leaf+('-on' if touch else '-off')
                modules=['layout.c']
                if leaf=='settings_runtime':modules+=['fontefalha.c','fonteregra.c','linguas.c','regexjs.c','js.c']
                binary=build(label,leaf+'.c',modules,touch,sanitize,False,module)
                r,record=run(binary,[],label)
                leaf_runs.append(record);leaf_results[leaf,touch]=r.stdout
            if leaf in ['agenda_runtime','busca_runtime','settings_runtime']:
                baseline=leaf_results[leaf,False].strip()
                assert baseline.startswith('TV ')
                assert all(line==baseline for line in leaf_results[leaf,True].strip().splitlines()),leaf
        values=build(mode+'-values','settings-values.c',companions,True,sanitize,True)
        for option in [43,137,192,279]:
            for portrait in [0,1]:
                for size in [0,1,2]:
                    label=f'{mode}-option{option}-portrait{portrait}-size{size}'
                    r,record=run(values,list(map(str,[option,0,portrait,size])),label)
                    assert 'PASS ' in r.stdout
        notice=build(mode+'-notice','settings-save-failure.c',companions,True,sanitize,True)
        r,record=run(notice,[],mode+'-notice-six-geometries')
        assert r.stdout.count('PASS real write failure rolls back; Cancel preserves file; retry saves/reloads')==6
        assert 'RESULT hidden-storage-error-notices 0 of6' in r.stdout
        notice_runs.append(record)
        r,record=run(notice,['--tvtrace'],mode+'-notice-tv-trace')
        assert r.stdout.count('TVTRACE ')==6
    verify(source,a.proof)
    receipt={'outcome':'passed','source_binding':proof,'compiler_version':subprocess.check_output(['cc','--version'],text=True).splitlines()[0],
        'settings_apply_cancel_persistence_cases':48,'storage_error_geometry_cases':12,'tv_notice_observations':12,
        'mode_leaf_executions':len(leaf_runs),'normal_and_sanitizer_modes':['normal','address+undefined'],
        'compiled':compile_records,'runs':runs,'reused_test_passes':False,
        'scope':'Fresh exact-source CPU fixtures: production Settings/pointer/layout/storage, Agenda and Search geometry and TV round trips. SDK headers declare types only; offline identity/provider/rendering services are explicit boundaries; unexpected dependencies abort. No GPU/device/full-app execution claim.'}
    (out/'runtime-verified.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print('Passed fresh exact-source CPU UI runtime/TV-mode fixtures')

if __name__=='__main__':main()
