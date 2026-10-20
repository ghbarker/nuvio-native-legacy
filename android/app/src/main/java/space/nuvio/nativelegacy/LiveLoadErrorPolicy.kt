package space.nuvio.nativelegacy

import androidx.media3.common.C
import androidx.media3.common.PlaybackException
import androidx.media3.common.util.UnstableApi
import androidx.media3.datasource.HttpDataSource
import androidx.media3.exoplayer.upstream.LoadErrorHandlingPolicy
import java.io.EOFException
import java.net.SocketException
import java.net.SocketTimeoutException
import javax.net.ssl.SSLException

/** Keep Media3's sample queues while retrying a temporary live transport failure.
 * VOD, opening failures, auth/format/decoder errors and exhausted parallel downloads
 * retain the supplied policy. No independent probe or second provider connection.
 */
@androidx.annotation.OptIn(UnstableApi::class)
internal class LiveLoadErrorPolicy(
    private val ordinary: LoadErrorHandlingPolicy,
    private val recovery: LiveLoadRecovery,
    private val changed: () -> Unit = {},
) : LoadErrorHandlingPolicy by ordinary {
    override fun getMinimumLoadableRetryCount(dataType: Int): Int =
        if (recovery.ownsRetries()) Int.MAX_VALUE else ordinary.getMinimumLoadableRetryCount(dataType)

    override fun getRetryDelayMsFor(info: LoadErrorHandlingPolicy.LoadErrorInfo): Long {
        if (transient(info.exception)) {
            val response = generateSequence<Throwable>(info.exception) { it.cause }.take(16)
                .filterIsInstance<HttpDataSource.InvalidResponseCodeException>().firstOrNull()
            val header = response?.headerFields?.entries?.firstOrNull { it.key.equals("Retry-After", true) }
                ?.value?.firstOrNull()
            // Unknown/date Retry-After stays with the ordinary policy; never ignore it.
            val seconds = header?.toLongOrNull()
            val minimum = if (header == null) 0L else if (seconds != null && seconds in 0..60) seconds * 1000 else 60_000L
            val decision = recovery.retry(info.loadEventInfo.loadTaskId, info.errorCount, minimum)
            if (decision != null) {
                changed()
                return if (decision >= 0) decision else C.TIME_UNSET
            }
        }
        recovery.concluded(info.loadEventInfo.loadTaskId)
        changed()
        // Loader captures the minimum at startLoading. A live load may have
        // captured MAX_VALUE before it failed or the user paused; explicitly
        // retain the ordinary retry budget for every non-owned failure.
        return if (info.errorCount > ordinary.getMinimumLoadableRetryCount(info.mediaLoadData.dataType))
            C.TIME_UNSET else ordinary.getRetryDelayMsFor(info)
    }

    override fun onLoadTaskConcluded(loadTaskId: Long) {
        recovery.concluded(loadTaskId)
        changed()
        ordinary.onLoadTaskConcluded(loadTaskId)
    }

    companion object {
        internal fun transient(error: Throwable): Boolean {
            val causes = generateSequence(error) { it.cause }.take(16).toList()
            if (causes.any { it is SSLException }) return false
            val http = causes.filterIsInstance<HttpDataSource.InvalidResponseCodeException>().firstOrNull()
            if (http != null) return http.responseCode == 408 || http.responseCode == 429 || http.responseCode in 500..599
            // A completed parallel-download cycle owns its existing fallback.
            if (causes.any { it is FalhaDownloadParalelo }) return false
            return causes.any {
                it is SocketTimeoutException || it is SocketException || it is EOFException ||
                    (it is HttpDataSource.HttpDataSourceException && it.reason in listOf(
                        PlaybackException.ERROR_CODE_IO_NETWORK_CONNECTION_FAILED,
                        PlaybackException.ERROR_CODE_IO_NETWORK_CONNECTION_TIMEOUT))
            }
        }
    }
}
