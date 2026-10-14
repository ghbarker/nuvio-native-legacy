#!/usr/bin/env python3
"""Prepare review-only pinned CI; publishing is disabled in the plan by default."""
import copy
import json
from pathlib import Path
import subprocess
import yaml

ROOT = Path(__file__).resolve().parent
REMOTE = '.review/source-preload'
CHECKOUT = {'uses': 'actions/checkout@v4', 'with': {'persist-credentials': False, 'fetch-depth': 0}}
CASE_IF = '${{ matrix.delivery }}'

SHIM = '''set -euo pipefail
compiler="$(command -v cc)"
sdl3_library="$(brew --prefix sdl3)/lib"
test -d "$sdl3_library"
mkdir -p "$GITHUB_WORKSPACE/build/review/cc-bin" "$GITHUB_WORKSPACE/build/review/compile"
cp .review/source-preload/build/cc_rpath.py "$GITHUB_WORKSPACE/build/review/cc-bin/cc"
chmod +x "$GITHUB_WORKSPACE/build/review/cc-bin/cc"
echo "REVIEW_REAL_CC=$compiler" >> "$GITHUB_ENV"
echo "REVIEW_SDL3_LIBRARY_DIR=$sdl3_library" >> "$GITHUB_ENV"
echo "REVIEW_CC_OUTPUT=$GITHUB_WORKSPACE/build/review/compile" >> "$GITHUB_ENV"
echo "$GITHUB_WORKSPACE/build/review/cc-bin" >> "$GITHUB_PATH"'''

SOURCE = 'python3 .review/source-preload/build/verify_source.py --case "${{ matrix.id }}" --source-commit "${{ matrix.source_commit }}" --target source-app --proof build/review/source-verified.json'
ANDROID_SOURCE = SOURCE.replace('--proof build/', '--proof source-app/build/')
APK_VERIFY = 'python3 "$GITHUB_WORKSPACE/.review/source-preload/build/verify_apk.py" @TEST@ --aapt "$ANDROID_HOME/build-tools/35.0.0/aapt" --apksigner "$ANDROID_HOME/build-tools/35.0.0/apksigner"'

def android_steps():
    template = yaml.safe_load(Path('/workspace/review/hide-watched/build/hide-watched-review.yml').read_text())
    steps = []
    for step in copy.deepcopy(template['jobs']['android']['steps']):
        name = step.get('name', '')
        if step.get('uses', '').startswith('actions/checkout@'):
            steps.append(copy.deepcopy(CHECKOUT))
            steps.append({'name': 'Materialize and verify exact pinned product source', 'working-directory': '.', 'run': ANDROID_SOURCE})
            continue
        if name.startswith('Verify retained app'):
            continue
        if name == 'Public application configuration and debug signing':
            step['run'] = step['run'].replace('$GITHUB_WORKSPACE/build/android/product.properties', '$GITHUB_WORKSPACE/source-app/build/android/product.properties')
        if name == 'Verify standard package and both ARM architectures':
            step['name'] = 'Verify standard package, both ARM cores, persistent signature and 16 KB native pages'
            step['run'] = APK_VERIFY.replace('@TEST@', '')
        if name == 'Prepare compatible test installation in CI working copy':
            step['if'] = CASE_IF
            step['run'] = step['run'].split('        # The separately reviewed')[0].split('# The separately reviewed')[0]
            step['run'] = step['run'].replace('20006', '20009').replace('2.0.6-test366', '2.0.9-test-preload1')
        if name == 'Compatible TV and Mobile test APK':
            step['if'] = CASE_IF
            step['run'] = step['run'].replace('2.0.6-test366', '2.0.9-test-preload1')
        if name == 'Verify compatible test package, label, persistent signature and native packaging':
            step['if'] = CASE_IF
            step['run'] = APK_VERIFY.replace('@TEST@', '--test')
        if step.get('uses', '').startswith('actions/upload-artifact@'):
            with_ = step['with']
            with_['path'] = with_['path'].replace('build/', 'source-app/build/')
            if 'standard' in with_['name']:
                with_['name'] = 'source-preload-standard-${{ matrix.id }}-${{ github.sha }}'
            elif 'test' in with_['name']:
                with_['name'] = 'source-preload-test-${{ matrix.id }}-${{ github.sha }}'
            else:
                with_['name'] = 'source-preload-android-proof-${{ matrix.id }}-${{ github.sha }}'
        steps.append(step)
    return steps

def main():
    plan_script = 'python3 .review/source-preload/build/prepare_cases.py'
    matrix = {'fail-fast': False, 'max-parallel': 2, 'matrix': '${{ fromJSON(needs.prepare.outputs.matrix) }}'}
    native = {'name': '${{ matrix.label }} (macOS)', 'needs': 'prepare', 'runs-on': 'macos-14', 'timeout-minutes': 75,
              'strategy': {**matrix, 'matrix': '${{ fromJSON(needs.prepare.outputs.native_matrix) }}'}, 'steps': [copy.deepcopy(CHECKOUT),
                  {'name': 'Materialize and verify exact pinned product source', 'run': SOURCE},
                  {'name': 'Native SDK and real SDL libraries', 'run': 'brew install sdl2 sdl3 sdl2_image sdl2_ttf ffmpeg ripgrep kotlin'},
                  {'name': 'Add only link-time SDL3 rpath while preserving all original compiler flags', 'run': SHIM},
                  {'name': 'Full app TV/Mobile builds and final behavior gates', 'working-directory': 'source-app',
                   'env': {'NUVIO_SOURCE_PRELOAD_REVIEW': '${{ github.workspace }}/.review/source-preload',
                           'NUVIO_SOURCE_PRELOAD_RESULTS': '${{ github.workspace }}/build/review/native',
                           'NUVIO_SOURCE_PRELOAD_CASE': '${{ matrix.id }}'},
                   'run': 'bash "$NUVIO_SOURCE_PRELOAD_REVIEW/build/run_native.sh"'},
                  {'name': 'Reverify pinned product source after native checks', 'run': SOURCE + ' --reverify'},
                  {'uses': 'actions/upload-artifact@v4', 'if': 'always()', 'with': {
                      'name': 'source-preload-native-${{ matrix.id }}-${{ github.sha }}', 'path': 'build/review/', 'retention-days': 14}}]}
    android = {'name': '${{ matrix.label }} (Android ARM32/ARM64)', 'needs': 'prepare', 'runs-on': 'ubuntu-24.04',
               'timeout-minutes': 90, 'strategy': matrix, 'env': {'NUVIO_ANDROID_JOBS': 2},
               'defaults': {'run': {'working-directory': 'source-app'}}, 'steps': android_steps()}
    media3 = {'name': 'Real Media3 paused loading and adoption (API35 x86_64)', 'needs': 'prepare',
              'if': "${{ needs.prepare.outputs.phase == 'final' && needs.prepare.outputs.media3 != 'null' }}", 'runs-on': 'ubuntu-24.04', 'timeout-minutes': 35,
              'steps': [copy.deepcopy(CHECKOUT),
                  {'name': 'Materialize exact Media3 test source and record probe or full-app scope', 'run': 'python3 .review/source-preload/build/verify_source.py --case "${{ fromJSON(needs.prepare.outputs.media3).id }}" --source-commit "${{ fromJSON(needs.prepare.outputs.media3).source_commit }}" --target source-app --proof build/review/source-verified.json'},
                  {'uses': 'actions/setup-java@v4', 'with': {'distribution': 'temurin', 'java-version': '17'}},
                  {'uses': 'gradle/actions/setup-gradle@v4', 'with': {'gradle-version': '8.9'}},
                  {'uses': 'android-actions/setup-android@v3', 'with': {'packages': 'platform-tools', 'log-accepted-android-sdk-licenses': False}},
                  {'name': 'Install probe SDK, NDK, CMake and deterministic media tools', 'run': "sudo apt-get update -qq\nsudo apt-get install -y ffmpeg\nsdkmanager 'platforms;android-35' 'build-tools;35.0.0' 'ndk;27.2.12479018' 'cmake;3.22.1'"},
                  {'name': 'Stage exact NvPlayer and helpers and generate local MP4/HLS assets', 'run': 'bash .review/source-preload/android/stage.sh "$GITHUB_WORKSPACE/source-app"\nbash .review/source-preload/android/generate-fixture.sh\ngradle -p .review/source-preload/android --console=plain :probe:assembleDebug :probe:assembleDebugAndroidTest'},
                  {'name': 'Enable KVM permissions', 'run': "echo 'KERNEL==\"kvm\", GROUP=\"kvm\", MODE=\"0666\", OPTIONS+=\"static_node=kvm\"' | sudo tee /etc/udev/rules.d/99-kvm4all.rules\nsudo udevadm control --reload-rules\nsudo udevadm trigger --name-match=kvm"},
                  {'uses': 'reactivecircus/android-emulator-runner@v2', 'with': {'api-level': 35, 'arch': 'x86_64', 'target': 'google_apis', 'disable-animations': True,
                    'script': 'gradle -p .review/source-preload/android --console=plain :probe:connectedDebugAndroidTest'}},
                  {'name': 'Require all real-player contracts with no ignored tests', 'run': 'python3 .review/source-preload/build/verify_instrumentation.py'},
                  {'uses': 'actions/upload-artifact@v4', 'if': 'always()', 'with': {'name': 'source-preload-media3-${{ github.sha }}',
                    'path': 'build/review/\n.review/source-preload/android/player-source.sha256\n.review/source-preload/android/probe/build/reports/\n.review/source-preload/android/probe/build/outputs/androidTest-results/', 'retention-days': 14}}]}
    publish = {'name': 'Publish uniquely tagged validated compatible test APK', 'needs': ['prepare', 'native', 'android', 'media3'],
               'if': "${{ needs.prepare.outputs.publish == 'true' && needs.prepare.outputs.phase == 'final' }}",
               'runs-on': 'ubuntu-24.04', 'timeout-minutes': 10, 'permissions': {'contents': 'write'},
               'steps': [copy.deepcopy(CHECKOUT),
                   {'uses': 'actions/download-artifact@v4', 'with': {'name': 'source-preload-test-${{ fromJSON(needs.prepare.outputs.delivery).id }}-${{ github.sha }}', 'path': 'delivery/apk'}},
                   {'uses': 'actions/download-artifact@v4', 'with': {'name': 'source-preload-android-proof-${{ fromJSON(needs.prepare.outputs.delivery).id }}-${{ github.sha }}', 'path': 'delivery/proof'}},
                   {'name': 'Verify identity, digest and pinned source then create a new release',
                    'env': {'GH_TOKEN': '${{ github.token }}', 'GH_REPO': '${{ github.repository }}', 'PLANNED_SOURCE': '${{ fromJSON(needs.prepare.outputs.delivery).source_commit }}'},
                    'run': 'python3 .review/source-preload/build/prepare_release.py\nif gh release view tv-mobile-test-source-preload-v1 >/dev/null 2>&1; then\n  echo "The unique preload release tag already exists; no assets will be overwritten."\n  exit 1\nfi\ngh release create tv-mobile-test-source-preload-v1 delivery/Nuvio-TV-Mobile-Test-Preload1.apk delivery/Nuvio-TV-Mobile-Test-Preload1.apk.sha256 delivery/proof/update-identity.json --target "$PLANNED_SOURCE" --prerelease --latest=false --title "Nuvio TV/Mobile Test: source preloading" --notes-file delivery/release-notes.txt'}]}
    workflow = {'name': 'Pinned source-preload, watched rebase, Media3 and compatible test APK',
                'on': {'push': {'branches': ['review/source-preload-v1']}, 'workflow_dispatch': {}},
                'permissions': {'contents': 'read'}, 'concurrency': {'group': 'source-preload-${{ github.sha }}', 'cancel-in-progress': True},
                'jobs': {'prepare': {'runs-on': 'ubuntu-24.04', 'permissions': {'contents': 'write'}, 'outputs': {k: '${{ steps.plans.outputs.' + k + ' }}' for k in ('matrix', 'native_matrix', 'phase', 'publish', 'delivery', 'media3')},
                                 'steps': [copy.deepcopy(CHECKOUT), {'name': 'Verify reviewed patches and publish isolated candidate source commits', 'id': 'plans', 'env': {'GH_TOKEN': '${{ github.token }}'}, 'run': plan_script}, {'uses': 'actions/upload-artifact@v4', 'if': 'always()', 'with': {'name': 'source-preload-materialization-${{ github.sha }}', 'path': 'build/review/', 'retention-days': 14}}]},
                         'native': native, 'android': android, 'media3': media3, 'publish': publish}}
    path = ROOT / 'source-preload-review.yml'
    path.write_text(yaml.safe_dump(workflow, sort_keys=False, width=120))
    checks = []
    for job in workflow['jobs'].values():
        for step in job['steps']:
            if 'run' not in step:
                continue
            # GitHub expressions become ordinary quoted test values for bash syntax checking.
            body = __import__('re').sub(r'\$\{\{.*?\}\}', 'ci-expression', step['run'])
            check = subprocess.run(['bash', '-n'], input=body, text=True, capture_output=True)
            assert check.returncode == 0, (step.get('name'), check.stderr)
            checks.append(step.get('name', 'run'))
    (ROOT / 'workflow-syntax.json').write_text(json.dumps({'yaml_parsed': True, 'bash_steps_checked': len(checks), 'product_commits_materialized_with_real_canonical_parent': True,
      'release_publish_default': False, 'compile_phase_cannot_publish': True, 'github_writes': False}, indent=2) + '\n')
    print(path, len(checks), 'bash steps checked')

if __name__ == '__main__':
    main()
