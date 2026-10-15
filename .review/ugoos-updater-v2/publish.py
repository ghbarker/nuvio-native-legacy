"""Attach only the two reviewed Windows helper files to the existing Logs18 release."""
from pathlib import Path
import hashlib, json, os, urllib.parse, urllib.request

ROOT_REVIEWED = True
REPOSITORY = 'ghbarker/nuvio-native-legacy'
RELEASE_ID = 414325097
TAG = 'tv-mobile-test-logs-v18'
SOURCE = '412d6bdc2a9eb6b3483626f765833e27fb82299d'
APK_ID = 645333818
APK_NAME = 'Nuvio-TV-Mobile-Test-Logs18.apk'
APK_BYTES = 59005344
APK_SHA256 = '2c7d7155350b2a2db4202fa49229e56c250af09308a14a85e379aa68d5f2e0b7'
# Filled only from the helper owner's final immutable byte manifest.
FILES = {
    'Nuvio-Ugoos-Update-v2.ps1': {'bytes': 9435, 'sha256': '6ba4e63f26177c5cd8cb779a5a935d7a60bb0f0e434986375bbd93ac02fe6232'},
    'windows-ugoos-update-v2.txt': {'bytes': 1915, 'sha256': '6320cc3ef34356e3f3385655ba878f02e6cd9b58cf3f3eed9a657009533547c3'},
}
HERE = Path(__file__).resolve().parent

def digest(data):
    return hashlib.sha256(data).hexdigest()

class SafeRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, request, response, code, message, headers, new_url):
        target = urllib.parse.urlsplit(new_url)
        assert target.scheme == 'https'
        redirected = super().redirect_request(request, response, code, message, headers, new_url)
        if redirected and target.netloc != urllib.parse.urlsplit(request.full_url).netloc:
            for values in [redirected.headers, redirected.unredirected_hdrs]:
                for key in list(values):
                    if key.lower() in ['authorization', 'cookie']:
                        del values[key]
        return redirected

def main():
    assert ROOT_REVIEWED is True, 'Root review is required before publication'
    assert os.environ['GITHUB_REPOSITORY'] == REPOSITORY
    assert set(FILES) == {'Nuvio-Ugoos-Update-v2.ps1', 'windows-ugoos-update-v2.txt'}
    contents = {name: (HERE / name).read_bytes() for name in FILES}
    for name, spec in FILES.items():
        assert len(contents[name]) == spec['bytes'] and digest(contents[name]) == spec['sha256'], name
    token = os.environ['GITHUB_TOKEN']
    opener = urllib.request.build_opener(SafeRedirect())

    def api(path, data=None):
        url = path if path.startswith('https://uploads.github.com/') else 'https://api.github.com/repos/' + REPOSITORY + '/' + path
        parsed = urllib.parse.urlsplit(url)
        assert parsed.scheme == 'https' and parsed.hostname in ['api.github.com', 'uploads.github.com']
        headers = {'Authorization': 'Bearer ' + token, 'Accept': 'application/vnd.github+json', 'X-GitHub-Api-Version': '2022-11-28'}
        if data is not None:
            headers['Content-Type'] = 'application/octet-stream'
        request = urllib.request.Request(url, data=data, headers=headers, method='POST' if data is not None else 'GET')
        with opener.open(request, timeout=60) as response:
            assert response.status == (201 if data is not None else 200)
            return json.load(response)

    def release():
        value = api('releases/' + str(RELEASE_ID))
        assert value['id'] == RELEASE_ID and value['tag_name'] == TAG and value['draft'] is False
        assert value['target_commitish'] == SOURCE
        ref = api('git/ref/tags/' + TAG)
        assert ref['object']['type'] == 'commit' and ref['object']['sha'] == SOURCE
        apk = [asset for asset in value['assets'] if asset['id'] == APK_ID]
        assert len(apk) == 1
        apk = apk[0]
        assert apk['name'] == APK_NAME and apk['size'] == APK_BYTES and apk['state'] == 'uploaded'
        assert apk['digest'] == 'sha256:' + APK_SHA256
        return value

    def bytes_verified(asset, name):
        spec = FILES[name]
        assert asset['name'] == name and asset['state'] == 'uploaded' and asset['size'] == spec['bytes']
        if asset.get('digest'):
            assert asset['digest'] == 'sha256:' + spec['sha256']
        url = 'https://github.com/' + REPOSITORY + '/releases/download/' + TAG + '/' + urllib.parse.quote(name)
        # Public download requests intentionally carry no GitHub token.
        with opener.open(urllib.request.Request(url), timeout=60) as response:
            assert response.status == 200
            data = response.read(spec['bytes'] + 1)
        assert data == contents[name] and digest(data) == spec['sha256'], name
        return {'id': asset['id'], 'name': name, 'bytes': len(data), 'sha256': digest(data), 'download_url': url, 'public_download_verified': True}

    before = release()
    before_assets = {asset['id']: (asset['name'], asset['size'], asset.get('digest')) for asset in before['assets']}
    uploaded = []
    for name in FILES:
        current = release()
        existing = [asset for asset in current['assets'] if asset['name'] == name]
        assert len(existing) <= 1
        if existing:
            proof = bytes_verified(existing[0], name)
            proof['action'] = 'reused-identical'
        else:
            upload = 'https://uploads.github.com/repos/' + REPOSITORY + '/releases/' + str(RELEASE_ID) + '/assets?' + urllib.parse.urlencode({'name': name})
            asset = api(upload, contents[name])
            proof = bytes_verified(asset, name)
            proof['action'] = 'uploaded'
        uploaded.append(proof)
    after = release()
    after_assets = {asset['id']: (asset['name'], asset['size'], asset.get('digest')) for asset in after['assets']}
    assert all(after_assets.get(key) == value for key, value in before_assets.items()), 'An existing asset changed'
    result = {'passed': True, 'review_commit': os.environ['GITHUB_SHA'], 'run_id': int(os.environ['GITHUB_RUN_ID']), 'release_id': RELEASE_ID, 'tag': TAG, 'target_source': SOURCE, 'existing_apk_untouched': True, 'sdk_or_tests_or_apk_uploads': False, 'assets': uploaded}
    output = Path('results/ugoos-updater-v2-publication.json')
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result), flush=True)

if __name__ == '__main__':
    main()
