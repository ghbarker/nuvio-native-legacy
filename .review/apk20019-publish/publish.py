#!/usr/bin/env python3
"""Publish one verified Logs9 test prerelease; never build, replace or force refs."""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path, PurePosixPath
import re
import stat
import subprocess
import sys
import tempfile
from urllib.parse import quote
import zipfile

REPOSITORY = 'ghbarker/nuvio-native-legacy'
REVIEW = 'a91e4a3d91481222222f69cec1a38164015c6e91'
BUILD_RUN = 38687834181
PLAN_SHA = 'a5198e61b5f3d9c998de95000e4c2b9b43a9bbfeceac90a6d66d72a6d947595e'
PRODUCT = '84e07919b57efd36acbbd71ec9ce53f20ea67c17'
TEST = 'efff065b63bb1234718ca20999265612bacb50b4'
CERTIFICATE = '862d7ef63a0dbde2929f6f231cae295510726d9ac28f9292496d2a38304be5e6'
JOBS = ('native', 'original-fontepref', 'original-fontepref_file', 'original-player_retido', 'android', 'jvm', 'all-gates')
HERE = Path(__file__).resolve().parent
RECIPE = Path('.review/apk20019')
OUT = Path('publication')


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def digest(value, length=64):
    return isinstance(value, str) and re.fullmatch('[0-9a-f]{' + str(length) + '}', value) is not None


def sha(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            h.update(block)
    return h.hexdigest()


def unique_object(pairs):
    result = {}
    for key, value in pairs:
        require(key not in result, 'Duplicate JSON key: ' + key)
        result[key] = value
    return result


def decode(data):
    return json.loads(data, object_pairs_hook=unique_object)


def validate_lock(lock, *, draft=False):
    require(lock['format'] == 1 and lock['repository'] == REPOSITORY, 'Wrong release-lock repository')
    require(lock['publisher_branch'] == 'review/apk20019-publish-20261015' and lock['source_plan_sha256'] == PLAN_SHA, 'Wrong publisher branch/source plan')
    old = lock['retained_validation']
    require(old == {'run_id': BUILD_RUN, 'review_commit': REVIEW, 'overall_run_success_required': False, 'conclusion': 'failure'}, 'Original failed run provenance changed')
    require(lock['excluded_failed_android'] == {'job_id': 116266667061, 'artifact_id': 11950529600, 'run_id': BUILD_RUN, 'outcome': 'failure', 'reuse_as_pass': False}, 'Failed Android origin must remain excluded')
    require(lock['validation']['workflow_path'] == '.github/workflows/apk20019-android-retry.yml', 'Wrong final retry workflow')
    require(lock['sources']['product']['commit'] == PRODUCT and lock['sources']['product']['tree'] == '03980d7707a30d112cf9b22f090cf4fab9a6e4dd', 'Wrong product source')
    require(lock['sources']['test']['commit'] == TEST and lock['sources']['test']['tree'] == 'db298030e77e126831f34139866067acbbb3aff3' and lock['sources']['test']['parent'] == PRODUCT, 'Wrong test source')
    require(lock['identity'] == {'package': 'space.nuvio.nativelegacy.touch', 'label': 'Nuvio TV/Mobile Test', 'version_code': 20019, 'version_name': '2.0.19-test-logs9', 'certificate_sha256': CERTIFICATE}, 'Wrong compatible update identity')
    release = lock['release']
    require(release['tag'] == 'tv-mobile-test-logs-v9' and release['asset'] == 'Nuvio-TV-Mobile-Test-Logs9.apk' and release['proof_asset'] == 'Logs9-validation.json' and release['prerelease'] is True, 'Only the Logs9 test prerelease is authorized')
    require(all(digest(lock[k]) for k in ('source_materializer_sha256', 'aggregate_recipe_sha256', 'retry_lock_sha256', 'retry_aggregate_sha256', 'bundle_sha256')) and digest(release['notes_sha256']), 'Missing recipe/notes bindings')
    require(set(lock['jobs']) == set(JOBS) and set(lock['artifacts']) == {'all_gates', 'android'}, 'Incomplete validation graph')
    for key in set(JOBS) - {'android', 'all-gates'}:
        item = lock['jobs'][key]
        require(item['run_id'] == BUILD_RUN and item['review_commit'] == REVIEW and type(item['id']) is int and item['id'] > 0, 'Retained successful origin changed')
        require(item['id'] != 116266667061 and item['artifact']['id'] != 11950529600, 'Failed Android cannot be retained as a pass')
    if draft:
        require(lock['root_reviewed'] is False and lock['all_gates_green'] is False, 'Draft lock must remain unapproved')
        require(lock['parent_review'] == lock['validation']['review_commit'] and digest(lock['parent_tree'], 40) and lock['validation']['run_attempt'] is None, 'Draft cannot claim a completed validation attempt')
        require(lock['all_gates_json_sha256'] is None and lock['apk'] == {'sha256': None, 'bytes': None}, 'Draft cannot claim actual final proof/APK bytes')
        require(all(lock['jobs'][k]['id'] is None for k in ('android', 'all-gates')), 'Fresh job IDs must await actual passes')
        require(all(x['id'] is None and x['sha256'] is None and x['bytes'] is None and x['name'] is None for x in lock['artifacts'].values()), 'Fresh artifact pins must await actual evidence')
        return
    final = lock['validation']
    require(lock['root_reviewed'] is True and lock['all_gates_green'] is True, 'Final root-reviewed all-green lock is required')
    require(type(final['run_id']) is int and final['run_id'] > 0 and final['run_id'] != BUILD_RUN and digest(final['review_commit'], 40) and final['review_commit'] != REVIEW, 'Fresh final validation origin is not bound')
    require(type(final['run_attempt']) is int and final['run_attempt'] > 0 and lock['parent_review'] == final['review_commit'] and digest(lock['parent_tree'], 40), 'Wrong final publisher parent/attempt')
    ids = [x['id'] for x in lock['jobs'].values()]
    require(all(type(x) is int and x > 0 for x in ids) and len(set(ids)) == len(ids), 'Seven distinct successful job IDs are required')
    for key in ('android', 'all-gates'):
        item = lock['jobs'][key]
        require(item['run_id'] == final['run_id'] and item['review_commit'] == final['review_commit'], 'Fresh job origin differs from final retry')
    for key, item in lock['artifacts'].items():
        require(type(item['id']) is int and item['id'] > 0 and type(item['bytes']) is int and item['bytes'] > 0 and digest(item['sha256']), 'Actual artifact bytes are not pinned')
        require(item['name'] == ('apk20019-retry-all-gates-' if key == 'all_gates' else 'apk20019-android-') + final['review_commit'], 'Wrong final artifact name')
        require(item['id'] != 11950529600, 'Failed Android artifact cannot be used')
    require(digest(lock['all_gates_json_sha256']) and digest(lock['apk']['sha256']) and type(lock['apk']['bytes']) is int and lock['apk']['bytes'] > 0, 'Actual aggregate/APK pins are missing')


def command(argv, *, cwd=None, env=None):
    result = subprocess.run(argv, cwd=cwd, env=env, text=True, capture_output=True)
    require(result.returncode == 0, 'Command failed: ' + argv[0])
    return result.stdout


def api(path, *, method='GET', data=None, file=None, binary=False):
    endpoint = path if path.startswith('https://uploads.github.com/') else 'repos/' + REPOSITORY + '/' + path
    args = ['gh', 'api', endpoint, '--method', method]
    if binary:
        args += ['-H', 'Accept: application/octet-stream']
    if data is not None:
        args += ['--input', '-']
    if file is not None:
        require(data is None and method == 'POST', 'Invalid upload operation')
        args += ['--input', str(file), '-H', 'Content-Type: application/octet-stream']
    result = subprocess.run(args, input=json.dumps(data).encode() if data is not None else None, capture_output=True)
    require(result.returncode == 0, 'GitHub API request failed: ' + path.split('?')[0])
    return result.stdout if binary else decode(result.stdout)


def verify_retry_recipe(lock):
    retry = Path('.review/apk20019-android-retry')
    require(sha(retry / 'retry-lock.json') == lock['retry_lock_sha256'] and sha(retry / 'aggregate_retry.py') == lock['retry_aggregate_sha256'], 'Reviewed retry recipe changed')
    value = decode((retry / 'retry-lock.json').read_text())
    require(value['frozen'] is True and value['root_reviewed'] is True and value['no_product_source_or_plan_changes'] is True and value['no_publication_or_reference_actions'] is True, 'Retry recipe was not approved')
    require(value['plan_sha256'] == PLAN_SHA and value['original_aggregate_sha256'] == lock['aggregate_recipe_sha256'] and value['excluded_failed_android'] == lock['excluded_failed_android'], 'Retry source/exclusion graph differs')
    require(value['sources'] == {k: {f: e[f] for f in ('commit', 'tree')} for k, e in lock['sources'].items()}, 'Retry source graph differs')
    for relative, expected in value['retry_script_sha256'].items():
        require(sha(Path('.review') / relative) == expected, 'Retry producer changed: ' + relative)
    for relative, expected in value['original_script_sha256'].items():
        require(sha(RECIPE / relative) == expected, 'Original producer changed: ' + relative)
    require(len(value['retained']) == 5 and len({x['scope'] for x in value['retained']}) == 5, 'Retry must retain exactly five successes')
    for entry in value['retained']:
        item = lock['jobs'][entry['scope']]
        require(item == {'id': entry['job_id'], 'name': entry['job_name'], 'run_id': BUILD_RUN, 'review_commit': REVIEW, 'artifact': entry['artifact']}, 'Retained root-lock origin differs from actual reviewed retry')


def artifact_binding(item, run_id, review):
    value = api('actions/artifacts/' + str(item['id']))
    require(value['id'] == item['id'] and value['name'] == item['name'] and value['expired'] is False, 'Artifact identity/expiry differs')
    require(value['workflow_run']['id'] == run_id and value['workflow_run']['head_sha'] == review, 'Artifact belongs to another producer')
    require(value['size_in_bytes'] == item['bytes'] and value.get('digest') == 'sha256:' + item['sha256'], 'Artifact metadata digest differs')
    return value


def verify_run(lock):
    old = api('actions/runs/' + str(BUILD_RUN))
    require(old['id'] == BUILD_RUN and old['head_sha'] == REVIEW and old['path'] == '.github/workflows/apk20019-review.yml' and old['status'] == 'completed' and old['conclusion'] == 'failure', 'Original failed run provenance changed')
    final = lock['validation']
    value = api('actions/runs/' + str(final['run_id']))
    require(value['id'] == final['run_id'] and value['head_sha'] == final['review_commit'] and value['path'] == final['workflow_path'] and value['head_branch'] == 'review/apk20019-android-retry-20261015', 'Final validation origin changed')
    require(value['repository']['full_name'] == REPOSITORY and value['head_repository']['full_name'] == REPOSITORY and old['repository']['full_name'] == REPOSITORY, 'Validation came from another repository')
    require(value['status'] == 'completed' and value['conclusion'] == 'success' and value['run_attempt'] == final['run_attempt'], 'Exact final retry attempt is not green')
    excluded = api('actions/jobs/' + str(lock['excluded_failed_android']['job_id']))
    require(excluded['run_id'] == BUILD_RUN and excluded['head_sha'] == REVIEW and excluded['status'] == 'completed' and excluded['conclusion'] == 'failure', 'Excluded failed Android status changed')
    safe = {}
    for key, expected in lock['jobs'].items():
        job = api('actions/jobs/' + str(expected['id']))
        require(job['id'] == expected['id'] and job['run_id'] == expected['run_id'] and job['head_sha'] == expected['review_commit'] and job['name'] == expected['name'], 'Producer job origin differs')
        require(job['status'] == 'completed' and job['conclusion'] == 'success', 'Required producer failed or is incomplete: ' + key)
        if key not in ('android', 'all-gates'):
            artifact_binding(expected['artifact'], BUILD_RUN, REVIEW)
        safe[key] = {'id': job['id'], 'name': job['name'], 'run_id': job['run_id'], 'review_commit': job['head_sha'], 'conclusion': 'success'}
    return safe


def download(item, run_id, review, name):
    artifact_binding(item, run_id, review)
    path = OUT / 'downloads' / name
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open('wb') as stream:
        result = subprocess.run(['gh', 'api', 'repos/' + REPOSITORY + '/actions/artifacts/' + str(item['id']) + '/zip'], stdout=stream, stderr=subprocess.PIPE)
    require(result.returncode == 0 and path.stat().st_size == item['bytes'] and sha(path) == item['sha256'], 'Actual artifact bytes differ')
    return path


def archive(path):
    z = zipfile.ZipFile(path)
    require(len(z.namelist()) == len(set(z.namelist())), 'Duplicate ZIP entries')
    require(sum(x.file_size for x in z.infolist()) < 2 * 1024**3, 'Unbounded ZIP expansion')
    for item in z.infolist():
        p = PurePosixPath(item.filename)
        require(not p.is_absolute() and '..' not in p.parts and '\\' not in item.filename and not stat.S_ISLNK(item.external_attr >> 16), 'Unsafe ZIP entry')
    return z


def read_json(z, name):
    require(z.getinfo(name).file_size <= 8 * 1024**2, 'Unbounded proof JSON')
    body = z.read(name)
    return decode(body), hashlib.sha256(body).hexdigest()


def unique_name(z, basename):
    names = [x for x in z.namelist() if PurePosixPath(x).name == basename]
    require(len(names) == 1, 'Expected one ' + basename)
    return names[0]


def verify_summary(lock, summary, summary_sha):
    final = lock['validation']
    require(summary_sha == lock['all_gates_json_sha256'] and summary['format'] == 1 and summary['outcome'] == 'passed', 'Final aggregate proof differs or is incomplete')
    require(summary['review_commit'] == final['review_commit'] and summary['github_run_id'] == final['run_id'], 'Final aggregate origin differs')
    require(summary['plan_sha256'] == PLAN_SHA and summary['source_bindings'] == lock['sources'] and summary['bundle_sha256'] == lock['bundle_sha256'], 'Final aggregate source graph differs')
    require(summary['retry_lock_sha256'] == lock['retry_lock_sha256'] and summary['original_validator_sha256'] == lock['aggregate_recipe_sha256'], 'Final aggregate recipe differs')
    require(summary['retained_failed_run_conclusion'] == 'failure' and summary['retained_records_rewritten'] is False and summary['excluded_failed_android'] == lock['excluded_failed_android'], 'Retained/excluded origin integrity differs')
    require(summary['validator_adapter_changes'] == [
        ['def __init__(self, root, plan, review, stage=HERE):', 'def __init__(self, root, plan, review, stage=HERE, included=JOBS):'],
        ['        for job, name in ARTIFACTS.items():\n', '        for job, name in ARTIFACTS.items():\n            if job not in included:\n                continue\n']], 'Aggregate adapter changed original assertions')
    origins = summary['producer_origins']
    require(len(origins) == 6 and {x['scope'] for x in origins} == set(JOBS) - {'all-gates'}, 'Six actual successful producer origins are required')
    for entry in origins:
        key = entry['scope']
        item = lock['jobs'][key]
        artifact = lock['artifacts']['android'] if key == 'android' else item['artifact']
        require(entry == {'scope': key, 'run_id': item['run_id'], 'review_commit': item['review_commit'], 'job_id': item['id'], 'job_conclusion': 'success', 'artifact': artifact}, 'Actual producer origin differs from root lock')
    require(summary['native']['complete_apps'] == 4 and summary['native']['actual_controls_layers'] == 2 and summary['native']['cached_status_groups'] == 32 and summary['native']['synopsis_groups'] == 28, 'Native contract groups incomplete')
    require(summary['original_full_source'] == {'full_source_runs': 6, 'normal': 3, 'asan_ubsan': 3}, 'Original normal/sanitizer checks incomplete')
    require(summary['jvm'] == {'fresh_helpers': 32, 'fresh_media3': 15, 'fresh_total': 47, 'historical_execution_reused': False}, 'JVM/Media3 checks incomplete')
    require(summary['publish_release_refs_or_backend_actions'] is False, 'Validation performed unexpected actions')
    require(set(summary['artifact_inputs_sha256']) == {'retained', 'fresh'} and all(v and all(digest(x) for x in v.values()) for v in summary['artifact_inputs_sha256'].values()), 'Raw per-origin proof bindings are missing')


def artifact_hash(summary, name, data):
    hashes = summary['artifact_inputs_sha256']['fresh']
    matches = [x for x in hashes if x.startswith('apk20019-android-' + summary['review_commit'] + '/') and x.endswith('/' + name)]
    require(len(matches) == 1 and hashlib.sha256(data).hexdigest() == hashes[matches[0]], 'Fresh Android proof bytes differ from aggregate: ' + name)


def verify_android(lock, summary, z):
    records = {}
    for name in z.namelist():
        if PurePosixPath(name).name != 'android-verified.json':
            continue
        value, digest_value = read_json(z, name)
        layer = value['layer']
        require(layer in ('product', 'test') and layer not in records, 'Unknown/duplicate Android layer')
        folder = PurePosixPath(name).parent
        artifact_hash(summary, str(folder / 'android-verified.json'), z.read(name))
        require(value['outcome'] == 'passed' and value['source_commit'] == lock['sources'][layer]['commit'] and value['tree'] == lock['sources'][layer]['tree'] and value['review_commit'] == lock['validation']['review_commit'] and value['plan_sha256'] == PLAN_SHA, 'Android source/origin differs')
        require(value['certificate_sha256'] == CERTIFICATE and value['native_16kb_pages'] == 'passed' and value['legacy_jni_overloads_preserved'] is True, 'Android signer/native/JNI proof incomplete')
        require(value['prior_apk_proofs_reused'] is False and value['release_or_backend_actions'] is False and value['private_log_opt_in'] is (layer == 'test'), 'Android isolation/freshness differs')
        require({x['abi'] for x in value['actual_arm_compiles']} == {'arm64-v8a', 'armeabi-v7a'} and len(value['actual_arm_compiles']) == 2, 'Both actual ARM builds are required')
        require(all(x['builds'] and all(b['main_sources'] > 0 and b['report_endpoint_configured'] is True and b['private_native_opt_in'] is (layer == 'test') for b in x['builds']) for x in value['actual_arm_compiles']), 'Actual native compile scope differs')
        expected_package = lock['identity']['package'] if layer == 'test' else 'space.nuvio.nativelegacy'
        require(value['package'] == expected_package and value['file_provider'] == {'authority': expected_package + '.atualizacao', 'nonexported': True, 'updater_path_only': True}, 'APK package/provider isolation differs')
        if layer == 'test':
            require(all(value[k] == lock['identity'][k] for k in ('version_code', 'version_name')), 'Compatible update version differs')
        require(summary['android'][layer] == {k: value[k] for k in ('apk_name', 'apk_sha256', 'apk_bytes', 'package', 'version_name', 'version_code', 'certificate_sha256')}, 'Aggregate APK identity differs')
        require(PurePosixPath(value['apk_name']).name == value['apk_name'], 'Unsafe APK name')
        build_name = str(folder / 'build-receipt.json')
        build, build_sha = read_json(z, build_name)
        artifact_hash(summary, build_name, z.read(build_name))
        require(build['verified'] is True and build['source_restored_clean'] is True and build['fresh_build_of_both_arms'] is True and build['prior_apk_proofs_reused'] is False and build['no_dispatch_release_upload_or_backend_actions'] is True, 'Actual APK build is not complete/fresh/clean')
        require(build['verification_sha256'] == digest_value and all(build[k] == value[k] for k in ('layer', 'source_commit', 'tree', 'review_commit', 'plan_sha256', 'source_proof_sha256', 'apk_name', 'apk_sha256')), 'Android build receipt differs')
        apk_name = str(folder / value['apk_name'])
        with z.open(apk_name) as stream:
            target = (OUT / lock['release']['asset']).open('wb') if layer == 'test' else None
            h = hashlib.sha256()
            size = 0
            try:
                for block in iter(lambda: stream.read(1024 * 1024), b''):
                    h.update(block)
                    size += len(block)
                    if target:
                        target.write(block)
            finally:
                if target:
                    target.close()
        require(h.hexdigest() == value['apk_sha256'] and size == value['apk_bytes'], 'Actual APK bytes differ from proof')
        if layer == 'test':
            require(h.hexdigest() == lock['apk']['sha256'] and size == lock['apk']['bytes'], 'Compatible APK differs from root-reviewed bytes')
        records[layer] = {'verification_sha256': digest_value, 'build_receipt_sha256': build_sha, 'apk_sha256': h.hexdigest(), 'apk_bytes': size}
    require(set(records) == {'product', 'test'}, 'Both fresh Android layers must be present')
    return records


def verify_actual_apk(lock, source):
    sdk = Path(os.environ.get('ANDROID_HOME') or os.environ['ANDROID_SDK_ROOT']) / 'build-tools/35.0.0'
    apk = OUT / lock['release']['asset']
    ident = lock['identity']
    badging = command([str(sdk / 'aapt'), 'dump', 'badging', str(apk)])
    require(re.search(r"^package: name='" + re.escape(ident['package']) + r"'", badging, re.M) and "versionCode='20019'" in badging and "versionName='" + ident['version_name'] + "'" in badging, 'Actual APK identity differs')
    require(re.search(r"^application-label:'" + re.escape(ident['label']) + r"'", badging, re.M), 'Actual APK label differs')
    signing = command([str(sdk / 'apksigner'), 'verify', '--verbose', '--print-certs', str(apk)])
    require(re.findall(r'^Signer #\d+ certificate SHA-256 digest: ([0-9a-f]+)$', signing, re.M) == [CERTIFICATE], 'Actual APK signer differs')
    command([sys.executable, str(source / 'tools/android-apk-pages.py'), str(apk.resolve())])
    xml = command([str(sdk / 'aapt'), 'dump', 'xmltree', str(apk), 'AndroidManifest.xml'])
    lines = xml.splitlines()
    providers = []
    for index, line in enumerate(lines):
        match = re.match(r'^(\s*)E: provider\b', line)
        if not match:
            continue
        indent = len(match.group(1))
        end = next((i for i in range(index + 1, len(lines)) if re.match(r'^\s*E:', lines[i]) and len(lines[i]) - len(lines[i].lstrip()) <= indent), len(lines))
        providers.append('\n'.join(lines[index:end]))
    matches = [x for x in providers if re.search(r'android:authorities[^\n]*="' + re.escape(ident['package'] + '.atualizacao') + '"', x)]
    require(len(matches) == 1 and 'androidx.core.content.FileProvider' in matches[0] and 'android.support.FILE_PROVIDER_PATHS' in matches[0], 'Actual APK FileProvider differs')
    require(re.search(r'android:exported[^\n]*\(type\s+0x12\)0x0\b', matches[0]) and re.search(r'android:grantUriPermissions[^\n]*\(type\s+0x12\)0x(?:ffffffff|1)\b', matches[0]), 'Actual APK provider permission scope differs')
    paths = command([str(sdk / 'aapt'), 'dump', 'xmltree', str(apk), 'res/xml/caminhos_atualizacao.xml'])
    require(not re.search(r'E: (?:cache-path|root-path|external-path|external-cache-path)\b', paths) and 'name="atualizacao"' in paths and 'path="dados/atualizacao/"' in paths, 'Actual APK updater file paths differ')
    spec = importlib.util.spec_from_file_location('release_dex', RECIPE / 'android/dex_methods.py')
    parser = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(parser)
    actual, dexes = parser.apk_methods(apk, 'Lspace/nuvio/nativelegacy/NvPlayer;')
    bridge = (source / 'src/video_android.c').read_text()
    lookups = re.findall(r'GetStaticMethodID\(env,\s*gCls,\s*"([^"]+)",\s*"([^"]+)"\)', bridge)
    require(len(lookups) >= 19 and len(set(lookups)) == len(lookups), 'Current JNI lookup closure incomplete')
    legacy = decode((RECIPE / 'android/legacy-jni-descriptors.json').read_text())['methods']
    for signature in lookups + [(x['name'], x['descriptor']) for x in legacy]:
        require(actual.get(tuple(signature), 0) & 8, 'Actual APK JNI method missing/nonstatic')
    private, _ = parser.apk_methods(apk, 'Lspace/nuvio/nativelegacy/TestLocalLogs;')
    require(('snapshot', '()[B') in private, 'Actual APK private diagnostics class missing')
    for klass, methods in {'NuvioActivity': [('baixarRegistro', '(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V'), ('onActivityResult', '(IILandroid/content/Intent;)V')], 'RegistroDownload': [('abrir', '(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V'), ('resultado', '(IILandroid/content/Intent;)Z'), ('fechar', '()V')], 'RegistroDownload$Companion': [('documento', '()Landroid/content/Intent;'), ('lerFim', '(Ljava/io/File;)Ljava/lang/String;'), ('limpar', '(Ljava/lang/String;)Ljava/lang/String;')]}.items():
        declared, _ = parser.apk_methods(apk, 'Lspace/nuvio/nativelegacy/' + klass + ';')
        require(all(x in declared and not declared[x] & 8 for x in methods), 'Actual APK log download API missing')
    with archive(apk) as z:
        for abi in ('arm64-v8a', 'armeabi-v7a'):
            for library in ('libmain.so', 'libSDL2.so', 'libcurl.so'):
                require('lib/' + abi + '/' + library in z.namelist(), 'Actual APK missing ARM library')
    return {**ident, 'native_16kb_pages': 'passed', 'abis': ['arm64-v8a', 'armeabi-v7a'], 'dex_sha256': dexes, 'private_diagnostics': True, 'jni_methods_verified': len(lookups)}


def ensure_tag(lock, source):
    tag = lock['release']['tag']
    refs = api('git/matching-refs/tags/' + tag)
    matches = [x for x in refs if x['ref'] == 'refs/tags/' + tag]
    require(len(matches) <= 1, 'Duplicate tag references')
    if not matches:
        with tempfile.TemporaryDirectory(prefix='logs9-git-auth-') as directory:
            helper = Path(directory) / 'askpass'
            helper.write_text('#!/bin/sh\ncase "$1" in\n*Username*) printf "%s\\n" x-access-token ;;\n*Password*) printf "%s\\n" "$GH_TOKEN" ;;\n*) exit 1 ;;\nesac\n')
            helper.chmod(0o700)
            command(['git', '-c', 'credential.helper=', 'push', '--porcelain', 'https://github.com/' + REPOSITORY + '.git', 'HEAD:refs/tags/' + tag], cwd=source, env=dict(os.environ, GIT_ASKPASS=str(helper), GIT_TERMINAL_PROMPT='0'))
        matches = [api('git/ref/tags/' + tag)]
    require(matches[0]['object']['type'] == 'commit' and matches[0]['object']['sha'] == TEST, 'Existing tag differs; no force update is allowed')


def find_release(tag):
    matches = []
    for page in range(1, 51):
        batch = api('releases?per_page=100&page=' + str(page))
        matches.extend(x for x in batch if x['tag_name'] == tag)
        if len(batch) < 100:
            break
    else:
        require(False, 'Release collection was not completely enumerated')
    require(len(matches) <= 1, 'Multiple releases use the intended tag')
    return matches[0] if matches else None


def verify_release_header(lock, record, notes):
    require(type(record['id']) is int and record['id'] > 0, 'Missing numeric release ID')
    require(record['tag_name'] == lock['release']['tag'] and record['target_commitish'] == TEST and record['name'] == lock['release']['title'] and record['prerelease'] is True, 'Existing release identity differs')
    require(record['body'].replace('\r\n', '\n').rstrip() == notes.rstrip(), 'Existing release notes differ')
    expected = 'https://uploads.github.com/repos/' + REPOSITORY + '/releases/' + str(record['id']) + '/assets'
    require(record['upload_url'].split('{', 1)[0] == expected, 'Unexpected release upload endpoint')


def verify_asset(asset, path):
    require(asset['state'] == 'uploaded' and asset['size'] == path.stat().st_size and asset.get('digest') == 'sha256:' + sha(path), 'Existing asset metadata differs; no replacement is allowed')
    body = api('releases/assets/' + str(asset['id']), binary=True)
    require(len(body) == path.stat().st_size and hashlib.sha256(body).hexdigest() == sha(path), 'Actual uploaded asset bytes differ')


def publish_release(lock, source, notes, assets):
    record = find_release(lock['release']['tag'])
    expected = {path.name: path for path in assets}
    if record is not None:
        verify_release_header(lock, record, notes)
        require(len(record['assets']) == len({x['name'] for x in record['assets']}) and {x['name'] for x in record['assets']} <= set(expected), 'Unexpected existing release assets')
        require(record['draft'] or len(record['assets']) == len(expected), 'Published release assets are incomplete')
        for asset in record['assets']:
            verify_asset(asset, expected[asset['name']])
    ensure_tag(lock, source)
    if record is None:
        record = api('releases', method='POST', data={'tag_name': lock['release']['tag'], 'target_commitish': TEST, 'name': lock['release']['title'], 'body': notes, 'draft': True, 'prerelease': True, 'make_latest': 'false'})
        (OUT / 'draft-created.json').write_text(json.dumps({'release_id': record['id'], 'tag': record['tag_name'], 'source': TEST}, indent=2) + '\n')
    verify_release_header(lock, record, notes)
    release_id = record['id']
    # Drafts are always read/uploaded/published by numeric ID, never by tag.
    record = api('releases/' + str(release_id))
    verify_release_header(lock, record, notes)
    require(len(record['assets']) == len({x['name'] for x in record['assets']}), 'Duplicate release asset names')
    require({x['name'] for x in record['assets']} <= set(expected), 'Unexpected existing release assets')
    for path in assets:
        existing = next((x for x in record['assets'] if x['name'] == path.name), None)
        if existing:
            verify_asset(existing, path)
        else:
            require(record['draft'] is True, 'Refusing to add assets to a public release')
            endpoint = record['upload_url'].split('{', 1)[0] + '?name=' + quote(path.name, safe='')
            uploaded = api(endpoint, method='POST', file=path)
            require(uploaded['name'] == path.name, 'Uploaded asset name differs')
            verify_asset(uploaded, path)
    record = api('releases/' + str(release_id))
    verify_release_header(lock, record, notes)
    require(len(record['assets']) == len(expected) and {x['name'] for x in record['assets']} == set(expected), 'Final asset set differs')
    for asset in record['assets']:
        verify_asset(asset, expected[asset['name']])
    if record['draft']:
        record = api('releases/' + str(release_id), method='PATCH', data={'draft': False, 'prerelease': True, 'make_latest': 'false'})
    require(record['draft'] is False, 'Release is still a draft')
    verify_release_header(lock, record, notes)
    return record


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check-draft', action='store_true')
    parser.add_argument('--check-lock', action='store_true')
    parser.add_argument('--publish', action='store_true')
    args = parser.parse_args()
    require(not (args.check_draft and (args.check_lock or args.publish)), 'Choose one lock check mode')
    lock_path = HERE / 'release-lock.json'
    lock = decode(lock_path.read_text())
    validate_lock(lock, draft=args.check_draft)
    require(sha(HERE / 'release-notes.md') == lock['release']['notes_sha256'], 'Reviewed notes changed')
    if args.check_draft or args.check_lock:
        print('Release lock structure checked offline; no network, build or publication.')
        return
    require(sys.flags.optimize == 0, 'Python optimization is not permitted')
    require(os.environ.get('GITHUB_REPOSITORY') == REPOSITORY and os.environ.get('GH_TOKEN') and os.environ.get('GITHUB_REF') == 'refs/heads/' + lock['publisher_branch'], 'Wrong publisher repository/branch/token scope')
    require(os.environ.get('APK20019_RELEASE_LOCK_SHA256') == sha(lock_path) and os.environ.get('NUVIO_REVIEW_PLAN_SHA256') == PLAN_SHA and digest(os.environ.get('GITHUB_SHA'), 40), 'Wrong publication lock/plan identity')
    require(sha(RECIPE / 'plan.json') == PLAN_SHA and sha(RECIPE / 'materialize.py') == lock['source_materializer_sha256'] and sha(RECIPE / 'aggregate.py') == lock['aggregate_recipe_sha256'], 'Reviewed source recipe changed')
    plan = decode((RECIPE / 'plan.json').read_text())
    require(plan['root_reviewed'] is True and plan['frozen'] is True and plan['sources'] == lock['sources'] and plan['identity'] == lock['identity'], 'Plan source/identity differs')
    require(command(['git', 'rev-parse', lock['parent_review'] + '^{tree}']).strip() == lock['parent_tree'], 'Final retry parent tree differs')
    command(['git', 'diff', '--quiet', lock['parent_review'], 'HEAD', '--', '.review/apk20019', '.review/apk20019-android-retry', '.review/logs8/native', '.review/logs8/kotlin', '.review/logs8/runtime', '.github/workflows/apk20019-review.yml', '.github/workflows/apk20019-android-retry.yml'])
    OUT.mkdir(exist_ok=True)
    verify_retry_recipe(lock)
    jobs = verify_run(lock)
    final = lock['validation']
    gate_zip = download(lock['artifacts']['all_gates'], final['run_id'], final['review_commit'], 'all-gates.zip')
    with archive(gate_zip) as z:
        summary, summary_sha = read_json(z, unique_name(z, 'all-gates-verified.json'))
    verify_summary(lock, summary, summary_sha)
    android_zip = download(lock['artifacts']['android'], final['run_id'], final['review_commit'], 'android.zip')
    with archive(android_zip) as z:
        android = verify_android(lock, summary, z)
    source = Path('publication-source').resolve()
    require(not source.exists(), 'Refusing to replace source materialization')
    command([sys.executable, str(RECIPE / 'materialize.py'), '--layer', 'test', '--output', str(source), '--proof', str((OUT / 'materialized-test.json').resolve()), '--base-repository', str(Path.cwd())])
    require(command(['git', 'rev-parse', 'HEAD', 'HEAD^{tree}', 'HEAD^'], cwd=source).splitlines() == [TEST, lock['sources']['test']['tree'], PRODUCT], 'Actual source materialization differs')
    ident = verify_actual_apk(lock, source)
    asset = OUT / lock['release']['asset']
    checksum = OUT / (asset.name + '.sha256')
    checksum.write_text(sha(asset) + '  ' + asset.name + '\n')
    provenance = {'format': 1, 'outcome': 'passed', 'repository': REPOSITORY, 'source_commit': TEST, 'source_tree': lock['sources']['test']['tree'], 'product_commit': PRODUCT, 'validation_review_commit': final['review_commit'], 'validation_run_id': final['run_id'], 'validation_run_attempt': lock['validation']['run_attempt'], 'publication_review_commit': os.environ['GITHUB_SHA'], 'source_plan_sha256': PLAN_SHA, 'release_lock_sha256': sha(lock_path), 'all_gates_sha256': summary_sha, 'jobs': jobs, 'validation_artifacts': lock['artifacts'], 'producer_origins': summary['producer_origins'], 'retained_failed_run_conclusion': 'failure', 'excluded_failed_android': lock['excluded_failed_android'], 'retained_records_rewritten': False, 'apk_sha256': sha(asset), 'apk_bytes': asset.stat().st_size, 'identity': ident, 'android_receipts': android, 'private_configs_or_logs_uploaded': False}
    proof = OUT / lock['release']['proof_asset']
    proof.write_text(json.dumps(provenance, indent=2) + '\n')
    if not args.publish:
        print('All checks passed; verify-only mode performed no remote writes.')
        return
    record = publish_release(lock, source, (HERE / 'release-notes.md').read_text(), [asset, checksum, proof])
    published_apk = next(x for x in record['assets'] if x['name'] == asset.name)
    url = 'https://github.com/' + REPOSITORY + '/releases/download/' + lock['release']['tag'] + '/' + asset.name
    require(published_apk['browser_download_url'] == url, 'Published URL differs')
    receipt = {**provenance, 'outcome': 'published-and-uploaded-bytes-verified', 'publication_run_id': int(os.environ['GITHUB_RUN_ID']), 'release_id': record['id'], 'asset_id': published_apk['id'], 'url': url, 'no_forced_tag_or_replaced_asset': True}
    (OUT / 'publication-receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(url)


if __name__ == '__main__':
    try:
        main()
    except Exception as error:
        OUT.mkdir(exist_ok=True)
        (OUT / 'publication-failure.json').write_text(json.dumps({'outcome': 'failed', 'reason': str(error), 'no_forced_tag_or_replaced_asset': True}, indent=2) + '\n')
        raise
