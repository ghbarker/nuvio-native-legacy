"""Retain all 15 playback contracts and require five local download contracts."""
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
PLAYBACK_TESTS = {'nextEpisodeSamplesLoadWhileCurrentPlayerKeepsPlayingAndTransferOnTransition', 'resumeMatchReusesPlayerAndMismatchStartsAtCorrectPosition', 'nextSeparateAudioHlsMeasuresColdPreferredRenditionAfterWarmDefault', 'highBitrateTitleByteReadyMeasuresFirstFrameBeforeAdditionalNetwork', 'earlyAudioTitleKeepsMatchingEnglishAndPrimesPortugueseWhileManualOverrideWorks', 'hlsPreparationKeepsManifestAndBufferedSegmentsOnAdoption', 'cancelQueuedRequestsAndStaleTokensStaySilent', 'opaqueMultiAudioMkvTitleAutomaticChoiceRetainsPlayerButMeasuresSinkReset', 'percentageResumeAndCacheAreTransferredAndHeadersMustMatch', 'nextMultiAudioMkvRetainsBufferedPeriodAndMeasuresFirstFrameAndAudioChoice', 'earlyAudioNextHlsLoadsPreferredMediaBeforePlayAndRetainsItsAudioTrack', 'nextEpisodePercentageResumeAndCancellationPreserveCurrentSession', 'audioPreferenceMismatchDiscardsStalePreparedPlayerAndKeepsColdDefaults', 'adoptedPlayerStillSupportsOrdinarySeekAndConfigMismatchFallsBack', 'pausedNetworkLoadIsAdoptedWithoutResetOrResolverReplay'}
LOCAL_LOG_TESTS = {'localLogTailAndSnapshotPreserveCurrentAndPreviousSessions', 'localLogDocumentRequestAndCancelKeepOtherActivityResultsAvailable', 'localLogResultWritesRedactedFileThroughContentResolver', 'localDownloadUsesDocumentPickerAndPreventsStackedRequests', 'localLogRedactionRemovesProviderCredentialsAndKeepsNextState'}
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
    assert required == PLAYBACK_TESTS | LOCAL_LOG_TESTS and len(required) == 20, 'All 15 playback and five download contracts are mandatory'
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
