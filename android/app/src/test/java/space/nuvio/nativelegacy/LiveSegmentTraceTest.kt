package space.nuvio.nativelegacy

import org.junit.Assert.*
import org.junit.Test

class LiveSegmentTraceTest {
    @Test fun preservesBackwardsSegmentOrderAndBoundsHistory() {
        val trace = LiveSegmentTrace()
        for (i in 0..15) trace.record(1, i * 1000L, (i + 1) * 1000L, 100, 20, false)
        trace.record(1, 7000, 8000, 100, 20, false)
        val report = trace.snapshot(0)!!
        assertEquals(12, report.split("; ").size)
        assertTrue(report.startsWith("1:5000..6000"))
        assertTrue(report.endsWith("1:7000..8000 bytes=100 loadMs=20 failed=false"))
        assertNull(trace.snapshot(15_000))
    }

    @Test fun limitsReportsWithoutLosingNewLoadsOrCrossingSessions() {
        val trace = LiveSegmentTrace()
        assertNull(trace.snapshot(0))
        trace.record(2, 0, 1000, 200, 10, false)
        assertNotNull(trace.snapshot(100))
        trace.record(2, 1000, 2000, 0, 500, true)
        assertNull(trace.snapshot(15_099))
        assertTrue(trace.snapshot(15_100)!!.contains("failed=true"))
        val fresh = LiveSegmentTrace()
        fresh.record(2, 0, 1000, 200, 10, false)
        assertNotNull(fresh.snapshot(15_101))
    }
    @Test fun distinguishesServerWaitFromSlowBody() {
        val timing = LiveTransferTiming(100)
        timing.opened(6100)
        timing.bytes(262144, 6200)
        timing.bytes(262144, 9200)
        assertEquals("bytes=524288 openMs=6000 firstByteMs=6100 bodyMs=3100 maxReadGapMs=3000 totalMs=9200", timing.summary(9300))
    }

    @Test fun smallResponsesAndEmptyTransfersDoNotProduceLogs() {
        val timing = LiveTransferTiming(0)
        assertNull(timing.summary(500))
        timing.bytes(-1, 500)
        timing.bytes(1024, 600)
        assertNull(timing.summary(1000))
    }

}
