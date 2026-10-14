#!/usr/bin/env python3
import hashlib
import json
from pathlib import Path
import xml.etree.ElementTree as ET

probe = Path('.review/source-preload-final/android')
required = {
    'pausedNetworkLoadIsAdoptedWithoutResetOrResolverReplay',
    'cancelQueuedRequestsAndStaleTokensStaySilent',
    'resumeMatchReusesPlayerAndMismatchStartsAtCorrectPosition',
    'percentageResumeAndCacheAreTransferredAndHeadersMustMatch',
    'hlsPreparationKeepsManifestAndBufferedSegmentsOnAdoption',
    'adoptedPlayerStillSupportsOrdinarySeekAndConfigMismatchFallsBack',
    'nextEpisodeSamplesLoadWhileCurrentPlayerKeepsPlayingAndTransferOnTransition',
    'nextEpisodePercentageResumeAndCancellationPreserveCurrentSession',
}
reports = list((probe / 'probe/build/outputs/androidTest-results').rglob('*.xml'))
assert reports, 'No instrumentation XML results were produced'
passed = set()
for report in reports:
    root = ET.parse(report).getroot()
    for test in root.iter('testcase'):
        assert test.find('failure') is None and test.find('error') is None and test.find('skipped') is None, test.attrib
        passed.add(test.attrib['name'].split('[')[0])
assert required <= passed, (required, passed)
names = ('NvPlayer', 'AudioSyncSink', 'AudioSyncTap', 'CacheMidia', 'CacheSessao',
         'GanhoAudioProcessor', 'GanhoMath', 'ParaleloDataSource', 'PassivoMedidor', 'RecriaHdr')
hashes = []
for name in names:
    relative = 'space/nuvio/nativelegacy/' + name + '.kt'
    staged = (probe / 'probe/src/player' / relative).read_bytes()
    actual = (Path('source-app/android/app/src/main/java') / relative).read_bytes()
    assert staged == actual, name
    hashes.append({'name': name, 'sha256': hashlib.sha256(actual).hexdigest()})
source = json.loads(Path('build/review/source-verified.json').read_text())
assert source['verified'] and source['phase'] == 'final'
proof = {'outcome': 'passed', 'source_commit': source['source_commit'], 'review_commit': source['build_commit'],
         'case': source['case'], 'source_scope': source['scope'],
         'real_media3_version': '1.8.0', 'emulator': 'API35 x86_64', 'required_tests': sorted(required),
         'tests_seen': sorted(passed), 'exact_staged_product_sources': hashes,
         'probe_scope': 'Real NvPlayer and Media3 with a tiny Activity and JNI callback-counting doubles; production FFmpeg decoder extension absent'}
Path('build/review/media3-verified.json').write_text(json.dumps(proof, indent=2) + '\n')
print('All real Media3 paused-load/adoption contracts passed against', source['source_commit'])
