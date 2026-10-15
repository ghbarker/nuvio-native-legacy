#!/usr/bin/env python3
"""Verify an immutable successful Android job and the fresh complete macOS group."""
import argparse, hashlib, json, os, re, subprocess, sys, zipfile
from pathlib import Path

REPO = 'ghbarker/nuvio-native-legacy'
SOURCE = '1cd707586a5bc94579904e5601de996e62326067'
TREE = '8cd25f39ec183133a61456ad56cdd78e5475cb89'
BASE = '3c28c0d73664fe82fabced499bd7171810e0c91b'
OLD_REVIEW = 'a267cf58784f40a5f26dd79d1f959ab2369e6e8b'
OLD_RUN = 38674815293
ANDROID_JOB = 116225902823
ARTIFACT = 11946190514
ARTIFACT_NAME = 'public-log-download-android-' + OLD_REVIEW
ZIP_SHA = '067e27a19e3af189ed07948e8fa9301e4cecfb75e1dc005847281b5ebcf17e11'
REPORT_SHA = 'c1fa2178c7f926d6f61ee4005e7216622b9bfdcbaea131b9cbade3e35fa3dafe'
PROOF_SHA = '370bb729cae9f4b2bd0061779058b47708098a124b634dacb361a277766e0602'
APK_SHA = '053484321e28e2cfdbb7927a51822f33633a41b33f883ce9c9eb36c191a98de6'

def sha(p): return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def api(path): return json.loads(subprocess.check_output(['gh', 'api', 'repos/' + REPO + path], text=True))
def proof(data, review):
    assert data['verified'] and data['source_commit'] == SOURCE and data['tree'] == TREE and data['base'] == BASE
    assert data['review_commit'] == review and data['tests_and_workflows_unchanged'] and data['product_commit_recreated_exactly']

def validate_android(root, source):
    assert sha(root / 'android-verified.json') == REPORT_SHA and sha(root / 'source-verified.json') == PROOF_SHA
    report = json.loads((root / 'android-verified.json').read_text()); proof(json.loads((root / 'source-verified.json').read_text()), OLD_REVIEW)
    assert report['outcome'] == 'passed' and report['source_commit'] == SOURCE and report['review_commit'] == OLD_REVIEW
    assert report['apk_sha256'] == APK_SHA and report['apk_bytes'] == 58594756 and report['package'] == 'space.nuvio.nativelegacy'
    assert report['version'] == '2.0.5' and report['version_code'] == 20005 and report['native_16kb_pages'] == 'passed'
    assert report['release_or_backend_actions'] is False
    assert {e['abi'] for e in report['actual_arm_compiles']} == {'arm64-v8a', 'armeabi-v7a'}
    assert all(e['builds'] and all(b['sources'] == ['ajustes.c', 'android.c'] for b in e['builds']) for e in report['actual_arm_compiles'])
    apk = root / 'standard-compile-only-debug.apk'
    assert apk.stat().st_size == report['apk_bytes'] and sha(apk) == APK_SHA
    sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'log-download-public'))
    from dex_methods import apk_methods
    for klass, compiled in report['compiled_android_classes'].items():
        actual, hashes = apk_methods(apk, 'L' + klass.replace('.', '/') + ';')
        assert hashes == report['apk_dex_sha256']
        for signature in compiled['methods']: assert tuple(signature) in actual and not (actual[tuple(signature)] & 8)
    lookups = re.findall(r'GetStaticMethodID\(env,\s*gCls,\s*"([^"]+)",\s*"([^"]+)"\)', (source / 'src/video_android.c').read_text())
    assert [list(e) for e in lookups] == report['all_player_jni_lookups']
    methods, _ = apk_methods(apk, 'Lspace/nuvio/nativelegacy/NvPlayer;')
    assert all(e in methods and methods[e] & 8 for e in lookups)
    assert not apk_methods(apk, 'Lspace/nuvio/nativelegacy/TestLocalLogs;')[0]
    bridge = re.findall(r'GetMethodID\(env,\s*cls,\s*"baixarRegistro",\s*"([^"]+)"\)', (source / 'src/android.c').read_text())
    assert bridge == [report['new_activity_jni_descriptor']]
    subprocess.run([sys.executable, str(source / 'tools/android-apk-pages.py'), str(apk)], check=True, timeout=60)
    return report

def validate_macos(root, source, review):
    proof(json.loads((root / 'source-verified.json').read_text()), review)
    native = json.loads((root / 'native/native-verified.json').read_text())
    assert native['outcome'] == 'passed' and native['source_commit'] == SOURCE and native['review_commit'] == review
    names = ['translations', 'full-source-tv', 'full-source-touch', 'ajustes_envio_auto-original', 'ajustes_padroes-adapted-append-only-expectation',
             'ajustes_ux_toque_modo-original', 'regcodigo-original', 'ajustes_envio_auto-sanitized-replay']
    assert [g['name'] for g in native['gates']] == names and all(g['exit_code'] == 0 for g in native['gates'])
    for gate in native['gates']: assert sha(root / 'native' / (gate['name'] + '.log')) == gate['log_sha256']
    for ui in ('tv', 'touch'): assert sha(root / 'native' / ('app-' + ui)) == native['full_source_compiles'][ui]
    for name, digest in native['original_test_sha256'].items():
        assert sha(source / name) == digest
        assert hashlib.sha256(subprocess.check_output(['git', '-C', str(source), 'show', BASE + ':' + name])).hexdigest() == digest
    assert len(native['original_test_sha256']) == 8 and native['original_reporting_viewer_crash_backend_unchanged']
    defaults = native['defaults_final_enum_metadata_adapter']
    assert defaults['exit_code'] == 0 and defaults['behavioral_default_assertions_unchanged']
    assert defaults['old_final_enum_expectation'] == 'assert(AJ_N == AJ_PROXIMO_CONTAGEM + 1);'
    assert defaults['new_final_enum_expectation'] == 'assert(AJ_N == AJ_FONTE_PRECARREGAR + 1);'
    assert defaults['original_fixture_sha256'] == native['original_test_sha256']['tests/ajustes_padroes.c']
    assert not defaults['original_unadapted_test_counted_as_pass'] and not defaults['product_and_original_tests_modified']
    sanitized = native['sanitized_original_replay']; assert sanitized['exit_code'] == 0 and sanitized['original_source_arguments_and_assertions_unchanged']
    sdk = json.loads((root / 'sdk/sdk-verified.json').read_text())
    assert sdk['outcome'] == 'passed' and sdk['review_commit'] == review and sdk['compiler'] == '/usr/bin/cc'
    assert sdk['required_headers'] == ['stdio.h', 'AvailabilityMacros.h', 'SDL2/SDL.h', 'OpenGL/gl.h']
    assert [c['mode'] for c in sdk['checks']] == ['preprocess', 'compile'] and all(c['exit_code'] == 0 for c in sdk['checks'])
    compiles = [json.loads(p.read_text()) for p in (root / 'compile').glob('*-invocation.json')]
    assert len(compiles) == native['compiler_invocations'] and compiles
    assert all(c['compiler'] == '/usr/bin/cc' and c['returncode'] == 0 for c in compiles)
    assert all(c['effective_arguments'] == c['arguments'] + ([c['added_link_rpath']] if c['added_link_rpath'] else []) for c in compiles)
    return native

def main():
    p = argparse.ArgumentParser(); p.add_argument('--source', type=Path, required=True); p.add_argument('--macos', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True); a = p.parse_args()
    assert os.environ['GH_REPO'] == REPO; review = os.environ['GITHUB_SHA']; assert re.fullmatch('[0-9a-f]{40}', review)
    source = a.source.resolve(); out = a.output.resolve(); out.mkdir(parents=True, exist_ok=True)
    run = api('/actions/runs/' + str(OLD_RUN))
    assert run['id'] == OLD_RUN and run['head_sha'] == OLD_REVIEW and run['status'] == 'completed'
    assert run['conclusion'] == 'failure', 'Original macOS driver failure must not be relabelled a run pass'
    jobs = api('/actions/runs/' + str(OLD_RUN) + '/jobs?per_page=100')['jobs']; selected = [j for j in jobs if j['id'] == ANDROID_JOB]
    assert len(selected) == 1; job = selected[0]
    assert job['run_id'] == OLD_RUN and job['status'] == 'completed' and job['conclusion'] == 'success'
    assert job['name'] == 'Standard Android APK with actual Kotlin/API and ARM32/ARM64 JNI checks'
    required = {'Build native dependencies for both ARM ABIs', 'Build the unchanged standard debug package with production tooling',
                'Verify actual APK DEX, Activity/helper API, JNI, identity and native pages', 'Reverify exact unchanged product source',
                'Run actions/upload-artifact@v4'}
    assert required <= {s['name'] for s in job['steps'] if s['conclusion'] == 'success'}
    artifact = api('/actions/artifacts/' + str(ARTIFACT))
    assert artifact['id'] == ARTIFACT and artifact['name'] == ARTIFACT_NAME and not artifact['expired']
    assert artifact['digest'] == 'sha256:' + ZIP_SHA and artifact['workflow_run']['id'] == OLD_RUN and artifact['workflow_run']['head_sha'] == OLD_REVIEW
    archive = out / 'original-android.zip'
    with archive.open('wb') as f:
        subprocess.run(['gh', 'api', 'repos/' + REPO + '/actions/artifacts/' + str(ARTIFACT) + '/zip'], stdout=f, check=True, timeout=120)
    assert sha(archive) == ZIP_SHA
    android = out / 'original-android'; android.mkdir(exist_ok=True)
    with zipfile.ZipFile(archive) as z:
        assert all(not Path(n).is_absolute() and '..' not in Path(n).parts for n in z.namelist())
        z.extractall(android)
    report = validate_android(android, source); native = validate_macos(a.macos.resolve(), source, review)
    origin = {'outcome': 'passed', 'source_commit': SOURCE, 'source_tree': TREE, 'review_commit': review,
        'android': {'job_id': ANDROID_JOB, 'run_id': OLD_RUN, 'review_commit': OLD_REVIEW, 'conclusion': 'success',
                    'artifact_id': ARTIFACT, 'artifact_name': ARTIFACT_NAME, 'artifact_zip_sha256': ZIP_SHA,
                    'report_sha256': REPORT_SHA, 'source_proof_sha256': PROOF_SHA, 'apk_sha256': APK_SHA,
                    'reused_unchanged_full_build': True, 'rebuilt_in_retry': False},
        'macos': {'run_id': int(os.environ['GITHUB_RUN_ID']), 'review_commit': review, 'fresh_complete_group': True,
                  'gate_count': len(native['gates']), 'native_proof_sha256': sha(a.macos / 'native/native-verified.json'),
                  'source_proof_sha256': sha(a.macos / 'source-verified.json'), 'sdk_proof_sha256': sha(a.macos / 'sdk/sdk-verified.json')},
        'original_mac_failure': {'run_id': OLD_RUN, 'job_id': 116225903161, 'conclusion': 'failure', 'artifact_id': 11944904976,
            'artifact_zip_sha256': '2b9cd8eb4f172bd1e8f9b2a6a1d4981f46cb7afdf736106e6c256e3d47bc1d7d',
            'cause': 'Absolute Xcode clang bypassed Apple /usr/bin/cc SDK driver; unchanged-source standard headers unavailable', 'counted_as_pass': False},
        'original_defaults_failure': {'run_id': 38675944719, 'job_id': 116229449952, 'conclusion': 'failure',
            'artifact_id': 11945843631, 'artifact_zip_sha256': '0e853bf4fa62f7c5bcad26053ea264a49f4842dd3ab1605719e7da88bc2300b4',
            'cause': 'Original stale AJ_N tail assertion expects279 on both baseline and feature; actual baseline tail is290',
            'counted_as_pass': False, 'external_metadata_adapter_explicit': True},
        'no_product_source_changes': True, 'no_pr_release_backend_actions': True}
    (out / 'validation-origin.json').write_text(json.dumps(origin, indent=2) + '\n')
    (out / 'original-android-job.json').write_text(json.dumps(job, indent=2) + '\n')
    (out / 'original-android-artifact.json').write_text(json.dumps(artifact, indent=2) + '\n')
    print('PASS exact successful Android build reuse and fresh complete macOS group; no rebuild or failed-result reuse')

if __name__ == '__main__': main()
