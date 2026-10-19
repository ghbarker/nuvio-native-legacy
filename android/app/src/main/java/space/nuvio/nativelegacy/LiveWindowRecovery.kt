package space.nuvio.nativelegacy

import androidx.media3.common.PlaybackException
import androidx.media3.common.Player
import androidx.media3.common.util.UnstableApi
import androidx.media3.exoplayer.hls.playlist.HlsPlaylistTracker
import androidx.media3.datasource.HttpDataSource

// Recover the existing live player before asking native code to replace its URL.
// The source's default live position respects its advertised live window/holdback.
@androidx.annotation.OptIn(UnstableApi::class)
internal class LiveWindowRecovery(
    private val schedule: (Runnable, Long) -> Unit,
    private val unschedule: (Runnable) -> Unit,
    private val active: () -> Boolean,
    private val status: (Long) -> Unit = {},
    private val report: (String) -> Unit = {},
) {
    private var pending: Runnable? = null
    private var windowStart = -1L
    private var attempts = 0
    private var stuckAttempted = false
    private var refusedAttempted = false
    private var pendingReason = ""

    companion object {
        // Same bounded HLS policy as nv_ls_buffer_prazo in the native watchdog.
        fun timeoutForSegment(segmentMs: Int): Long = when {
            segmentMs <= 0 -> 8_000L
            segmentMs >= 20_000 -> 60_000L
            else -> maxOf(30_000L, segmentMs * 3L)
        }
    }

    fun recover(player: Player, error: PlaybackException, nowMs: Long, timeoutMs: Long = 8_000L,
                failed: () -> Unit): Boolean {
        if (!active()) return false
        // A fatal load error releases the tracker. Prepare re-reads the original
        // MediaItem URI, including provider redirects, rather than retrying the
        // refused segment URI. No URL/signature rewriting or account mutation.
        val causes = generateSequence(error.cause) { it.cause }.take(8).toList()
        val reason = when {
            error.errorCode == PlaybackException.ERROR_CODE_BEHIND_LIVE_WINDOW -> "window"
            !player.isCurrentMediaItemLive -> return false
            error.errorCode == PlaybackException.ERROR_CODE_IO_UNSPECIFIED &&
                causes.any { it is HlsPlaylistTracker.PlaylistResetException } -> "reset"
            error.errorCode == PlaybackException.ERROR_CODE_IO_UNSPECIFIED &&
                causes.any { it is HlsPlaylistTracker.PlaylistStuckException } -> "stuck"
            error.errorCode == PlaybackException.ERROR_CODE_IO_BAD_HTTP_STATUS &&
                causes.any { it is HttpDataSource.InvalidResponseCodeException && it.responseCode == 403 } -> "refused"
            else -> return false
        }
        fun decline(why: String): Boolean {
            report("[live-recovery] event=fallback reason=$reason policy=$why")
            return false
        }
        // A second error before READY is a failed recovery, not another hidden retry.
        if (pending != null) return decline("pending-error")
        // These two retries are one-shot for this player session, even after READY.
        // A broken feed or permanently refused URL must reach normal source fallback.
        if (reason == "stuck" && stuckAttempted) return decline("stuck-used")
        if (reason == "refused" && refusedAttempted) return decline("refused-used")
        if (windowStart < 0 || nowMs - windowStart >= 60_000L) {
            windowStart = nowMs
            attempts = 0
        }
        if (attempts >= 2) return decline("rate-limit")
        if (reason == "stuck") stuckAttempted = true
        if (reason == "refused") refusedAttempted = true
        attempts++
        val waitMs = timeoutMs.coerceIn(8_000L, 60_000L)
        val deadline = object : Runnable {
            override fun run() {
                if (pending !== this) return
                pending = null
                if (active()) {
                    // READY may already be visible before its listener callback.
                    // A real timeout must stop the still-preparing player before
                    // reporting failure, or it can resume behind the error UI.
                    if (player.playbackState == Player.STATE_READY) {
                        report("[live-recovery] event=ready reason=$reason at=deadline")
                        status(0L)
                    }
                    else {
                        report("[live-recovery] event=timeout reason=$reason deadlineMs=$waitMs")
                        try { player.stop() } finally { status(0L); failed() }
                    }
                }
            }
        }
        pending = deadline
        pendingReason = reason
        report("[live-recovery] event=start reason=$reason attempt=$attempts deadlineMs=$waitMs")
        try {
            status(waitMs)
            // No setMediaItem, stop, release, play or URL lookup: retain the
            // session, surface, track choices, volume and requested pause state.
            player.seekToDefaultPosition()
            player.prepare()
            if (pending === deadline) schedule(deadline, waitMs)
        } catch (_: Exception) {
            report("[live-recovery] event=fallback reason=$reason policy=prepare-failed")
            cancel()
            return false
        }
        return true
    }

    fun ready(): Boolean {
        val recovering = pending != null
        if (recovering) report("[live-recovery] event=ready reason=$pendingReason")
        cancel()
        return recovering
    }

    fun cancel() {
        val task = pending ?: return
        pending = null
        unschedule(task)
        status(0L)
    }
}
