"""Fresh Settings/profile/view regressions for the two-file #527 rebase conflict."""
from pathlib import Path
import argparse,hashlib,json,os,subprocess,sys
from common import source_binding

parser=argparse.ArgumentParser();parser.add_argument('--source',type=Path,required=True);parser.add_argument('--proof',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--cc',default='cc');args=parser.parse_args()
source=args.source.resolve();recipe=Path(__file__).resolve().parent;output=args.output.resolve()
assert not output.is_relative_to(recipe)
output.mkdir(parents=True,exist_ok=True);assert not list(output.iterdir()),'Require empty fresh output'
digest=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
plan,binding,manifest=source_binding(source,args.proof)
git=lambda *a:subprocess.check_output(['git','-C',str(source),*a],text=True).strip()
head,tree=git('rev-parse','HEAD'),git('rev-parse','HEAD^{tree}')
fixtures={str(p.relative_to(recipe)):digest(p)for p in recipe.rglob('*')if p.is_file()and '__pycache__'not in p.parts}
env={**os.environ,'PYTHONDONTWRITEBYTECODE':'1'}

def run(command,log):
    with log.open('w')as stream:result=subprocess.run(command,stdout=stream,stderr=subprocess.STDOUT,env=env,timeout=900)
    if result.returncode:print(log.read_text(errors='replace')[-16000:],flush=True)
    assert result.returncode==0,(command,result.returncode)

alignment=output/'settings-alignment.json'
run([sys.executable,str(recipe/'settings_layout.py'),'--source',str(source),'--output',str(alignment),'--cc',args.cc],output/'settings-alignment.log')
aligned=json.loads(alignment.read_text());assert aligned['passed']and aligned['source_commit']==head and aligned['tree']==tree and len(aligned['modes'])==4
records=[]
for group in ['settings','collection-state']:
    for mode in ['normal','asan']:
        directory=output/(group+'-'+mode);log=output/(group+'-'+mode+'.log')
        command=[sys.executable,str(recipe/group/'run.py'),'--source',str(source),'--output',str(directory),'--cc',args.cc]
        if mode=='asan':command.append('--sanitize')
        run(command,log)
        receipt=directory/'receipt.json';raw=json.loads(receipt.read_text())
        assert raw['passed']and raw['source_commit']==head and raw['source_tree']==tree and raw['source_clean']
        wanted={'ui','profile'}if group=='settings'else{'views','adapted-original'}
        assert {r['group']for r in raw['records']}==wanted and len(raw['records'])==2
        inner=[]
        for record in raw['records']:
            child=Path(record['receipt']);assert child.is_file()and child.resolve().is_relative_to(directory)
            assert digest(child)==record['receipt_sha256']
            value=json.loads(child.read_text())
            assert value['passed']and value['source_commit']==head and value['source_tree']==tree and value['source_clean']
            assert value['sanitizers']==(['address','undefined']if mode=='asan'else[])and not value['private']and not value['abort_traps_fired']
            if group=='settings':
                assert value['fixture_assertions_unchanged']and len(value['cases'])==(36 if record['group']=='ui'else 1)
                assert all(case['returncode']==0 for case in value['cases'])
            else:
                assert value['assertions_preserved']
                if record['group']=='views':assert value['views_assertions']==8035
                else:assert value['original_assertion_sites_preserved']==47 and value['added_grid_assertion_sites']==1
            assert digest(child.parent/'probe')==value['binary_sha256']
            for name,expected in value['logs_sha256'].items():assert digest(child.parent/name)==expected
            inner.append({'group':record['group'],'receipt':str(child.relative_to(output)),'receipt_sha256':digest(child)})
        records.append({'group':group,'mode':mode,'command':command,'producer_log':log.name,'producer_log_sha256':digest(log),'receipt':str(receipt.relative_to(output)),'receipt_sha256':digest(receipt),'raw_records':inner})
source_binding(source,args.proof)
assert git('rev-parse','HEAD')==head and git('rev-parse','HEAD^{tree}')==tree and not git('status','--porcelain')
for relative,expected in fixtures.items():assert digest(recipe/relative)==expected
artifacts={str(p.relative_to(output)):digest(p)for p in output.rglob('*')if p.is_file()and p.suffix!='.o'}
result={'passed':True,'pr':527,'source_commit':head,'source_tree':tree,'source_binding':binding,'source_proof_sha256':digest(args.proof),'compiler':args.cc,'records':records,'alignment_receipt':'settings-alignment.json','alignment_sha256':digest(alignment),'fixture_inputs_sha256':fixtures,'artifacts_sha256':artifacts,'all_original_fixture_assertions_preserved':True,'retained_backend_behavior_not_claimed_as_fresh_execution':True,'previous_passes_reused_as_fresh':False}
(output/'receipt.json').write_text(json.dumps(result,indent=2)+'\n')
print('PASS #527 Settings/profile/collection-state normal+ASan and four-platform positional Settings alignment',flush=True)
