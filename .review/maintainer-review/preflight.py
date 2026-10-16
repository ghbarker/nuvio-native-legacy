"""Validate only the independently reviewed metadata candidates and exact public suites."""
from pathlib import Path
import ast,hashlib,json,os,subprocess
from common import HERE,approved,sha

p=approved()
m=json.loads((HERE/'source-candidates.json').read_text())
workspace=Path(os.environ['GITHUB_WORKSPACE'])
w=json.loads((workspace/p['workflow_path']).read_text())
selected=p['selected_candidates']
assert selected and set(selected)=={'512'} and sorted(m)==sorted(selected)
assert p['job_candidates']=={'native':selected,'android':[]}
active={name:values for name,values in p['job_candidates'].items() if values}
assert set(w['jobs'])==set(active)=={'native'}
assert p['required_jobs']==['native'] and p['sdk_producers']==sum(map(len,active.values()))==len(selected)
assert w['on']['push']['branches']==[p['review_branch']]
assert w['env']['REBASE_PLAN_SHA256']==sha(HERE/'plan.json')
normalized=json.loads(json.dumps(w))
normalized['env']['REBASE_PLAN_SHA256']='<PLAN_SHA256>'
cache_counts=[]
for name,job in w['jobs'].items():
    assert job['if'] is True and job['strategy']['fail-fast'] is False
    assert job['strategy']['matrix']=={'pr':active[name]}
    assert job['env']['REBASE_PR']=='${{ matrix.pr }}'
    normalized['jobs'][name]['if']='<ROOT_GATE>'
    uploads=[s for s in job['steps'] if s.get('uses','').startswith('actions/upload-artifact@')]
    assert len(uploads)==2
    assert any('-proof-' in s['with']['name'] and '!results/**/*.apk' in s['with']['path'] for s in uploads)
    for step in job['steps']:
        if step.get('uses','').startswith(('actions/cache@','actions/cache/restore@')):
            keys=[step['with']['key']]+[x.strip() for x in step['with'].get('restore-keys','').splitlines() if x.strip()]
            assert len(keys)<=10 and len(keys)==len(set(keys))
            cache_counts.append(len(keys))
assert cache_counts==[]
assert hashlib.sha256(json.dumps(normalized,sort_keys=True,separators=(',',':')).encode()).hexdigest()==p['workflow_normalized_sha256']
override=p['inherited_workflow_override']
assert sha(workspace/override['path'])==override['review_sha256']
upstream=subprocess.check_output(['git','show',p['current_master']+':'+override['path']])
assert hashlib.sha256(upstream).hexdigest()==override['upstream_sha256']
assert (workspace/override['path']).read_bytes()==upstream.replace(b'  push:\n',b'  push:\n    branches-ignore:\n      - '+p['review_branch'].encode()+b'\n',1)
for key,source in m.items():
    assert str(source['pr'])==key and source['base']==p['current_master']
    assert source['source_identity_kind']=='actual-remote' and p['remote_source_pins_ready']
    assert source['fresh_sdk_required'] is True
    assert source['native_translation_units'] and source['compiled_source_inputs'] and source['rust_inputs']
    assert source['head']==source['public_test_commit'] and source['parents']==[source['product_head']]
    assert subprocess.check_output(['git','rev-parse',source['head']+':'+override['path']],text=True).strip()==override['upstream_blob']
    delta=subprocess.check_output(['git','diff','--name-only',source['product_head'],source['head']],text=True).splitlines()
    assert delta==source['public_test_commit_paths'] and delta and all(x.startswith('tests/')for x in delta)
    assert source['public_suites']
    for suite in source['public_suites']:
        assert suite['script'].startswith('tests/') and suite['script'].endswith('.sh') and suite['inputs']
        assert set(suite['modes'])<={'normal','asan'}
        for variant in suite.get('variants',[{}]):assert {**suite,**variant}['required_markers']
for name in p['recipe_inputs']:
    if name.endswith('.py'):ast.parse((HERE/name).read_text(),filename=name)
print(json.dumps({'source_bound':True,'selected_candidates':selected,'sdk_producers':len(selected),
    'cache_keys':cache_counts,'unselected_original_producers_not_rerun':True,'replacement_only':True}))
