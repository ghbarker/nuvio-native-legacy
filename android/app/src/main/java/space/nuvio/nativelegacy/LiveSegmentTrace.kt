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
