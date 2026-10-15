"""Fail closed on ten fresh SDK producers and their source-bound raw evidence."""
from pathlib import Path
import argparse, hashlib, json, os, re, urllib.request
from common import HERE, approved, sha

PRS = ['431', '526', '527', '533', '536']
BAD = ['ERROR: AddressSanitizer', 'runtime error:', 'UNEXPECTED dependency:']

def load(path):
    return json.loads(Path(path).read_text())

def inside(root, relative):
    path = (root / relative).resolve()
    assert path.is_relative_to(root.resolve()) and path.is_file(), str(path)
    return path

def hashed(root, relative, digest):
    path = inside(root, relative)
    assert re.fullmatch('[0-9a-f]{64}', digest or '') and sha(path) == digest, str(path)
    return path

def maps(root, values):
    assert isinstance(values, dict) and values
    for relative, digest in values.items(): hashed(root, relative, digest)

def raw_json(root, relative, digest):
    return load(hashed(root, relative, digest))

def source(value, manifest):
    assert value['verified'] and value['source_clean']
    assert str(value['candidate_pr']) == str(manifest['pr'])
    assert value['source_commit'] == manifest['head'] and value['tree'] == manifest['tree']
    assert value['base'] == manifest['base'] == PLAN['base']
    assert value['source_parents'] == manifest['parents']
    assert value['manifest_sha256'] == PLAN['source_manifest_sha256']
    assert value['review_commit'] == REVIEW and value['workflow_run_id'] == RUN
    assert value['workflow_run_attempt'] == ATTEMPT

def bound(value, binding):
    assert value['source_binding'] == binding
    assert value['plan_sha256'] == PLAN_HASH and value['outcome'] == 'passed'

def current(value, manifest):
    assert value['source_commit'] == manifest['head']
    assert value.get('source_tree', value.get('tree')) == manifest['tree']

def record(root, row):
    assert row['returncode'] == 0 and row['command']
    text = hashed(root, row['log'], row['log_sha256']).read_text(errors='replace')
    assert not any(x in text for x in BAD), row['log']
    return text

def sanitizer(commands, mode):
    assert commands
    flat = [str(token) for command in commands for token in command]
    assert not any('NV_TEST_LOCAL_LOGS' in x for x in flat)
    present = any(x.startswith('-fsanitize=') and 'address' in x and 'undefined' in x for x in flat)
    assert present == (mode == 'asan'), mode

def api(path):
    req = urllib.request.Request('https://api.github.com/repos/' + os.environ['GITHUB_REPOSITORY'] + '/' + path,
        headers={'Authorization': 'Bearer ' + os.environ['GITHUB_TOKEN'], 'Accept': 'application/vnd.github+json', 'X-GitHub-Api-Version': '2022-11-28'})
    with urllib.request.urlopen(req, timeout=60) as response: return json.load(response)

def pages(path, key):
    result = []
    for page in range(1, 20):
        rows = api(path + ('&' if '?' in path else '?') + 'per_page=100&page=' + str(page))[key]
        result += rows
        if len(rows) < 100: return result
    raise AssertionError('Unbounded API pagination')

def kotlin(root, binding, manifest):
    value = load(root / 'kotlin-verified.json'); bound(value, binding)
    assert value['groups'] == ['interface-mode', 'registro-download']
    assert [x['name'] for x in value['gates']] == ['kotlin-version', 'interface-mode', 'registro-download']
    logs = [record(root, row) for row in value['gates']]
    assert 'Android interface: all TV signals' in logs[1] and 'PASS' in logs[1]
    assert 'registro download: recreation, export, redaction, cancellation, failure OK' in logs[2] and 'registro download: Activity saved-state wiring OK' in logs[2], 'The actual upstream download regression must run'
    assert value['assertion_source_sha256']['interface-mode'] == sha(HERE / 'fixtures/pr431/android_interface_mode.py')
    expected = next(x['sha256'] for x in manifest['reviewed_test_paths'] + manifest['paths'] if x['path'] == 'tests/registro_download.py') if any(x['path'] == 'tests/registro_download.py' for x in manifest['reviewed_test_paths'] + manifest['paths']) else None
    if expected: assert value['assertion_source_sha256']['registro-download'] == expected
    adapter=value['registro_adapter']
    assert adapter['python_assertions_unchanged'] and adapter['embedded_kotlin_sources_unchanged'] and adapter['assertion_count']==3
    assert adapter['original_sha256']==value['assertion_source_sha256']['registro-download']
    hashed(root,'registro-download-adapted.py',adapter['generated_sha256'])
    hashed(root,'registro-download-launch.py',adapter['launcher_sha256'])
    for key in ['compiler_jar_sha256','stdlib_sha256']:assert re.fullmatch('[0-9a-f]{64}',adapter[key])
    for key in ['compiler_sha256', 'java_sha256']: assert re.fullmatch('[0-9a-f]{64}', value[key])
    return {'groups': 2, 'receipt_sha256': sha(root / 'kotlin-verified.json')}

def focused431(root, q, binding, manifest):
    assert q['outcome'] == 'passed' and q['source_binding'] == binding
    assert q['fresh_normal_and_sanitizers'] and q['compiled_fixtures'] == 22 and q['executed_cases'] == 70
    hashed(root, q['runtime_log'], q['runtime_log_sha256'])
    v = raw_json(root, q['runtime_receipt'], q['runtime_receipt_sha256']); runtime = root / 'runtime'
    assert v['outcome'] == 'passed' and v['source_binding'] == binding and not v['reused_test_passes']
    assert v['normal_and_sanitizer_modes'] == ['normal', 'address+undefined']
    for key, count in [('settings_apply_cancel_persistence_cases',48), ('storage_error_geometry_cases',12), ('tv_notice_observations',12), ('mode_leaf_executions',18)]: assert v[key] == count
    assert len(v['compiled']) == 22 and len(v['runs']) == 70
    assert len({x['label'] for x in v['compiled']}) == 22 and len({x['name'] for x in v['runs']}) == 70
    for row in v['compiled']:
        label = row['label']; mode = 'asan' if label.startswith('asan-ubsan-') else 'normal'
        assert row['sanitizers'] == (['address','undefined'] if mode == 'asan' else [])
        hashed(runtime, label + '/fixture', row['binary_sha256'])
        for dep in row['dependency_files']: hashed(runtime, dep['path'], dep['sha256'])
    for row in v['runs']:
        assert row['returncode'] == 0
        text = hashed(runtime, row['name'] + '.log', row['log_sha256']).read_text(errors='replace')
        assert not any(x in text for x in BAD)
    return {'compiled_fixtures':22, 'executed_cases':70, 'settings_cases':48, 'storage_cases':12, 'tv_observations':12}

def focused527(root, q, binding, manifest):
    assert q['passed'] and q['source_binding'] == binding and q['all_original_fixture_assertions_preserved']
    assert q['retained_backend_behavior_not_claimed_as_fresh_execution'] and not q['previous_passes_reused_as_fresh']
    maps(root, q['artifacts_sha256'])
    align = raw_json(root, q['alignment_receipt'], q['alignment_sha256'])
    current(align, manifest); assert align['passed'] and len(align['modes']) == 4
    assert {(x['group'],x['mode']) for x in q['records']} == {(g,m) for g in ['settings','collection-state'] for m in ['normal','asan']}
    assert len(q['records']) == 4
    for row in q['records']:
        hashed(root, row['producer_log'], row['producer_log_sha256'])
        raw_json(root, row['receipt'], row['receipt_sha256'])
        wanted = {'ui','profile'} if row['group'] == 'settings' else {'views','adapted-original'}
        assert {x['group'] for x in row['raw_records']} == wanted and len(row['raw_records']) == 2
        for child in row['raw_records']:
            path = hashed(root, child['receipt'], child['receipt_sha256']); v = load(path)
            current(v, manifest); assert v['passed'] and v['source_clean'] and not v['private'] and not v['abort_traps_fired']
            assert v['sanitizers'] == (['address','undefined'] if row['mode'] == 'asan' else [])
            sanitizer(v['commands'], row['mode']); hashed(path.parent, 'probe', v['binary_sha256']); maps(path.parent, v['logs_sha256'])
            if row['group'] == 'settings':
                assert v['fixture_assertions_unchanged'] and len(v['cases']) == (36 if child['group'] == 'ui' else 1)
                assert all(x['returncode'] == 0 for x in v['cases'])
                marker='PASS global8choice checks=235' if child['group']=='ui' else '133 assertions, 17 profile changes'
                assert all(marker in x['stdout_last'] for x in v['cases'])
            else:
                assert v['assertions_preserved']
                if child['group'] == 'views': assert v['views_assertions'] == 8035
                else: assert v['original_assertion_sites_preserved'] == 47 and v['added_grid_assertion_sites'] == 1
    return {'mode_groups':4,'settings_cases_per_mode':36,'profile_cases_per_mode':1,'profile_assertions_per_mode':133,'view_assertions_per_mode':8035,'historical_original_sites_per_mode':47,'grid_sites_per_mode':1,'positional_settings_modes':4}

def focused533(root, q, binding, manifest):
    assert q['passed'] and q['source_binding'] == binding and q['group_count'] == len(q['groups']) == 18
    names = ['core','parser','planner','worker','ui-handlers','preparation','concurrency','profile-bank','profile-scope']
    assert {(x['name'],x['mode']) for x in q['groups']} == {(n,m) for n in names for m in ['normal','asan']}
    hashed(root, 'current-ui.c', q['generated_ui_sha256']); hashed(root, 'current-ui.manifest.json', q['ui_manifest_sha256'])
    for row in q['groups']:
        directory = root / row['directory']; sanitizer([row['compile']['command']], row['mode'])
        record(directory,row['compile']); record(directory,row['run']); hashed(directory,'fixture',row['binary_sha256'])
    return {'groups_per_mode':9,'normal_and_asan_groups':18}

def focused536(root, q, binding, manifest):
    assert q['passed'] and q['source_binding'] == binding and q['group_count'] == len(q['groups']) == 2
    assert {x['name'] for x in q['groups']} == {'settings-menu-normal','settings-menu-asan'}
    maps(root,q['raw_files_sha256'])
    for row in q['groups']:
        v = raw_json(root,row['receipt'],row['receipt_sha256']); current(v,manifest)
        mode = 'asan' if row['name'].endswith('-asan') else 'normal'
        assert v['passed'] and v['outcome'] == 'passed' and v['kind'] == 'menu' and not v['private'] and not v['abort_traps_fired']
        assert v['sanitizers'] == (['address','undefined'] if mode == 'asan' else [])
        assert row['cases'] == len(v['cases']) == 6 and all(x['actual_returncode'] == x['returncode'] == 0 for x in v['cases'])
        sanitizer(v['commands'],mode); hashed(root,row['binary'],row['binary_sha256']); hashed(root,row['execution_log'],row['execution_log_sha256'])
        raw_json(root,row['root_receipt'],row['root_receipt_sha256'])
    return {'normal_and_asan_groups':2,'geometry_cases_per_mode':6}

def focused(root,binding,manifest):
    v = load(root/'focused-verified.json'); bound(v,binding); assert v['candidate_pr'] == manifest['pr']
    record(root,v['gate']); assert v['entry_sha256'] == sha(HERE/'fixtures'/('pr'+str(manifest['pr']))/'entry.py')
    maps(root,v['raw_json_sha256']); maps(root,v['raw_logs_sha256'])
    q = raw_json(root,v['receipt_relative'],v['receipt_sha256']); current(q,manifest)
    return {431:focused431,527:focused527,533:focused533,536:focused536}[manifest['pr']](root/'raw',q,binding,manifest)

def native(root,binding,manifest):
    v=load(root/'native-verified.json');bound(v,binding);assert v['candidate_pr']==manifest['pr']
    assert [x['ui'] for x in v['full_apps']]==['tv','touch']
    for row in v['gates']:record(root,row)
    assert {'sdk-compile','sdk-run','languages','link-tv','link-touch'} <= {x['name'] for x in v['gates']}
    counts={}
    for app in v['full_apps']:
        binary=hashed(root,'app-'+app['ui'],app['sha256']);assert binary.stat().st_size==app['bytes']>0
        assert app['translation_units']==len(app['compile_records']) and app['translation_units']>100
        filenames=[]
        for row in app['compile_records']:
            record(root/('objects-'+app['ui']),row); cmd=row['command'];assert '-c' in cmd and not any('NV_TEST_LOCAL_LOGS' in x for x in cmd)
            filenames.append(Path(cmd[cmd.index('-c')+1]).name)
            assert ('-DNV_TOUCH_UI=1' in cmd)==(app['ui']=='touch')
        assert {'main.c','descoberta.c','vertudo.c','ajustes.c','sync.c'}<=set(filenames)
        counts[app['ui']]=app['translation_units']
    evidence=None
    if manifest['pr']==526:
        assert v['focused'];q=raw_json(root,v['focused']['receipt_relative'],v['focused']['receipt_sha256']);current(q,manifest)
        assert q['success'] and q['candidate_pr']==526 and q['base']==manifest['base'] and q['review_commit']==REVIEW
        assert q['manifest_sha256']==PLAN['source_manifest_sha256'] and len(q['runs'])==2
        assert {x['name'] for x in q['runs']}=={'normal','asan-ubsan'}
        for row in q['runs']:
            directory=root/'features';text=record(directory,row);assert row['case_passes']==text.splitlines().count('synopsis_review: PASS')==2
            calls=[json.loads(x) for x in hashed(directory,row['compile_invocations'],row['compile_invocations_sha256']).read_text().splitlines()]
            assert calls==row['compile_calls'] and len(calls)==1 and calls[0]['exit']==0 and row['test_assertions_unchanged']
            call=calls[0];assert re.fullmatch('[0-9a-f]{64}',call['binary_sha256']) and call['binary_bytes']>0
            if row['name']=='asan-ubsan':assert call['sanitizers_injected']==['-fsanitize=address,undefined','-fno-omit-frame-pointer'] and call['nm_exit']==0 and call['asan_runtime_symbol'] and call['ubsan_runtime_symbol']
            else:assert call['sanitizers_injected']==[]
        evidence={'normal_and_asan_groups':2,'synopsis_cases_per_mode':2}
    else:assert v['focused'] is None
    return {'full_app_translation_units':counts,'focused':evidence}

def android(root,binding,manifest):
    v=load(root/'android-verified.json');bound(v,binding)
    apk=hashed(root,'standard-validation.apk',v['apk_sha256']);assert apk.stat().st_size==v['apk_bytes']>0
    assert v['package']=='space.nuvio.nativelegacy' and v['private_logging_absent'] and v['native_16kb_pages']=='passed'
    assert v['version_code']==sum(int(x)*scale for x,scale in zip(v['version'].split('.'),[10000,100,1]))
    assert re.fullmatch('[0-9a-f]{64}',v['certificate_sha256']) and set(v['android_classes'])=={'activity','player'}
    assert v['all_player_jni_lookups'] and len(v['all_player_jni_lookups'])==len({tuple(x) for x in v['all_player_jni_lookups']})
    assert v['apk_dex_sha256'] and all(re.fullmatch('[0-9a-f]{64}',x) for x in v['android_classes'].values())
    assert [x['abi'] for x in v['actual_arm_compiles']]==['arm64-v8a','armeabi-v7a']
    changed={Path(x['path']).name for x in manifest['paths'] if x['path'].endswith('.c')}
    counts={}
    for arm in v['actual_arm_compiles']:
        assert arm['compile_databases'];counts[arm['abi']]=[]
        for database in arm['compile_databases']:
            f=raw_json(root,database['file'],database['sha256'])
            assert database['source_abi_compiler_bound'] and f['source_abi_build_bound'] and f['sysroot_bound'] and f['abi']==arm['abi']
            assert f['compile_database_sha256']==database['compile_database_sha256'] and f['cmake_cache_sha256']==database['cmake_cache_sha256']
            assert f['compiler']['ndk_version']=='27.2.12479018' and re.fullmatch('[0-9a-f]{64}',f['compiler']['sha256'])
            assert f['compiler']['name'].startswith('clang') and len(f['targets'])==1
            target=f['targets'][0];assert ('aarch64' in target) if arm['abi']=='arm64-v8a' else ('armv7' in target and 'androideabi' in target)
            units=f['source_relative_units'];assert len(units)==database['main_translation_units'] and len(units)>100
            assert changed<=set(map(PathName,units)) and {'vertudo.c','ajustes.c','perfis.c'}<=set(map(PathName,units))
            assert set(database['changed_translation_units'])==changed and all(x.startswith('src/') and '..' not in Path(x).parts for x in units)
            counts[arm['abi']].append(len(units))
    return {'apk_sha256':v['apk_sha256'],'apk_bytes':v['apk_bytes'],'arm_translation_units':counts,'jni_lookups':len(v['all_player_jni_lookups'])}

def PathName(value):return Path(value).name

def main():
    global PLAN, PLAN_HASH, REVIEW, RUN, ATTEMPT
    p=argparse.ArgumentParser();p.add_argument('--input',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    PLAN=approved();PLAN_HASH=sha(HERE/'plan.json');REVIEW=os.environ['GITHUB_SHA'];RUN=int(os.environ['GITHUB_RUN_ID']);ATTEMPT=int(os.environ['GITHUB_RUN_ATTEMPT'])
    assert PLAN['candidates']==PRS and PLAN['required_jobs']==['native','android']
    needs=json.loads(os.environ['REQUIRED_JOB_RESULTS']);assert set(needs)=={'native','android'} and all(x['result']=='success' for x in needs.values())
    manifests=load(HERE/'source-candidates.json');assert set(manifests)==set(PRS)
    run=api('actions/runs/'+str(RUN));assert run['head_sha']==REVIEW and run['run_attempt']==ATTEMPT
    jobs=pages('actions/runs/'+str(RUN)+'/attempts/'+str(ATTEMPT)+'/jobs','jobs')
    artifacts=pages('actions/runs/'+str(RUN)+'/artifacts','artifacts')
    result=[];base=a.input.resolve()
    expected_names={f'rebase-{pr}-{lane}-{REVIEW}-attempt{ATTEMPT}' for pr in PRS for lane in ['native','android']}
    assert {x.name for x in base.iterdir()}==expected_names
    for pr in PRS:
        m=manifests[pr];assert str(m['pr'])==pr
        pair={'pr':int(pr),'source_commit':m['head'],'tree':m['tree'],'base':m['base'],'producers':{}}
        for lane in ['native','android']:
            name=f'PR{pr} '+('macOS TV/Mobile' if lane=='native' else 'Android ARM32/ARM64')
            matches=[x for x in jobs if x['name']==name];assert len(matches)==1,name
            job=matches[0];assert job['head_sha']==REVIEW and job['run_id']==RUN and job['run_attempt']==ATTEMPT and job['status']=='completed' and job['conclusion']=='success',name
            artifact_name=f'rebase-{pr}-{lane}-{REVIEW}-attempt{ATTEMPT}'
            matches=[x for x in artifacts if x['name']==artifact_name and not x['expired']];assert len(matches)==1,artifact_name
            artifact=matches[0];assert artifact['workflow_run']['id']==RUN and artifact['workflow_run']['head_sha']==REVIEW
            assert artifact['size_in_bytes']>0 and re.fullmatch('sha256:[0-9a-f]{64}',artifact.get('digest') or '')
            directory=base/artifact_name;proof=load(directory/'source-verified.json');source(proof,m)
            evidence=native(directory/'native',proof,m) if lane=='native' else android(directory/'android',proof,m)
            if lane=='android' and pr!='526':evidence['focused']=focused(directory/'features',proof,m)
            if lane=='android' and pr=='431':evidence['kotlin']=kotlin(directory/'kotlin',proof,m)
            pair['producers'][lane]={'job_id':job['id'],'job_name':name,'conclusion':job['conclusion'],'run_id':RUN,'run_attempt':ATTEMPT,'review_commit':REVIEW,'artifact_id':artifact['id'],'artifact_name':artifact_name,'artifact_digest':artifact.get('digest'),'artifact_bytes':artifact['size_in_bytes'],'source_proof_sha256':sha(directory/'source-verified.json'),'verified':evidence,'raw_json_sha256':{str(x.relative_to(directory)):sha(x) for x in sorted(directory.rglob('*.json'))},'raw_logs_sha256':{str(x.relative_to(directory)):sha(x) for x in sorted(directory.rglob('*.log'))}}
        result.append(pair)
    output={'outcome':'passed','all_ten_fresh_producers_passed':True,'source_manifest_sha256':PLAN['source_manifest_sha256'],'plan_sha256':PLAN_HASH,'review_commit':REVIEW,'workflow_run_id':RUN,'workflow_run_attempt':ATTEMPT,'base':PLAN['base'],'candidates':result,'producer_count':10,'full_macos_apps':10,'standard_dual_arm_apks':5,'previous_passes_reused_as_fresh':False,'scope':'All ten current-review SDK producers passed; actual apps/APKs, raw focused normal/ASan and PR431 Kotlin logs independently verified. No device execution claim.'}
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(output,indent=2)+'\n');print('PASS all ten fresh SDK producer origins, five exact sources and raw assertion chains',flush=True)

if __name__=='__main__':main()
