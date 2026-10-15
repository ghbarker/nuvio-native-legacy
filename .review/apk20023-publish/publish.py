#!/usr/bin/env python3
"""Publish one verified Logs13 test prerelease; never build, replace or force refs."""
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
CERTIFICATE = '862d7ef63a0dbde2929f6f231cae295510726d9ac28f9292496d2a38304be5e6'
JOBS = ('native', 'original-fontepref', 'original-fontepref_file', 'original-player_retido', 'android', 'jvm', 'feature-contracts', 'all-gates')
HERE = Path(__file__).resolve().parent
RECIPE = Path('.review/apk20023')
OUT = Path('publication')
BIG = Path(os.environ.get('RUNNER_TEMP') or tempfile.gettempdir()) / 'nuvio-logs13-publication'


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
    require(lock['publisher_branch'] == 'review/apk20023-publish-20261015', 'Wrong publisher branch')
    require(lock['validation']['workflow_path'] == '.github/workflows/apk20023-review.yml', 'Wrong fresh validation workflow')
    require(set(lock['sources']) == {'product', 'test'}, 'Both source layers must be pinned')
    for layer, entry in lock['sources'].items():
        require(entry['manifest'] == layer + '-source-manifest.json', 'Unexpected source manifest path')
    require(lock['identity'] == {'package': 'space.nuvio.nativelegacy.touch', 'label': 'Nuvio TV/Mobile Test', 'version_code': 20023, 'version_name': '2.0.23-test-logs13', 'certificate_sha256': CERTIFICATE}, 'Wrong compatible update identity')
    release = lock['release']
    require(release['tag'] == 'tv-mobile-test-logs-v13' and release['asset'] == 'Nuvio-TV-Mobile-Test-Logs13.apk' and release['proof_asset'] == 'Logs13-validation.json' and release['prerelease'] is True, 'Only the Logs13 test prerelease is authorized')
    require(digest(release['notes_sha256']), 'Reviewed release notes are not pinned')
    require(set(lock['jobs']) == set(JOBS) and set(lock['artifacts']) == {'all_gates', 'android'}, 'Incomplete seven-producer fresh validation graph')
    require(not any(k in lock for k in ('retained_validation', 'excluded_failed_android', 'retry_lock_sha256', 'producer_origins')), 'Single-run lock must not inherit retained/retry proof origins')
    if draft:
        require(lock['root_reviewed'] is False and lock['all_gates_green'] is False, 'Draft lock must remain unapproved')
        require(all(lock[k] is None for k in ('parent_review', 'parent_tree', 'source_plan_sha256', 'source_materializer_sha256', 'aggregate_recipe_sha256', 'bundle_sha256', 'all_gates_json_sha256')), 'Draft cannot invent validated source/recipe/proof pins')
        require(all(lock['validation'][k] is None for k in ('run_id', 'review_commit', 'run_attempt')), 'Draft cannot claim a validation run')
        require(all(entry[k] is None for entry in lock['sources'].values() for k in ('commit', 'tree', 'parent', 'manifest_sha256', 'delta_patch_sha256')), 'Draft source pins must await final accepted composition')
        require(lock['apk'] == {'sha256': None, 'bytes': None}, 'Draft cannot claim actual APK bytes')
        require(all(x['id'] is None for x in lock['jobs'].values()), 'Draft job IDs must await actual passes')
        require(all(all(x[k] is None for k in ('id', 'sha256', 'bytes', 'name')) for x in lock['artifacts'].values()), 'Draft artifact pins must await actual evidence')
        return
    final = lock['validation']
    require(lock['root_reviewed'] is True and lock['all_gates_green'] is True, 'Final root-reviewed all-green lock required')
    require(all(digest(lock[k]) for k in ('source_plan_sha256', 'source_materializer_sha256', 'aggregate_recipe_sha256', 'bundle_sha256', 'all_gates_json_sha256')), 'Final source/recipe/proof hashes missing')
    for entry in lock['sources'].values():
        require(all(digest(entry[k], 40) for k in ('commit', 'tree', 'parent')) and all(digest(entry[k]) for k in ('manifest_sha256', 'delta_patch_sha256')), 'Actual final source bindings are missing')
    require(lock['sources']['test']['parent'] == lock['sources']['product']['commit'], 'Private source must be the approved product overlay')
    require(type(final['run_id']) is int and final['run_id'] > 0 and digest(final['review_commit'], 40) and type(final['run_attempt']) is int and final['run_attempt'] > 0, 'Actual validation run/attempt missing')
    require(lock['parent_review'] == final['review_commit'] and digest(lock['parent_tree'], 40), 'Publisher must inherit exact successful source review')
    ids = [x['id'] for x in lock['jobs'].values()]
    require(all(type(x) is int and x > 0 for x in ids) and len(set(ids)) == len(JOBS), 'Seven producers and aggregate need eight distinct successful jobs')
    for key, item in lock['artifacts'].items():
        require(type(item['id']) is int and item['id'] > 0 and type(item['bytes']) is int and item['bytes'] > 0 and digest(item['sha256']), 'Actual artifact bytes are not pinned')
        require(item['name'] == ('apk20023-all-gates-' if key == 'all_gates' else 'apk20023-android-') + final['review_commit'], 'Wrong actual artifact name')
    require(digest(lock['apk']['sha256']) and type(lock['apk']['bytes']) is int and lock['apk']['bytes'] > 0, 'Actual APK byte pins missing')


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





def artifact_binding(item, run_id, review):
    value = api('actions/artifacts/' + str(item['id']))
    require(value['id'] == item['id'] and value['name'] == item['name'] and value['expired'] is False, 'Artifact identity/expiry differs')
    require(value['workflow_run']['id'] == run_id and value['workflow_run']['head_sha'] == review, 'Artifact belongs to another producer')
    require(value['size_in_bytes'] == item['bytes'] and value.get('digest') == 'sha256:' + item['sha256'], 'Artifact metadata digest differs')
    return value


def verify_run(lock):
    final = lock['validation']
    value = api('actions/runs/' + str(final['run_id']))
    require(value['id'] == final['run_id'] and value['head_sha'] == final['review_commit'] and value['path'] == final['workflow_path'] and value['head_branch'] == 'review/apk20023-20261015', 'Fresh validation origin changed')
    require(value['repository']['full_name'] == REPOSITORY and value['head_repository']['full_name'] == REPOSITORY, 'Validation came from another repository')
    require(value['status'] == 'completed' and value['conclusion'] == 'success' and value['run_attempt'] == final['run_attempt'], 'Exact complete validation attempt is not green')
    attempt = api('actions/runs/' + str(final['run_id']) + '/attempts/' + str(final['run_attempt']) + '/jobs?per_page=100')
    require(attempt['total_count'] == len(JOBS) and len(attempt['jobs']) == len(JOBS), 'Unexpected validation attempt job graph')
    require({x['id'] for x in attempt['jobs']} == {x['id'] for x in lock['jobs'].values()}, 'Pinned jobs are not the exact successful attempt')
    safe = {}
    for key, expected in lock['jobs'].items():
        job = api('actions/jobs/' + str(expected['id']))
        require(job['id'] == expected['id'] and job['run_id'] == final['run_id'] and job['head_sha'] == final['review_commit'] and job['name'] == expected['name'], 'Producer job belongs to another run/review')
        require(job['status'] == 'completed' and job['conclusion'] == 'success', 'Required producer failed or is incomplete: ' + key)
        safe[key] = {'id': job['id'], 'name': job['name'], 'run_id': job['run_id'], 'review_commit': job['head_sha'], 'conclusion': 'success'}
    return safe


def download(item, run_id, review, name):
    artifact_binding(item, run_id, review)
    path = BIG / 'downloads' / name
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
    require(summary_sha == lock['all_gates_json_sha256'] and summary['format'] == 1 and summary['outcome'] == 'passed', 'Final aggregate proof differs or incomplete')
    require(summary['review_commit'] == final['review_commit'] and summary['github_run_id'] == str(final['run_id']) and summary['github_run_attempt'] == str(final['run_attempt']), 'Final aggregate run/review/attempt differs')
    require(summary['plan_sha256'] == lock['source_plan_sha256'] and summary['source_bindings'] == lock['sources'] and summary['bundle_sha256'] == lock['bundle_sha256'], 'Final aggregate source graph differs')
    require(set(summary['job_results']) == set(JOBS) - {'all-gates'} and all(x['result'] == 'success' for x in summary['job_results'].values()), 'Every fresh producer must pass')
    require(summary['native']['complete_apps'] == 4 and summary['native']['actual_controls_layers'] == 2 and summary['native']['cached_status_groups'] == 32 and summary['native']['synopsis_groups'] == 36, 'Native contract groups incomplete')
    features = summary['features']
    expected = {'fresh_groups': 36, 'caption_style_libass': True, 'collection_settings_profiles_views_touch': True, 'collection_pointer_groups': 4, 'collection_pointer_gesture_cases_per_group': 75, 'collection_pointer_reachability_cases_per_group': 36, 'collection_pointer_async_cases_per_group': 15, 'collection_pointer_reachability_title_activations_per_group': 2160, 'settings_subtitle_style_groups': 4, 'settings_subtitle_style_menu_cases_per_group': 6, 'settings_subtitle_style_editor_cases_per_group': 6, 'subtitle_parity_renderer_groups': 4, 'artwork_registry_groups': 4}
    require(all(features.get(k) == v for k, v in expected.items()) and bool(features['actual_libass_version']), 'Required collection/Style/outline groups incomplete')
    require(summary['original_full_source'] == {'full_source_runs': 6, 'normal': 3, 'asan_ubsan': 3}, 'Original normal/sanitizer checks incomplete')
    require(summary['jvm'] == {'fresh_helpers': 32, 'fresh_media3': 15, 'fresh_total': 47, 'historical_execution_reused': False}, 'Fresh JVM/Media3 checks incomplete')
    require(summary['publish_release_refs_or_backend_actions'] is False, 'Validation performed unexpected actions')
    require(summary['artifact_inputs_sha256'] and all(digest(v) for v in summary['artifact_inputs_sha256'].values()), 'Flat raw proof bindings missing')
    require('producer_origins' not in summary and 'retained_failed_run_conclusion' not in summary, 'Single-run validation must not inherit retained/retry proofs')
    require(summary['recipe_inputs_sha256']['aggregate.py'] == lock['aggregate_recipe_sha256'] and summary['recipe_inputs_sha256']['plan.json'] == lock['source_plan_sha256'], 'Actual aggregate recipe differs')


def artifact_hash(summary, name, data):
    hashes = summary['artifact_inputs_sha256']
    matches = [x for x in hashes if x.startswith('apk20023-android-' + summary['review_commit'] + '/') and x.endswith('/' + name)]
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
        require(value['outcome'] == 'passed' and value['source_commit'] == lock['sources'][layer]['commit'] and value['tree'] == lock['sources'][layer]['tree'] and value['review_commit'] == lock['validation']['review_commit'] and value['plan_sha256'] == lock['source_plan_sha256'], 'Android source/origin differs')
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
            target = (BIG / lock['release']['asset']).open('wb') if layer == 'test' else None
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
    apk = BIG / lock['release']['asset']
    ident = lock['identity']
    badging = command([str(sdk / 'aapt'), 'dump', 'badging', str(apk)])
    require(re.search(r"^package: name='" + re.escape(ident['package']) + r"'", badging, re.M) and "versionCode='20023'" in badging and "versionName='" + ident['version_name'] + "'" in badging, 'Actual APK identity differs')
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
        with tempfile.TemporaryDirectory(prefix='logs13-git-auth-') as directory:
            helper = Path(directory) / 'askpass'
            helper.write_text('#!/bin/sh\ncase "$1" in\n*Username*) printf "%s\\n" x-access-token ;;\n*Password*) printf "%s\\n" "$GH_TOKEN" ;;\n*) exit 1 ;;\nesac\n')
            helper.chmod(0o700)
            command(['git', '-c', 'credential.helper=', 'push', '--porcelain', 'https://github.com/' + REPOSITORY + '.git', 'HEAD:refs/tags/' + tag], cwd=source, env=dict(os.environ, GIT_ASKPASS=str(helper), GIT_TERMINAL_PROMPT='0'))
        matches = [api('git/ref/tags/' + tag)]
    require(matches[0]['object']['type'] == 'commit' and matches[0]['object']['sha'] == lock['sources']['test']['commit'], 'Existing tag differs; no force update is allowed')


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
    require(record['tag_name'] == lock['release']['tag'] and record['target_commitish'] == lock['sources']['test']['commit'] and record['name'] == lock['release']['title'] and record['prerelease'] is True, 'Existing release identity differs')
    require(record['body'].replace('\r\n', '\n').rstrip() == notes.rstrip(), 'Existing release notes differ')
    expected = 'https://uploads.github.com/repos/' + REPOSITORY + '/releases/' + str(record['id']) + '/assets'
    require(record['upload_url'].split('{', 1)[0] == expected, 'Unexpected release upload endpoint')


def verify_asset(asset, path):
    require(asset['state'] == 'uploaded' and asset['size'] == path.stat().st_size and asset.get('digest') == 'sha256:' + sha(path), 'Existing asset metadata differs; no replacement is allowed')
    body = api('releases/assets/' + str(asset['id']), binary=True)
    require(len(body) == path.stat().st_size and hashlib.sha256(body).hexdigest() == sha(path), 'Actual uploaded asset bytes differ')


def publish_release(lock, source, notes, assets):
    allowed = {lock['release']['asset'], lock['release']['asset'] + '.sha256', lock['release']['proof_asset']}
    require(len(assets) == 3 and {path.name for path in assets} == allowed, 'Only APK, checksum and safe proof may be uploaded')
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
        record = api('releases', method='POST', data={'tag_name': lock['release']['tag'], 'target_commitish': lock['sources']['test']['commit'], 'name': lock['release']['title'], 'body': notes, 'draft': True, 'prerelease': True, 'make_latest': 'false'})
        (OUT / 'draft-created.json').write_text(json.dumps({'release_id': record['id'], 'tag': record['tag_name'], 'source': lock['sources']['test']['commit']}, indent=2) + '\n')
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
    require(os.environ.get('APK20023_RELEASE_LOCK_SHA256') == sha(lock_path) and os.environ.get('NUVIO_REVIEW_PLAN_SHA256') == lock['source_plan_sha256'] and digest(os.environ.get('GITHUB_SHA'), 40), 'Wrong publication lock/plan identity')
    require(sha(RECIPE / 'plan.json') == lock['source_plan_sha256'] and sha(RECIPE / 'materialize.py') == lock['source_materializer_sha256'] and sha(RECIPE / 'aggregate.py') == lock['aggregate_recipe_sha256'], 'Reviewed source recipe changed')
    plan = decode((RECIPE / 'plan.json').read_text())
    require(plan['root_reviewed'] is True and plan['frozen'] is True and plan['sources'] == lock['sources'] and plan['identity'] == lock['identity'], 'Plan source/identity differs')
    require(command(['git', 'rev-parse', lock['parent_review'] + '^{tree}']).strip() == lock['parent_tree'], 'Final validation parent tree differs')
    command(['git', 'diff', '--quiet', lock['parent_review'], 'HEAD', '--', '.review/apk20023', '.review/logs8/native', '.review/logs8/kotlin', '.review/logs8/runtime', '.github/workflows/apk20023-review.yml'])
    OUT.mkdir(exist_ok=True)
    require(not BIG.exists(), 'Refusing to replace a prior temporary publication directory')
    BIG.mkdir(parents=True)
    allowed = {'.review/apk20023-publish/publish.py', '.review/apk20023-publish/release-lock.json', '.review/apk20023-publish/release-notes.md', '.github/workflows/apk20023-publish.yml'}
    require(set(command(['git', 'diff', '--name-only', lock['parent_review'], 'HEAD']).splitlines()) == allowed, 'Publisher changes exceed the four reviewed files')
    jobs = verify_run(lock)
    final = lock['validation']
    gate_zip = download(lock['artifacts']['all_gates'], final['run_id'], final['review_commit'], 'all-gates.zip')
    with archive(gate_zip) as z:
        summary, summary_sha = read_json(z, unique_name(z, 'all-gates-verified.json'))
    verify_summary(lock, summary, summary_sha)
    android_zip = download(lock['artifacts']['android'], final['run_id'], final['review_commit'], 'android.zip')
    with archive(android_zip) as z:
        android = verify_android(lock, summary, z)
    source = (BIG / 'source').resolve()
    require(not source.exists(), 'Refusing to replace source materialization')
    command([sys.executable, str(RECIPE / 'materialize.py'), '--layer', 'test', '--output', str(source), '--proof', str((OUT / 'materialized-test.json').resolve()), '--base-repository', str(Path.cwd())])
    require(command(['git', 'rev-parse', 'HEAD', 'HEAD^{tree}', 'HEAD^'], cwd=source).splitlines() == [lock['sources']['test']['commit'], lock['sources']['test']['tree'], lock['sources']['product']['commit']], 'Actual source materialization differs')
    ident = verify_actual_apk(lock, source)
    asset = BIG / lock['release']['asset']
    checksum = OUT / (asset.name + '.sha256')
    checksum.write_text(sha(asset) + '  ' + asset.name + '\n')
    provenance = {'format': 1, 'outcome': 'passed', 'repository': REPOSITORY, 'source_commit': lock['sources']['test']['commit'], 'source_tree': lock['sources']['test']['tree'], 'product_commit': lock['sources']['product']['commit'], 'validation_review_commit': final['review_commit'], 'validation_run_id': final['run_id'], 'validation_run_attempt': final['run_attempt'], 'publication_review_commit': os.environ['GITHUB_SHA'], 'source_plan_sha256': lock['source_plan_sha256'], 'release_lock_sha256': sha(lock_path), 'all_gates_sha256': summary_sha, 'jobs': jobs, 'validation_artifacts': lock['artifacts'], 'features': summary['features'], 'all_producers_same_fresh_run': True, 'apk_sha256': sha(asset), 'apk_bytes': asset.stat().st_size, 'identity': ident, 'android_receipts': android, 'private_configs_or_logs_uploaded': False}
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
