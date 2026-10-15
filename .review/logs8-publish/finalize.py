#!/usr/bin/env python3
"""Finish the exact uploaded draft after its tag lookup failed; never replace assets."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

REPO = 'ghbarker/nuvio-native-legacy'
RELEASE_ID = 413425920
SOURCE = '260c83a604b2a35787714a2946566d1dfd049704'
ORIGIN = '2ba1300688960fd801e844bb378bb0394fa784a3'
LOCK_SHA = 'd60ba1e9080444716c330b71ef25e174d8dc2a75e448176131ad805a05651a8f'
ASSETS = {
    'Nuvio-TV-Mobile-Test-Logs8.apk': (642691654, 58922208, '4f5d34215ec5ad4f25f9283ef3ac82c75df6ad230c94a23b40b4c31495091130'),
    'Nuvio-TV-Mobile-Test-Logs8.apk.sha256': (642691652, 97, '0b3ed095c692f50fcce49ac6dd3a310be40170c556021d469e65b874eeafce54'),
    'Logs8-validation.json': (642691653, 6552, '19db5ac93bb3d5a43c698b045b1dd90e896aeb821704687d8865407c1a4c9a25'),
}


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def api(path, data=None, binary=False):
    args = ['gh', 'api', 'repos/' + REPO + '/' + path]
    if binary:
        args += ['-H', 'Accept: application/octet-stream']
    if data is not None:
        args += ['--method', 'PATCH', '--input', '-']
    p = subprocess.run(args, input=json.dumps(data).encode() if data is not None else None, capture_output=True)
    require(p.returncode == 0, 'GitHub request failed: ' + path)
    return p.stdout if binary else json.loads(p.stdout)


def main():
    require(os.environ.get('GITHUB_REPOSITORY') == REPO and os.environ.get('GITHUB_REF') == 'refs/heads/review/logs8-publish-20261015', 'Wrong finalization repository or branch')
    here = Path(__file__).resolve().parent
    raw = (here/'release-lock.json').read_bytes()
    require(hashlib.sha256(raw).hexdigest() == LOCK_SHA, 'Reviewed lock changed')
    lock = json.loads(raw)
    require(lock['root_reviewed'] is True, 'Release lock was not reviewed')
    require(api('git/ref/tags/' + lock['release']['tag'])['object']['sha'] == SOURCE, 'Release tag changed')
    run = api('actions/runs/38681906448')
    require(run['head_sha'] == lock['final_validation']['review_commit'] and run['status'] == 'completed' and run['conclusion'] == 'success', 'Final validation is not green')
    release = api('releases/' + str(RELEASE_ID))
    notes = (here/'release-notes.md').read_text()
    require(hashlib.sha256(notes.encode()).hexdigest() == lock['release']['draft_notes_sha256'], 'Release notes changed')
    require(release['id'] == RELEASE_ID and release['tag_name'] == lock['release']['tag'] and release['target_commitish'] == SOURCE, 'Wrong release or source')
    require(release['name'] == lock['release']['title'] and release['prerelease'] is True and release['body'].replace('\r\n', '\n').rstrip() == notes.rstrip(), 'Release title, notes or type differs')
    require(len(release['assets']) == len(ASSETS) and {a['name'] for a in release['assets']} == set(ASSETS), 'Release assets differ')
    payloads = {}
    for asset in release['assets']:
        asset_id, size, sha = ASSETS[asset['name']]
        require(asset['id'] == asset_id and asset['size'] == size and asset['state'] == 'uploaded' and asset.get('digest') == 'sha256:' + sha, 'Asset metadata changed')
        body = api('releases/assets/' + str(asset_id), binary=True)
        require(len(body) == size and hashlib.sha256(body).hexdigest() == sha, 'Actual uploaded asset bytes differ')
        payloads[asset['name']] = body
    proof = json.loads(payloads['Logs8-validation.json'])
    require(proof['outcome'] == 'passed' and proof['source_commit'] == SOURCE and proof['publication_review_commit'] == ORIGIN, 'Uploaded validation proof differs')
    require(proof['release_lock_sha256'] == LOCK_SHA and proof['all_gates_sha256'] == 'dff656deeb1ff05e381e971adcd36deb0e4221f1f9ce53096e758eacea0317ce', 'Uploaded validation lock/gates differ')
    require(proof['identity'] == {**lock['identity'], 'native_16kb_pages': 'passed', 'abis': ['arm64-v8a', 'armeabi-v7a']} and proof['apk_sha256'] == lock['apk']['sha256'] and proof['apk_bytes'] == lock['apk']['bytes'], 'APK validation identity differs')
    require(payloads['Nuvio-TV-Mobile-Test-Logs8.apk.sha256'].decode() == lock['apk']['sha256'] + '  Nuvio-TV-Mobile-Test-Logs8.apk\n', 'Checksum file differs')
    if release['draft']:
        release = api('releases/' + str(RELEASE_ID), {'draft': False, 'prerelease': True, 'make_latest': 'false'})
    require(release['draft'] is False and release['prerelease'] is True, 'Release was not published')
    out = Path('publication'); out.mkdir(exist_ok=True)
    receipt = {**proof, 'outcome': 'published-and-uploaded-bytes-verified', 'publication_run_id': int(os.environ['GITHUB_RUN_ID']), 'finalization_review_commit': os.environ['GITHUB_SHA'], 'original_publisher_run_id': 38682900053, 'original_publisher_run_conclusion': 'failure', 'original_failure': 'Draft release exists but GitHub tag lookup returned 404 after creation', 'release_id': RELEASE_ID, 'url': 'https://github.com/' + REPO + '/releases/download/' + lock['release']['tag'] + '/Nuvio-TV-Mobile-Test-Logs8.apk', 'no_asset_tag_or_source_replacement': True}
    (out/'publication-receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(receipt['url'])


if __name__ == '__main__':
    main()
