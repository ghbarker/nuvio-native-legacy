"""Two fresh, source-bound normal/ASan groups for retained #504/#365 cases."""
from pathlib import Path
import argparse,hashlib,json,re,subprocess,sys

HERE=Path(__file__).resolve().parent
sys.path.insert(0,str(HERE.parents[1]))
from common import source_binding
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
p=argparse.ArgumentParser()
p.add_argument('--source',type=Path,required=True)
p.add_argument('--proof',type=Path,required=True)
p.add_argument('--output',type=Path,required=True)
p.add_argument('--cc',default='/usr/bin/cc')
a=p.parse_args();src=a.source.resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
plan,binding,manifest=source_binding(src,a.proof)
origin=json.loads((HERE/'provenance.json').read_text())
assert origin['candidate']==manifest['head']and origin['tree']==manifest['tree']and origin['base']==manifest['base']
for name,digest in origin['assertion_fixtures_unchanged'].items():assert sha(HERE/name)==digest,name
groups=[]
for mode in ['normal','asan']:
    dest=out/mode;log=out/(mode+'.log')
    command=[sys.executable,str(HERE/'run.py'),'--source',str(src),'--out',str(dest),'--cc',a.cc,
             *(['--sanitize']if mode=='asan'else[])]
    with log.open('w')as stream:result=subprocess.run(command,stdout=stream,stderr=subprocess.STDOUT,timeout=600)
    if result.returncode:print('\n'.join(log.read_text().splitlines()[-70:]),flush=True)
    assert result.returncode==0,(mode,result.returncode)
    raw=dest/'receipt.json';report=json.loads(raw.read_text())
    assert report['passed']and report['all_passed']and report['source_clean']
    assert report['source_commit']==manifest['head']and report['source_tree']==manifest['tree']
    assert report['case_count']==21 and len(report['cases'])==21
    assert report['sanitized']==(mode=='asan')
    assert report['sanitizers']==(['address','undefined']if mode=='asan'else[])
    assert len(report['records'])==25 and all(r['exit_code']==0 and r['expectation_met']for r in report['records'])
    assert [r['name']for r in report['records'][4:]]==report['cases']
    for path,digest in report['dependency_sha256'].items():assert sha(path)==digest,path
    for path,digest in report['raw_files_sha256'].items():assert sha(dest/path)==digest,path
    assert report['extracted_sha256']==origin['extracted_sha256']
    for path,digest in report['extracted_sha256'].items():assert sha(dest/path)==digest,path
    assert sha(dest/report['binary'])==report['binary_sha256']
    full=(dest/'full-tmdb.log').read_text()
    phases={}
    for line in full.splitlines():
        if line.startswith('PASS tmdb-full '):
            values=dict(re.findall(r'([a-z_]+)=([^ ]+)',line))
            phase=values.pop('phase');assert phase not in phases;phases[phase]=values
    assert list(phases)==origin['full_case_phases']
    for phase,values in phases.items():
        assert values['known']=='768'and values['other_home']=='379'and values['other_outside']=='379'
        assert values['preferences']=='byte-identical'and values['actual_projection']=='both'
    assert phases['installed-eight']['tmdb_home']==phases['installed-eight']['tmdb_outside']=='5'
    assert phases['removed-seven']['tmdb_home']==phases['removed-seven']['tmdb_outside']=='0'
    assert phases['reinstalled-eight']['tmdb_home']==phases['reinstalled-eight']['tmdb_outside']=='5'
    assert phases['installed-disabled']['tmdb_home']=='0'and phases['installed-disabled']['tmdb_outside']=='10'
    assert phases['disabled-unknown-id']['tmdb_outside']=='10'
    assert phases['changed-generation-deferred']['tmdb_home']==phases['changed-generation-deferred']['tmdb_outside']=='5'
    assert phases['stable-generation-replay']['tmdb_home']==phases['stable-generation-replay']['tmdb_outside']=='0'
    groups.append({'name':'removed-addon-rows-'+mode,'cases':21,'sanitize':mode=='asan',
                   'receipt':str(raw.relative_to(out)),'receipt_sha256':sha(raw),
                   'binary':str((dest/report['binary']).relative_to(out)),
                   'binary_sha256':report['binary_sha256'],'command':command,
                   'execution_log':log.name,'execution_log_sha256':sha(log),'full_case_phases':phases})
source_binding(src,a.proof)
raw={str(path.relative_to(out)):sha(path)for path in sorted(out.rglob('*'))if path.is_file()and path.suffix!='.o'}
receipt={'passed':True,'outcome':'passed','issues':[504,365],'source_commit':manifest['head'],
         'source_tree':manifest['tree'],'source_binding':binding,'proof_sha256':sha(a.proof),
         'groups':groups,'group_count':2,'cases_each':21,'fixture_provenance_sha256':sha(HERE/'provenance.json'),
         'raw_files_sha256':raw,'scope':'Fresh21retained CPU runtime cases each normal/ASan; full768-row two-list Settings projections,10removed rows,758preserved rows, preferences/reinstall/generation/profile guards. Complete real row store plus verbatim actual Settings/discovery code. No SDK, GUI, network or physicalscreen claim.'}
(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
print('PASS #504/#365:21retained cases each normal/ASan;768rows/both actual Settings lists',flush=True)
