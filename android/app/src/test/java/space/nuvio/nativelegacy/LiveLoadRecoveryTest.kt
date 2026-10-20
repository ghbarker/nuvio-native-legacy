package space.nuvio.nativelegacy

import org.junit.Assert.*
import org.junit.Test
import java.io.EOFException
import java.io.IOException
import java.net.SocketTimeoutException
import javax.net.ssl.SSLHandshakeException

class LiveLoadRecoveryTest {
    private class Fixture {
        var now = 0L
        var frames = 1L
        val recovery = LiveLoadRecovery { now }
        fun observe(buffer: Long = 20_000, live: Boolean = true, started: Boolean = true,
                    paused: Boolean = false, buffering: Boolean = false, segment: Int = 6_000,
                    advance: Boolean = true) {
            if (advance) frames++
            recovery.observe(LiveLoadRecovery.Observation(live, started, paused, buffering,
                now, buffer, frames, segment))
        }
    }
    @Test fun bufferedDeliveryFailureKeepsOneLoaderOwnerAndDoesNotResetItsBudget() {
        val f = Fixture(); f.observe()
        assertEquals(1000L, f.recovery.retry(1, 1))
        assertEquals("loader", f.recovery.report().owner)
        assertEquals(50_000L, f.recovery.report().remainingMs)
        f.now = 10_000; f.observe(buffer = 10_000)
        assertEquals(3000L, f.recovery.retry(1, 5))
        assertEquals(40_000L, f.recovery.report().remainingMs)
        f.recovery.concluded(1)
        assertEquals("none", f.recovery.report().owner)
        assertEquals(0, f.recovery.report().pending)
    }
    @Test fun emptyBufferGetsFiniteSegmentGraceThenOrdinaryFallback() {
        val f = Fixture(); f.observe(buffer = 0, buffering = true, segment = 12_000)
        assertEquals(1000L, f.recovery.retry(1, 1))
        f.now = 33_000; f.observe(buffer = 0, buffering = true, segment = 12_000)
        assertEquals(-1L, f.recovery.retry(1, 12))
        assertEquals("none", f.recovery.report().owner)
    }
    @Test fun readyWithoutNewFramesCannotHideFrozenDecoder() {
        val f = Fixture(); f.observe(); f.recovery.retry(1, 1)
        f.now = 8_001; f.observe(advance = false)
        assertFalse(f.recovery.ownsRetries())
        assertEquals("none", f.recovery.report().owner)
        assertNull(f.recovery.retry(1, 5))
    }
    @Test fun stalePlayerObservationDoesNotExtendRecovery() {
        val f = Fixture(); f.observe(); f.recovery.retry(1, 1)
        f.now = 2501
        assertFalse(f.recovery.ownsRetries())
        assertEquals(0L, f.recovery.report().remainingMs)
    }
    @Test fun manualPauseAndStopNeverStartOrKeepNetworkProtection() {
        val f = Fixture(); f.observe(); f.recovery.retry(1, 1)
        f.observe(paused = true)
        assertFalse(f.recovery.ownsRetries())
        assertEquals(0L, f.recovery.report().remainingMs)
        f.recovery.cancel(); f.observe()
        assertNull(f.recovery.retry(2, 1))
        assertEquals("none", f.recovery.report().owner)
    }
    @Test fun vodAndStartupRetainOrdinaryPolicy() {
        val f = Fixture()
        assertNull(f.recovery.retry(1, 1))
        f.observe(live = false); assertFalse(f.recovery.ownsRetries())
        f.observe(started = false); assertNull(f.recovery.retry(1, 1))
    }
    @Test fun windowRecoveryTakesOwnershipWithoutACompetingLoaderLease() {
        val f = Fixture(); f.observe(); f.recovery.retry(1, 1)
        f.recovery.window(36_000)
        assertEquals("window", f.recovery.report().owner)
        assertEquals(0, f.recovery.report().pending)
        assertNull(f.recovery.retry(2, 1))
        f.recovery.concluded(1) // late old callback cannot end the window lease
        assertEquals("window", f.recovery.report().owner)
        f.recovery.window(0)
        assertEquals("none", f.recovery.report().owner)
    }
    @Test fun largeReserveAndSegmentsCannotCreateUnboundedLease() {
        val f = Fixture(); f.observe(buffer = Long.MAX_VALUE / 2, segment = Int.MAX_VALUE)
        assertEquals(1000L, f.recovery.retry(1, 1))
        assertEquals(60_000L, f.recovery.report().remainingMs)
        assertEquals(-1L, f.recovery.retry(1, 21))
    }
    @Test fun concurrentPlaylistAndSegmentLoadsAreIndependentlyConcluded() {
        val f = Fixture(); f.observe()
        f.recovery.retry(1, 1); f.recovery.retry(2, 1)
        f.recovery.concluded(1)
        assertEquals("loader", f.recovery.report().owner)
        assertEquals(1, f.recovery.report().pending)
        f.recovery.concluded(2); assertEquals("none", f.recovery.report().owner)
    }
    @Test fun serverRetryDelayCannotBeShortenedOrExtendTheDeadline() {
        val f = Fixture(); f.observe()
        assertEquals(12_000L, f.recovery.retry(1, 1, 12_000))
        assertEquals(-1L, f.recovery.retry(1, 2, 60_000))
        assertEquals("none", f.recovery.report().owner)
    }
    @Test fun unrelatedErrorsAndExhaustedParallelDownloadAreNotTransientHlsRetries() {
        assertTrue(LiveLoadErrorPolicy.transient(SocketTimeoutException()))
        assertTrue(LiveLoadErrorPolicy.transient(EOFException()))
        assertFalse(LiveLoadErrorPolicy.transient(SSLHandshakeException("fixture")))
        assertFalse(LiveLoadErrorPolicy.transient(IOException("fixture")))
        assertFalse(LiveLoadErrorPolicy.transient(FalhaDownloadParalelo(SocketTimeoutException())))
    }
}
