package space.nuvio.nativelegacy

import android.net.Uri
import androidx.media3.common.C
import androidx.media3.datasource.DataSpec
import androidx.media3.datasource.HttpDataSource
import androidx.media3.exoplayer.source.LoadEventInfo
import androidx.media3.exoplayer.source.MediaLoadData
import androidx.media3.exoplayer.upstream.DefaultLoadErrorHandlingPolicy
import androidx.media3.exoplayer.upstream.LoadErrorHandlingPolicy
import org.junit.Assert.*
import org.junit.Test

class LiveLoadErrorPolicyTest {
    private var now = 0L
    private val recovery = LiveLoadRecovery { now }
    private val ordinary = DefaultLoadErrorHandlingPolicy()
    private val policy = LiveLoadErrorPolicy(ordinary, recovery)
    private val spec = DataSpec(Uri.parse("https://example.test/live.m3u8"))
    private fun observe(live: Boolean = true, paused: Boolean = false) {
        recovery.observe(LiveLoadRecovery.Observation(live, true, paused, false, now, 20_000, now + 1, 6000))
    }
    private fun failure(code: Int, count: Int = 1, headers: Map<String, List<String>> = emptyMap()): LoadErrorHandlingPolicy.LoadErrorInfo {
        val error = HttpDataSource.InvalidResponseCodeException(code, "fixture", null, headers, spec, byteArrayOf())
        return LoadErrorHandlingPolicy.LoadErrorInfo(LoadEventInfo(1, spec, 0L), MediaLoadData(C.DATA_TYPE_MEDIA), error, count)
    }
    @Test fun temporaryHttpErrorsUseBoundedRetriesWithoutReplacementRequests() {
        observe()
        assertEquals(Int.MAX_VALUE, policy.getMinimumLoadableRetryCount(C.DATA_TYPE_MEDIA))
        assertEquals(1000L, policy.getRetryDelayMsFor(failure(503)))
        assertEquals("loader", recovery.report().owner)
        policy.onLoadTaskConcluded(1)
        assertEquals("none", recovery.report().owner)
    }
    @Test fun authMissingAndGoneResponsesRetainOrdinaryBudgetEvenAfterUnlimitedMinimumWasCaptured() {
        observe()
        assertEquals(Int.MAX_VALUE, policy.getMinimumLoadableRetryCount(C.DATA_TYPE_MEDIA))
        for (code in listOf(401, 403, 404, 410)) {
            val first = failure(code)
            assertEquals(ordinary.getRetryDelayMsFor(first), policy.getRetryDelayMsFor(first))
            assertEquals(C.TIME_UNSET, policy.getRetryDelayMsFor(failure(code, 4)))
            assertEquals("none", recovery.report().owner)
        }
    }
    @Test fun vodDoesNotGetTheExtendedRetryAllowance() {
        observe(live = false)
        assertEquals(ordinary.getMinimumLoadableRetryCount(C.DATA_TYPE_MEDIA), policy.getMinimumLoadableRetryCount(C.DATA_TYPE_MEDIA))
        val f = failure(503)
        assertEquals(ordinary.getRetryDelayMsFor(f), policy.getRetryDelayMsFor(f))
        assertEquals("none", recovery.report().owner)
    }
    @Test fun exhaustedParallelCycleCannotBecomeAnExtendedHttpRetry() {
        val http = failure(503).exception
        assertFalse(LiveLoadErrorPolicy.transient(FalhaDownloadParalelo(http)))
    }
    @Test fun retryAfterIsRespectedAndNeverCreatesAnUnboundedWait() {
        observe()
        assertEquals(12_000L, policy.getRetryDelayMsFor(failure(429, headers = mapOf("Retry-After" to listOf("12")))))
        assertEquals(C.TIME_UNSET, policy.getRetryDelayMsFor(failure(503, headers = mapOf("Retry-After" to listOf("120")))))
    }
    @Test fun pauseAndStaleUiObservationsRestoreTheOrdinaryBudget() {
        observe()
        policy.getRetryDelayMsFor(failure(503))
        observe(paused = true)
        assertEquals(C.TIME_UNSET, policy.getRetryDelayMsFor(failure(503, 4)))
        observe(); now = 3000
        assertEquals(ordinary.getMinimumLoadableRetryCount(C.DATA_TYPE_MEDIA), policy.getMinimumLoadableRetryCount(C.DATA_TYPE_MEDIA))
        assertEquals(C.TIME_UNSET, policy.getRetryDelayMsFor(failure(503, 4)))
    }
}
