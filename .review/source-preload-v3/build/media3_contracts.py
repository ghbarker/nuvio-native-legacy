"""Frozen V2 runtime contract set; retain every successful V1 contract."""
import hashlib
import json
from pathlib import Path

ORIGINAL_TESTS = {
    'pausedNetworkLoadIsAdoptedWithoutResetOrResolverReplay',
    'cancelQueuedRequestsAndStaleTokensStaySilent',
    'resumeMatchReusesPlayerAndMismatchStartsAtCorrectPosition',
    'percentageResumeAndCacheAreTransferredAndHeadersMustMatch',
    'hlsPreparationKeepsManifestAndBufferedSegmentsOnAdoption',
    'adoptedPlayerStillSupportsOrdinarySeekAndConfigMismatchFallsBack',
    'nextEpisodeSamplesLoadWhileCurrentPlayerKeepsPlayingAndTransferOnTransition',
    'nextEpisodePercentageResumeAndCancellationPreserveCurrentSession',
}
PRELOAD_CLASS = 'space.nuvio.preloadtest.PreloadContractTest'
STARTUP_CLASS = 'space.nuvio.preloadtest.StartupContractTest'
STARTUP_FILE = 'probe/src/androidTest/java/space/nuvio/preloadtest/StartupContractTest.kt'

def load_contracts(path):
    path = Path(path)
    manifest = json.loads(path.read_text())
    listed = manifest['runtime_test_methods']
    required = set(listed)
    assert len(listed) == len(required), 'Duplicate runtime method names'
    assert ORIGINAL_TESTS <= required, 'An original V1 runtime contract was removed'
    startup = set(manifest.get('startup_test_methods', []))
    assert startup and startup <= required and not startup & ORIGINAL_TESTS, 'Expanded startup contracts are required'
    helpers = {entry['path']: entry for entry in manifest['helper_files']}
    startup_files = [entry for key, entry in helpers.items() if key.endswith('/StartupContractTest.kt')]
    assert len(startup_files) == 1, 'Startup test class omitted or ambiguous in frozen helper graph'
    classes = manifest.get('runtime_test_classes', {
        PRELOAD_CLASS: sorted(required - startup), STARTUP_CLASS: sorted(startup)})
    mapped = {name for names in classes.values() for name in names}
    assert mapped == required, 'Class map and required runtime contract set differ'
    assert ORIGINAL_TESTS <= set(classes.get(PRELOAD_CLASS, []))
    assert startup <= set(classes.get(STARTUP_CLASS, [])), 'Startup contracts must run in their actual test class'
    return manifest, required, startup, classes, hashlib.sha256(path.read_bytes()).hexdigest()
