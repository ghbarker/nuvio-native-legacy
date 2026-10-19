package space.nuvio.nativelegacy

// TEST BUILD ONLY: numeric segment history for the live replay investigation.
// No URI, header, media label, exception message or payload enters this buffer.
internal class LiveSegmentTrace {
    private val recent = ArrayDeque<String>()
    private var lastReport: Long? = null

    fun record(track: Int, startMs: Long, endMs: Long, bytes: Long, loadMs: Long, failed: Boolean) {
        if (recent.size == 12) recent.removeFirst()
        recent.addLast("$track:$startMs..$endMs bytes=$bytes loadMs=$loadMs failed=$failed")
    }

    fun snapshot(nowMs: Long): String? {
        if (recent.isEmpty()) return null
        val last = lastReport
        if (last != null && nowMs - last < 15_000L) return null
        lastReport = nowMs
        val result = recent.joinToString("; ")
        recent.clear()
        return result
    }
}

// TEST BUILD ONLY: transport timing, never addresses, headers or content.
internal class LiveTransferTiming(private val startMs: Long) {
    private var openedMs = startMs
    private var firstMs: Long? = null
    private var previousMs: Long? = null
    private var largestGapMs = 0L
    private var received = 0L

    fun opened(nowMs: Long) { openedMs = nowMs }
    fun bytes(count: Int, nowMs: Long) {
        if (count <= 0) return
        if (firstMs == null) firstMs = nowMs
        previousMs?.let { largestGapMs = maxOf(largestGapMs, nowMs - it) }
        previousMs = nowMs
        received += count
    }
    fun summary(nowMs: Long): String? {
        if (received < 262144) return null
        return "bytes=$received openMs=${openedMs - startMs} firstByteMs=${(firstMs ?: openedMs) - startMs} bodyMs=${nowMs - (firstMs ?: openedMs)} maxReadGapMs=$largestGapMs totalMs=${nowMs - startMs}"
    }
}
