#!/usr/bin/env python3
"""Publish one independently verified, compatible test APK to the fork only."""
import argparse
import hashlib
import json
import os
import re
import stat
import subprocess
import sys
import tempfile
import zipfile
from pathlib import Path, PurePosixPath

REPOSITORY = 'ghbarker/nuvio-native-legacy'
BUILD_REVIEW = '9204d4d71e8456e85ebfee329d7ee74609912262'
BUILD_RUN = 38679339730
PLAN_SHA = 'efac80b5b9436720e32e16a8124f9b49b8e71d7d70d244afe6d82640e804f48b'
PRODUCT = '68b520db152b7473be3500d04e01ca5bd4424719'
TEST = '260c83a604b2a35787714a2946566d1dfd049704'
CERTIFICATE = '862d7ef63a0dbde2929f6f231cae295510726d9ac28f9292496d2a38304be5e6'
HERE = Path(__file__).resolve().parent
OUT = Path('publication')


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def sha(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            h.update(block)
    return h.hexdigest()


def valid_digest(value, length=64):
    return isinstance(value, str) and re.fullmatch('[0-9a-f]{'+str(length)+'}', value) is not None


def validate_lock(lock):
    require(lock.get('format') == 1 and lock.get('root_reviewed') is True, 'Root-reviewed final release lock is required')
    require(lock['repository'] == REPOSITORY and lock['source_plan_sha256'] == PLAN_SHA, 'Wrong repository or source plan')
    require(lock['publisher_branch'] == 'review/logs8-publish-20261015', 'Wrong publication branch')
    require(lock['parent_review'] == BUILD_REVIEW, 'Wrong publisher base review')
    require(lock['original']['review_commit'] == BUILD_REVIEW and lock['original']['run_id'] == BUILD_RUN, 'Wrong original build origin')
    require(lock['original']['android_job_id'] == 116240061121, 'Wrong original Android job')
    require(lock['original']['excluded_native_job_id'] == 116240061458 and lock['original']['excluded_native_artifact_id'] == 11946857868, 'Wrong excluded native origin')
    require(lock['original']['overall_run_success_required'] is False, 'Original failed native must retain honest provenance')
    final = lock['final_validation']
    require(type(final['run_id']) is int and final['run_id'] > 0 and final['run_id'] != BUILD_RUN, 'Final successful validation run is not bound')
    require(valid_digest(final['review_commit'], 40) and final['review_commit'] != BUILD_REVIEW, 'Fresh native review is not bound')
    require(isinstance(final['workflow_path'], str) and final['workflow_path'].startswith('.github/workflows/') and final['workflow_path'].endswith('.yml'), 'Final validation workflow is not bound')
    for item in lock['artifacts'].values():
        require(type(item['id']) is int and item['id'] > 0 and type(item['bytes']) is int and item['bytes'] > 0, 'Artifact ID and exact ZIP size are required')
        require(valid_digest(item['sha256']) and isinstance(item['name'], str), 'Artifact digest and name are required')
    require(lock['artifacts']['all_gates']['name'] == 'logs8-all-gates-'+final['review_commit'], 'Wrong all-gates artifact name')
    require(set(lock['artifacts']) == {'all_gates','android_product','android_test'}, 'Expected final gate and separate standard/test APK origins')
    standard = lock['artifacts']['android_product']
    require(standard['name'] == 'logs8-android-'+BUILD_REVIEW and standard['id'] == 11947769079 and standard['sha256'] == 'f4e469ccca47189b67bc26286ffd51877b200a1001ce50277e1536d676adc99c', 'Wrong independently successful standard APK artifact')
    test_origin = lock['android_test_origin']
    require(type(test_origin['run_id']) is int and test_origin['run_id'] > 0 and test_origin['run_id'] != BUILD_RUN and valid_digest(test_origin['review_commit'],40) and test_origin['review_commit'] != BUILD_REVIEW, 'Fresh compatible Android run/review is not bound')
    require(type(test_origin['job_id']) is int and test_origin['job_id'] > 0 and test_origin['job_id'] not in (116240061121,116240061458), 'Fresh successful compatible Android job is not bound')
    require(lock['artifacts']['android_test']['name'] == 'logs8-android-test-'+test_origin['review_commit'], 'Compatible APK must come from the fresh successful Android test job')
    require(valid_digest(lock['apk']['sha256']) and type(lock['apk']['bytes']) is int and lock['apk']['bytes'] > 0, 'Actual compatible APK digest and size are not bound')
    require(lock['sources']['product']['commit'] == PRODUCT and lock['sources']['test']['commit'] == TEST, 'Wrong product/test source')
    require(lock['sources']['product']['parent'] == '3c28c0d73664fe82fabced499bd7171810e0c91b' and lock['sources']['test']['parent'] == PRODUCT, 'Wrong source parents')
    require(lock['identity'] == {'package':'space.nuvio.nativelegacy.touch','label':'Nuvio TV/Mobile Test','version_code':20018,'version_name':'2.0.18-test-logs8','certificate_sha256':CERTIFICATE}, 'Wrong update identity')
    require(lock['release']['tag'] == 'tv-mobile-test-logs-v8' and lock['release']['asset'] == 'Nuvio-TV-Mobile-Test-Logs8.apk' and lock['release']['prerelease'] is True, 'Only the Logs8 test prerelease is authorized')
    require(valid_digest(lock['release']['draft_notes_sha256']) and valid_digest(lock['source_materializer_sha256']), 'Recipe file bindings are missing')


def command(argv, *, cwd=None, env=None, input_text=None):
    result = subprocess.run(argv, cwd=cwd, env=env, input=input_text, text=True, capture_output=True)
    require(result.returncode == 0, 'Command failed: '+argv[0]+' '+(argv[1] if len(argv)>1 else ''))
    return result.stdout


def api(path, *, allow_missing=False, data=None):
    argv = ['gh','api','repos/'+REPOSITORY+'/'+path]
    if data is not None:
        argv += ['--method','PATCH','--input','-']
    result = subprocess.run(argv, input=json.dumps(data) if data is not None else None, text=True, capture_output=True)
    if result.returncode and allow_missing and '(HTTP 404)' in result.stderr:
        return None
    require(result.returncode == 0, 'GitHub API request failed for '+path)
    return json.loads(result.stdout)


def run_binding(run_id, review, *, successful, workflow=None):
    record = api('actions/runs/'+str(run_id))
    require(record['id'] == run_id and record['head_sha'] == review, 'Workflow run/source mismatch')
    require(record['repository']['full_name'] == REPOSITORY and record['head_repository']['full_name'] == REPOSITORY, 'Workflow came from another repository')
    require(record['status'] == 'completed', 'Validation run has not finished')
    if successful:
        require(record['conclusion'] == 'success', 'Final validation run did not pass')
    if workflow is not None:
        require(record['path'] == workflow, 'Wrong final validation workflow')
    return record


def job_binding(job_id, run_id, review, *, conclusion='success'):
    record = api('actions/jobs/'+str(job_id))
    require(record['id'] == job_id and record['run_id'] == run_id and record['head_sha'] == review, 'Job origin mismatch')
    require(record['status'] == 'completed' and record['conclusion'] == conclusion, 'Required job did not reach the expected terminal result')
    return record


def artifact_binding(item, run_id, review):
    record = api('actions/artifacts/'+str(item['id']))
    require(record['id'] == item['id'] and record['name'] == item['name'], 'Artifact identity mismatch')
    require(record.get('expired') is False, 'Required artifact expired')
    require(record.get('digest') == 'sha256:'+item['sha256'], 'Artifact digest mismatch')
    require(record['workflow_run']['id'] == run_id and record['workflow_run']['head_sha'] == review, 'Artifact source/run mismatch')
    if 'bytes' in item:
        require(record['size_in_bytes'] == item['bytes'], 'Artifact ZIP size mismatch')
    return record


def download(item, run_id, review, name):
    record = artifact_binding(item, run_id, review)
    path = OUT/'downloads'/name
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open('wb') as stream:
        result = subprocess.run(['gh','api','repos/'+REPOSITORY+'/actions/artifacts/'+str(item['id'])+'/zip'], stdout=stream, stderr=subprocess.PIPE)
    require(result.returncode == 0 and path.stat().st_size == record['size_in_bytes'] and sha(path) == item['sha256'], 'Downloaded artifact failed digest/size verification')
    return path


def archive(path):
    z = zipfile.ZipFile(path)
    names = z.namelist()
    require(sum(i.file_size for i in z.infolist()) < 2*1024**3, 'Unbounded artifact expansion')
    require(len(names) == len(set(names)), 'Duplicate ZIP entries')
    for info in z.infolist():
        p = PurePosixPath(info.filename)
        require(not p.is_absolute() and '..' not in p.parts and '\\' not in info.filename, 'Unsafe ZIP path')
        require(not stat.S_ISLNK(info.external_attr >> 16), 'ZIP symlink is forbidden')
    return z


def read_json(z, name):
    require(z.getinfo(name).file_size <= 8*1024*1024, 'Unexpectedly large proof JSON')
    data = z.read(name)
    return json.loads(data), hashlib.sha256(data).hexdigest()


def unique_name(z, basename):
    names = [name for name in z.namelist() if PurePosixPath(name).name == basename]
    require(len(names) == 1, 'Expected one '+basename)
    return names[0]


def verify_summary(lock, summary):
    final = lock['final_validation']
    require(summary.get('format') == 2 and summary['outcome'] == 'passed', 'Final all-gates proof is incomplete')
    require(summary['review_commit'] == final['review_commit'] and summary['plan_sha256'] == PLAN_SHA, 'All-gates review/plan mismatch')
    require(summary['sources'] == lock['sources'], 'All-gates source graph mismatch')
    origin = summary['validation_origin']
    require(origin['repository'] == REPOSITORY, 'Wrong validation repository')
    old = origin['original']
    require(old['run_id'] == BUILD_RUN and old['review_commit'] == BUILD_REVIEW and old['overall_run_success_required'] is False, 'Original validation origin was rewritten')
    excluded = origin['excluded_failed_native']
    require(excluded == {'run_id':BUILD_RUN,'job_id':116240061458,'artifact_id':11946857868,'never_reused':True}, 'Failed native result was not explicitly excluded')
    job_binding(116240061458, BUILD_RUN, BUILD_REVIEW, conclusion='failure')
    retained = old['retained_jobs']
    expected = {
        'kotlin-player':(116240061490,11947413334,'7e66e4977f322a0d9480d9886a29caa169004dbd31174d6acdfe9a6a7a9ede1d'),
        'original-fontepref':(116240061412,11947858007,'72b22554246a3911893f787d57ecff90bea2c2e89111b0205114c3f21aabaacc'),
        'original-fontepref_file':(116240061382,11947288910,'b21f6789a79832be256e592905a9953f7a301ab3ec6363c6f7e129e2da0f1436'),
        'original-player_retido':(116240061383,11947573752,'3a8b9a3f96e65e47e6d234bb90365e2b62e1b13012c3d5591bf76f362d59ac5a')}
    require(len(retained) == 4 and len({x['job_id'] for x in retained}) == 4, 'Exactly four successful original jobs must be retained')
    require({x['scope'] for x in retained} == set(expected), 'Unexpected retained job scopes')
    for entry in retained:
        require(entry['run_id'] == BUILD_RUN and entry['review_commit'] == BUILD_REVIEW and entry['conclusion'] == 'success', 'Retained job provenance changed')
        require((entry['job_id'],entry['artifact']['id'],entry['artifact']['sha256']) == expected[entry['scope']], 'Retained job/artifact differs from reviewed successful gates')
        metadata = job_binding(entry['job_id'], BUILD_RUN, BUILD_REVIEW)
        require(metadata['name'] == entry['job_name'], 'Retained job name mismatch')
        artifact_binding(entry['artifact'], BUILD_RUN, BUILD_REVIEW)
    standard = origin['retained_android_product_phase']
    require(standard['run_id'] == BUILD_RUN and standard['review_commit'] == BUILD_REVIEW and standard['job_id'] == 116240061121, 'Wrong original standard build origin')
    require(standard['job_conclusion'] == 'failure' and standard['step_conclusion'] == 'success' and standard['root_approved_successful_phase_reuse'] is True and standard['old_private_test_excluded'] is True, 'Original Android failure/private-test exclusion was rewritten')
    require(standard['step_number'] == 12 and standard['step_name'] == 'Build and verify clean standard product APK with both ARM cores', 'Wrong standard build phase')
    require(standard['apk_sha256'] == 'd20c026d1772f708475102ddef349fd5b7002bf5e52c4478b8eeb79da1439c3d', 'Wrong individually verified standard APK')
    old_android = job_binding(116240061121, BUILD_RUN, BUILD_REVIEW, conclusion='failure')
    steps = [x for x in old_android['steps'] if x['number'] == 12 and x['name'] == standard['step_name']]
    require(len(steps) == 1 and steps[0]['status'] == 'completed' and steps[0]['conclusion'] == 'success', 'Original standard build phase did not pass')
    require(all(standard['artifact'][k] == lock['artifacts']['android_product'][k] for k in ['id','name','sha256']), 'Original standard artifact differs from root lock')
    artifact_binding(standard['artifact'], BUILD_RUN, BUILD_REVIEW)
    for key,name in [('fresh_native','native'),('fresh_android_test','android-test')]:
        fresh = origin[key]
        expected_origin = final if key == 'fresh_native' else lock['android_test_origin']
        require(fresh['run_id'] == expected_origin['run_id'] and fresh['review_commit'] == expected_origin['review_commit'] and fresh['conclusion'] == 'success' and fresh['job_name'] == name, 'Fresh native/test-Android provenance changed')
        if key == 'fresh_android_test':
            require(fresh['job_id'] == expected_origin['job_id'], 'Fresh Android job differs from root lock')
        require(fresh['job_id'] not in (116240061458,116240061121) and fresh['artifact']['id'] not in (11946857868,11947769079), 'Failed old job/artifact cannot be reused as a fresh pass')
        metadata = job_binding(fresh['job_id'], fresh['run_id'], fresh['review_commit'])
        require(metadata['name'] == name, 'Fresh job name mismatch')
        artifact_binding(fresh['artifact'], fresh['run_id'], fresh['review_commit'])
        if key == 'fresh_android_test':
            require(all(fresh['artifact'][k] == lock['artifacts']['android_test'][k] for k in ['id','name','sha256']), 'Fresh compatible APK artifact differs from root lock')
    require(summary['raw_proof_files_unchanged'] is True and summary['old_failed_native_partial_outputs_reused'] is False, 'Raw validation origin integrity is incomplete')
    hashes = summary['proof_hashes']
    require(all(valid_digest(hashes[k]) for k in ['native','kotlin','historical_player15']), 'Missing native/Kotlin/runtime proof hashes')
    require(set(hashes['android']) == {'product','test'} and all(valid_digest(v) for v in hashes['android'].values()), 'Missing Android proof hashes')
    require(set(hashes['original3']) == {'fontepref','fontepref_file','player_retido'} and all(valid_digest(v) for v in hashes['original3'].values()), 'Missing original ownership sanitizer proofs')


def verify_android(lock, summary, z, layer):
    selected = []
    for name in z.namelist():
        if PurePosixPath(name).name == 'android-verified.json':
            record,digest = read_json(z,name)
            if record.get('layer') == layer:
                selected.append((name,record,digest))
            elif layer == 'test':
                require(False, 'Fresh test artifact contains another Android layer')
    require(len(selected) == 1, 'Expected exactly one verified '+layer+' APK')
    name,record,digest = selected[0]
    review = BUILD_REVIEW if layer == 'product' else lock['android_test_origin']['review_commit']
    require(record['outcome'] == 'passed' and digest == summary['proof_hashes']['android'][layer], 'Android proof was changed')
    require(record['source_commit'] == lock['sources'][layer]['commit'] and record['tree'] == lock['sources'][layer]['tree'] and record['review_commit'] == review, 'Android source/review mismatch')
    require(record['certificate_sha256'] == CERTIFICATE and record['native_16kb_pages'] == 'passed' and record['legacy_jni_overloads_preserved'], 'Android signing/native/JNI verification missing')
    arms = record['actual_arm_compiles']
    require({x['abi'] for x in arms} == {'arm64-v8a','armeabi-v7a'} and all(x['builds'] for x in arms), 'Both actual ARM compiles are required')
    require(all(b['main_sources'] > 0 and b['report_endpoint_configured'] is True and b['private_native_opt_in'] == (layer=='test') for arm in arms for b in arm['builds']), 'Native compile/reporting opt-in mismatch')
    require(record['file_provider']['nonexported'] and record['file_provider']['updater_path_only'], 'Unexpected file-provider scope')
    expected_package = lock['identity']['package'] if layer=='test' else 'space.nuvio.nativelegacy'
    require(record['package'] == expected_package and record['private_log_opt_in'] == (layer=='test'), 'Product/test isolation failed')
    if layer == 'test':
        for key in ['version_code','version_name']:
            require(record[key] == lock['identity'][key], 'Wrong compatible update version')
    folder = PurePosixPath(name).parent
    require(PurePosixPath(record['apk_name']).name == record['apk_name'], 'Unsafe APK filename')
    apk_name = str(folder/record['apk_name'])
    candidates = [x for x in summary['apks'] if x['layer'] == layer]
    require(len(candidates) == 1 and candidates[0] == {'layer':layer,'name':record['apk_name'],'sha256':record['apk_sha256'],'bytes':record['apk_bytes']}, 'All-gates APK binding differs')
    build,build_hash = read_json(z,str(folder/'build-receipt.json'))
    require(build['verified'] and build['source_restored_clean'] and build['verification_sha256'] == digest and build['apk_sha256'] == record['apk_sha256'], 'Android build/cleanup receipt is incomplete')
    require(build['source_commit'] == record['source_commit'] and build['tree'] == record['tree'] and build['review_commit'] == review and build['layer'] == layer, 'Android build receipt origin mismatch')
    h = hashlib.sha256(); size = 0
    destination = OUT/lock['release']['asset'] if layer=='test' else None
    with z.open(apk_name) as stream:
        target = destination.open('wb') if destination else None
        try:
            for block in iter(lambda: stream.read(1024*1024), b''):
                h.update(block);size += len(block)
                if target:
                    target.write(block)
        finally:
            if target:
                target.close()
    require(h.hexdigest() == record['apk_sha256'] and size == record['apk_bytes'], 'Actual APK bytes do not match the verified build')
    if layer=='test':
        require(h.hexdigest() == lock['apk']['sha256'] and size == lock['apk']['bytes'], 'Test APK differs from the root-reviewed digest/size')
    else:
        require(h.hexdigest() == 'd20c026d1772f708475102ddef349fd5b7002bf5e52c4478b8eeb79da1439c3d', 'Standard APK differs from the approved successful phase')
    return {'review_commit':review,'verification_sha256':digest,'build_receipt_sha256':build_hash,'apk_sha256':h.hexdigest(),'apk_bytes':size}


def verify_actual_apk(lock, source):
    sdk = Path(os.environ.get('ANDROID_HOME') or os.environ['ANDROID_SDK_ROOT'])/'build-tools/35.0.0'
    apk = OUT/lock['release']['asset']
    badging = command([str(sdk/'aapt'),'dump','badging',str(apk)])
    identity = lock['identity']
    require(re.search(r"^package: name='"+re.escape(identity['package'])+r"'",badging,re.M), 'Actual APK has the wrong package')
    require("versionCode='20018'" in badging and "versionName='"+identity['version_name']+"'" in badging, 'Actual APK has the wrong version')
    require(re.search(r"^application-label:'"+re.escape(identity['label'])+r"'",badging,re.M), 'Actual APK has the wrong app label')
    signing = command([str(sdk/'apksigner'),'verify','--verbose','--print-certs',str(apk)])
    certificates = re.findall(r'^Signer #\d+ certificate SHA-256 digest: ([0-9a-f]+)$',signing,re.M)
    require(certificates == [CERTIFICATE], 'Actual APK signature does not match the persistent test certificate')
    command([sys.executable,str(source/'tools/android-apk-pages.py'),str(apk.resolve())])
    with zipfile.ZipFile(apk) as z:
        for abi in ['arm64-v8a','armeabi-v7a']:
            for library in ['libmain.so','libSDL2.so','libcurl.so']:
                require('lib/'+abi+'/'+library in z.namelist(), 'Actual APK is missing an ARM library')
    return {'package':identity['package'],'label':identity['label'],'version_code':20018,'version_name':identity['version_name'],'certificate_sha256':CERTIFICATE,'native_16kb_pages':'passed','abis':['arm64-v8a','armeabi-v7a']}


def tag_binding(lock, source):
    tag = lock['release']['tag']
    existing = api('git/ref/tags/'+tag, allow_missing=True)
    if existing is None:
        with tempfile.TemporaryDirectory(prefix='logs8-git-auth-') as temporary:
            helper = Path(temporary)/'askpass'
            helper.write_text('#!/bin/sh\ncase "$1" in\n*Username*) printf "%s\\n" x-access-token ;;\n*Password*) printf "%s\\n" "$GH_TOKEN" ;;\n*) exit 1 ;;\nesac\n')
            helper.chmod(0o700)
            env = dict(os.environ,GIT_ASKPASS=str(helper),GIT_TERMINAL_PROMPT='0')
            command(['git','-c','credential.helper=','push','--porcelain','https://github.com/'+REPOSITORY+'.git','HEAD:refs/tags/'+tag],cwd=source,env=env)
        existing = api('git/ref/tags/'+tag)
    require(existing['ref'] == 'refs/tags/'+tag and existing['object']['type'] == 'commit' and existing['object']['sha'] == TEST, 'Existing tag points to a different object; no force update is allowed')


def verify_release(lock, record, notes, assets):
    require(record['tag_name'] == lock['release']['tag'] and record['target_commitish'] == TEST and record['name'] == lock['release']['title'] and record['prerelease'] is True, 'Existing release does not match the reviewed test release')
    require(record['body'].replace('\r\n','\n').rstrip() == notes.rstrip(), 'Existing release notes differ')
    actual = record['assets']
    require(len(actual) == len(assets) and {x['name'] for x in actual} == {p.name for p in assets}, 'Existing release asset set differs; no assets will be replaced')
    for path in assets:
        asset = next(x for x in actual if x['name'] == path.name)
        require(asset['state'] == 'uploaded' and asset['size'] == path.stat().st_size and asset.get('digest') == 'sha256:'+sha(path), 'Uploaded release asset failed digest/size verification')


def main():
    p = argparse.ArgumentParser();p.add_argument('--check-lock',action='store_true');p.add_argument('--publish',action='store_true');args=p.parse_args()
    lock_path = HERE/'release-lock.json';lock=json.loads(lock_path.read_text());validate_lock(lock)
    require(sha(HERE/'release-notes.md') == lock['release']['draft_notes_sha256'], 'Root delivery draft changed')
    if args.check_lock:
        print('Root release lock and delivery draft are structurally complete; no network or mutation performed.');return
    require(os.environ.get('GITHUB_REPOSITORY') == REPOSITORY and os.environ.get('GH_TOKEN'), 'Run this only in the fork with its scoped Actions token')
    require(os.environ.get('GITHUB_REF') == 'refs/heads/'+lock['publisher_branch'], 'Wrong publication branch')
    require(os.environ.get('LOGS8_RELEASE_LOCK_SHA') == sha(lock_path), 'Workflow/release-lock digest mismatch')
    require(valid_digest(os.environ.get('GITHUB_SHA'),40), 'Publication review commit is missing')
    plan_path=Path('.review/logs8/plan.json');require(sha(plan_path) == PLAN_SHA, 'Original frozen source plan changed')
    plan=json.loads(plan_path.read_text());require(plan['frozen'] and plan['sources'] == lock['sources'] and plan['identity'] == lock['identity'], 'Source plan graph/identity mismatch')
    materializer=Path('.review/logs8/materialize.py');require(sha(materializer) == lock['source_materializer_sha256'], 'Source materializer changed')
    OUT.mkdir(exist_ok=True)
    final=lock['final_validation'];run_binding(final['run_id'],final['review_commit'],successful=True,workflow=final['workflow_path'])
    run_binding(BUILD_RUN,BUILD_REVIEW,successful=False)
    gates_zip=download(lock['artifacts']['all_gates'],final['run_id'],final['review_commit'],'all-gates.zip')
    with archive(gates_zip) as z:
        summary,summary_hash=read_json(z,unique_name(z,'all-gates-verified.json'))
    verify_summary(lock,summary)
    test_origin = lock['android_test_origin']
    run_binding(test_origin['run_id'],test_origin['review_commit'],successful=False)
    android = {}
    for layer,origin_run,origin_review in [('product',BUILD_RUN,BUILD_REVIEW),('test',test_origin['run_id'],test_origin['review_commit'])]:
        android_zip=download(lock['artifacts']['android_'+layer],origin_run,origin_review,'android-'+layer+'.zip')
        with archive(android_zip) as z:
            android[layer]=verify_android(lock,summary,z,layer)
    # Only after the complete proof/actual APK digest gates, materialize source objects.
    command(['git','cat-file','-e',plan['upstream']['commit']+'^{commit}'])
    source=Path('publication-source').resolve();require(not source.exists(), 'Refusing to replace existing source materialization')
    command([sys.executable,str(materializer),'--layer','test','--output',str(source),'--proof',str((OUT/'materialized-test.json').resolve()),'--base-repository',str(Path.cwd())])
    require(command(['git','rev-parse','HEAD'],cwd=source).strip() == TEST and command(['git','rev-parse','HEAD^{tree}'],cwd=source).strip() == lock['sources']['test']['tree'] and command(['git','rev-parse','HEAD^'],cwd=source).strip() == PRODUCT, 'Materialized Git source object differs')
    identity=verify_actual_apk(lock,source)
    asset=OUT/lock['release']['asset'];checksum=OUT/(asset.name+'.sha256');checksum.write_text(sha(asset)+'  '+asset.name+'\n')
    validation={'outcome':'passed','repository':REPOSITORY,'source_commit':TEST,'source_tree':lock['sources']['test']['tree'],'product_commit':PRODUCT,'validation_review_commit':final['review_commit'],'publication_review_commit':os.environ['GITHUB_SHA'],'source_plan_sha256':PLAN_SHA,'release_lock_sha256':sha(lock_path),'all_gates_sha256':summary_hash,'validation_origin':summary['validation_origin'],'apk_sha256':sha(asset),'apk_bytes':asset.stat().st_size,'identity':identity,'android_receipts':android,'private_configs_or_logs_uploaded':False}
    proof=OUT/'Logs8-validation.json';proof.write_text(json.dumps(validation,indent=2)+'\n')
    url='https://github.com/'+REPOSITORY+'/releases/download/'+lock['release']['tag']+'/'+asset.name
    notes=(HERE/'release-notes.md').read_text().replace('[insert verified APK link after CI]',url)
    notes_path=OUT/'release-notes.md';notes_path.write_text(notes)
    if not args.publish:
        print('All release gates and actual APK checks passed; verify-only mode performed no remote writes.');return
    assets=[asset,checksum,proof]
    existing=api('releases/tags/'+lock['release']['tag'],allow_missing=True)
    if existing is not None:
        verify_release(lock,existing,notes,assets)
    tag_binding(lock,source)
    if existing is None:
        command(['gh','release','create',lock['release']['tag'],*[str(x) for x in assets],'--repo',REPOSITORY,'--verify-tag','--target',TEST,'--draft','--prerelease','--latest=false','--title',lock['release']['title'],'--notes-file',str(notes_path)])
        existing=api('releases/tags/'+lock['release']['tag'])
    verify_release(lock,existing,notes,assets)
    if existing['draft']:
        existing=api('releases/'+str(existing['id']),data={'draft':False,'prerelease':True,'make_latest':'false'})
    verify_release(lock,existing,notes,assets);require(existing['draft'] is False,'Release is still a draft')
    apk_asset=next(x for x in existing['assets'] if x['name']==asset.name)
    require(apk_asset['browser_download_url']==url,'Published URL differs')
    receipt={**validation,'publication_run_id':int(os.environ['GITHUB_RUN_ID']),'release_id':existing['id'],'asset_id':apk_asset['id'],'url':url,'outcome':'published-and-metadata-verified','no_forced_tag_or_replaced_asset':True}
    (OUT/'publication-receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print('Published compatible Logs8 test update:',url)


if __name__ == '__main__':
    try:
        main()
    except Exception as error:
        OUT.mkdir(exist_ok=True)
        (OUT/'publication-failure.json').write_text(json.dumps({'outcome':'failed','reason':str(error),'no_forced_tag_or_replaced_asset':True},indent=2)+'\n')
        raise
