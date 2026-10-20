package space.nuvio.nativelegacy

/** One bounded owner for transient HLS retries and same-player window recovery.
 * Player observations arrive on its application looper; Loader queries use copies only.
 * This never opens a connection, changes a URL, rewrites timestamps or touches Player.
 */
internal class LiveLoadRecovery(private val clock: () -> Long) {
    data class Observation(val liveHls: Boolean, val started: Boolean, val paused: Boolean,
        val buffering: Boolean, val positionMs: Long, val bufferedMs: Long,
        val renderedFrames: Long, val segmentMs: Int)
    data class Report(val owner: String, val reason: String, val remainingMs: Long,
        val bufferedMs: Long, val pending: Int, val retries: Int, val revision: Long)
    private var observation: Observation? = null
    private var observedAt = -10_000L
    private var movedAt = -10_000L
    private var windowUntil = 0L
    private var closed = false
    private val tasks = LinkedHashMap<Long, Long>()
    private var retries = 0
    private var revision = 0L
    private var reason = "idle"

    @Synchronized fun observe(next: Observation) {
        val now = clock()
        val previous = observation
        val moved = previous == null || if (next.renderedFrames >= 0)
            next.renderedFrames != previous.renderedFrames
        else next.positionMs > previous.positionMs
        if (moved) movedAt = now
        observation = next
        observedAt = now
    }

    private fun eligible(now: Long): Boolean {
        val o = observation ?: return false
        return !closed && o.liveHls && o.started && !o.paused &&
            now - observedAt in 0..2_500L && (o.buffering || now - movedAt < 8_000L)
    }
    @Synchronized fun ownsRetries(): Boolean = eligible(clock()) && windowUntil <= clock()

    /** null delegates unchanged to the ordinary policy; -1 exhausts this recovery. */
    @Synchronized fun retry(task: Long, errorCount: Int, minimumDelayMs: Long = 0): Long? {
        val now = clock()
        if (!eligible(now) || windowUntil > now) return null
        val o = observation!!
        val until = tasks[task] ?: run {
            if (tasks.size >= 8) { changed("task-limit"); return -1L }
            val reserve = (o.bufferedMs - (now - observedAt)).coerceAtLeast(0L)
            val grace = LiveWindowRecovery.timeoutForSegment(o.segmentMs)
            (now + (reserve + grace).coerceIn(8_000L, 60_000L)).also { tasks[task] = it }
        }
        val delay = maxOf((errorCount * 1_000L).coerceIn(1_000L, 3_000L), minimumDelayMs)
        if (errorCount > 20 || now + delay >= until) {
            tasks.remove(task)
            changed("exhausted")
            return -1L
        }
        retries++
        changed(if (o.bufferedMs > 0 && !o.buffering) "retry-buffered" else "retry-buffering")
        return delay
    }
    @Synchronized fun concluded(task: Long) {
        if (tasks.remove(task) != null) changed("load-concluded")
    }
    @Synchronized fun window(timeoutMs: Long) {
        tasks.clear()
        windowUntil = if (timeoutMs > 0 && !closed) clock() + timeoutMs.coerceAtMost(60_000L) else 0
        changed(if (windowUntil > 0) "window-start" else "window-finished")
    }
    @Synchronized fun cancel() {
        closed = true
        tasks.clear()
        windowUntil = 0
        changed("cancelled")
    }
    private fun changed(why: String) { reason = why; revision++ }
    @Synchronized fun report(): Report {
        val now = clock()
        val networkUntil = if (eligible(now)) tasks.values.minOrNull() ?: 0 else 0
        val owner = when {
            closed -> "none"
            windowUntil > now -> "window"
            networkUntil > now -> "loader"
            else -> "none"
        }
        val remaining = when (owner) {
            "window" -> windowUntil - now
            "loader" -> networkUntil - now
            else -> 0
        }
        return Report(owner, reason, remaining, observation?.bufferedMs ?: 0, tasks.size, retries, revision)
    }
}
