package space.nuvio.nativelegacy

import android.graphics.Bitmap
import android.os.Handler
import android.os.Looper
import android.os.SystemClock
import android.view.PixelCopy
import androidx.media3.common.C
import androidx.media3.common.MediaItem
import androidx.media3.common.PlaybackException
import androidx.media3.common.Player
import androidx.media3.common.util.UnstableApi
import androidx.media3.datasource.DefaultHttpDataSource
import androidx.media3.exoplayer.DefaultLoadControl
import androidx.media3.exoplayer.ExoPlayer
import androidx.media3.exoplayer.hls.HlsManifest
import androidx.media3.exoplayer.source.DefaultMediaSourceFactory
import androidx.media3.exoplayer.upstream.DefaultLoadErrorHandlingPolicy
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import androidx.test.rule.ActivityTestRule
import org.json.JSONArray
import org.json.JSONObject
import org.junit.Assert.*
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith
import space.nuvio.livebench.BenchActivity
import java.net.InetAddress
import java.net.ServerSocket
import java.net.Socket
import java.util.concurrent.CountDownLatch
import java.util.concurrent.Executors
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicInteger

@RunWith(AndroidJUnit4::class)
@androidx.annotation.OptIn(UnstableApi::class)
class LiveDeliveryBenchTest {
    @get:Rule val activity = ActivityTestRule(BenchActivity::class.java)
    private val instrumentation get() = InstrumentationRegistry.getInstrumentation()
    private val main = Handler(Looper.getMainLooper())
    private fun ui(block: () -> Unit) = instrumentation.runOnMainSync(block)

    /** Finite local synthetic broadcast; no real credentials/provider connections. */
    private class Broadcast : AutoCloseable {
        private val assets = InstrumentationRegistry.getInstrumentation().context.assets
        private val server = ServerSocket(0, 16, InetAddress.getByName("127.0.0.1"))
        private val workers = Executors.newFixedThreadPool(4)
        private val born = SystemClock.elapsedRealtime()
        private val wall = System.currentTimeMillis() - 80_000L
        val requests = AtomicInteger()
        val rejected = AtomicInteger()
        @Volatile var until = 0L
        @Volatile var closed = false
        val url get() = "http://127.0.0.1:${server.localPort}/live.m3u8"
        init {
            Thread {
                while (!closed) try {
                    val socket = server.accept()
                    workers.execute { socket.use { serve(it) } }
                } catch (_: Exception) { if (!closed) throw IllegalStateException("accept") }
            }.apply { isDaemon = true; start() }
        }
        private fun serve(socket: Socket) {
            try {
                socket.soTimeout = 3000
                val reader = socket.getInputStream().bufferedReader()
                val path = reader.readLine()?.split(' ')?.getOrNull(1) ?: return
                while (!reader.readLine().isNullOrEmpty()) { }
                requests.incrementAndGet()
                val failed = SystemClock.elapsedRealtime() < until
                val body = if (failed) ByteArray(0) else if (path == "/live.m3u8") {
                    val end = (40 + (SystemClock.elapsedRealtime() - born) / 2000).toInt().coerceAtMost(89)
                    val first = (end - 45).coerceAtLeast(0)
                    val date = java.text.SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss.SSS'Z'", java.util.Locale.US)
                    date.timeZone = java.util.TimeZone.getTimeZone("UTC")
                    buildString {
                        append("#EXTM3U\n#EXT-X-VERSION:3\n#EXT-X-TARGETDURATION:2\n#EXT-X-MEDIA-SEQUENCE:$first\n")
                        append("#EXT-X-PROGRAM-DATE-TIME:${date.format(java.util.Date(wall + first * 2000L))}\n")
                        for (i in first until end) append("#EXTINF:2.000,\nseg%03d.ts\n".format(i))
                    }.toByteArray()
                } else if (path.matches(Regex("/seg[0-9]{3}\\.ts"))) assets.open("hls/${path.drop(1)}").use { it.readBytes() }
                else ByteArray(0)
                if (failed) rejected.incrementAndGet()
                val status = if (failed) "503 Service Unavailable" else "200 OK"
                val type = if (path.endsWith("m3u8")) "application/vnd.apple.mpegurl" else "video/mp2t"
                socket.getOutputStream().apply {
                    write("HTTP/1.1 $status\r\nContent-Type: $type\r\nContent-Length: ${body.size}\r\nConnection: close\r\n\r\n".toByteArray())
                    write(body); flush()
                }
            } catch (_: Exception) { /* Player cancellation closes its socket. */ }
        }
        override fun close() { closed = true; server.close(); workers.shutdownNow() }
    }

    private fun runCase(coordinated: Boolean, outageMs: Long): JSONObject = Broadcast().use { source ->
        lateinit var player: ExoPlayer
        var started = false
        var firstFrame = false
        var errors = 0
        var buffering = 0
        var lastFrameNs = 0L
        var maxFrameGapMs = 0L
        var frames = 0
        var ptsBackwards = 0
        var lastPts = -1L
        val watch = java.util.concurrent.atomic.AtomicBoolean()
        var blackSamples = 0
        var pixelSamples = 0
        var previousPixels = 0
        var pictureChangedAt = 0L
        var maxUnchangedImageMs = 0L
        val recovery = LiveLoadRecovery { SystemClock.elapsedRealtime() }
        val ordinary = DefaultLoadErrorHandlingPolicy()
        ui {
            val policy = if (coordinated) LiveLoadErrorPolicy(ordinary, recovery) else ordinary
            val http = DefaultHttpDataSource.Factory().setConnectTimeoutMs(1500).setReadTimeoutMs(1500)
            val factory = DefaultMediaSourceFactory(activity.activity)
                .setDataSourceFactory(LivePlaylistDataSource.Factory(http, http))
                .setLoadErrorHandlingPolicy(policy)
            player = ExoPlayer.Builder(activity.activity).setMediaSourceFactory(factory)
                .setLoadControl(DefaultLoadControl.Builder().setBufferDurationsMs(20_000, 30_000, 1500, 1500).build())
                .build()
            player.setVideoSurfaceView(activity.activity.video)
            player.setVideoFrameMetadataListener { pts, releaseNs, _, _ ->
                synchronized(recovery) {
                    if (watch.get()) {
                        if (lastFrameNs != 0L) maxFrameGapMs = maxOf(maxFrameGapMs, (releaseNs - lastFrameNs) / 1_000_000)
                        if (lastPts >= 0 && pts < lastPts) ptsBackwards++
                        lastFrameNs = releaseNs; lastPts = pts; frames++
                    }
                }
            }
            player.addListener(object : Player.Listener {
                override fun onRenderedFirstFrame() { firstFrame = true }
                override fun onPlaybackStateChanged(state: Int) {
                    if (state == Player.STATE_READY) started = true
                    if (watch.get() && state == Player.STATE_BUFFERING) buffering++
                }
                override fun onPlayerError(error: PlaybackException) { errors++ }
            })
            val live = MediaItem.LiveConfiguration.Builder().setTargetOffsetMs(22_000)
                .setMinPlaybackSpeed(1f).setMaxPlaybackSpeed(1f).build()
            player.setMediaItem(MediaItem.Builder().setUri(source.url).setLiveConfiguration(live).build())
            player.prepare(); player.play()
        }
        fun sample(): Long {
            var reserve = 0L
            ui {
                reserve = player.totalBufferedDuration
                val manifest = player.currentManifest as? HlsManifest
                recovery.observe(LiveLoadRecovery.Observation(player.isCurrentMediaItemLive && manifest != null,
                    started, !player.playWhenReady, player.playbackState == Player.STATE_BUFFERING,
                    player.currentPosition, reserve, player.videoDecoderCounters?.renderedOutputBufferCount?.toLong() ?: -1,
                    manifest?.mediaPlaylist?.targetDurationUs?.div(1000)?.toInt() ?: 0))
            }
            return reserve
        }
        try {
            val startDeadline = SystemClock.elapsedRealtime() + 25_000
            var reserve = 0L
            while (SystemClock.elapsedRealtime() < startDeadline) {
                reserve = sample()
                if (firstFrame && reserve >= 15_000) break
                Thread.sleep(100)
            }
            assertTrue("fixture must render and build a measured reserve, got $reserve", firstFrame && reserve >= 15_000)
            watch.set(true)
            val before = source.requests.get()
            val faultAt = SystemClock.elapsedRealtime()
            source.until = faultAt + outageMs
            val end = faultAt + outageMs + 8_000
            while (SystemClock.elapsedRealtime() < end) {
                sample()
                val bitmap = Bitmap.createBitmap(64, 36, Bitmap.Config.ARGB_8888)
                val done = CountDownLatch(1)
                ui {
                    PixelCopy.request(activity.activity.video, bitmap, { result ->
                        if (result == PixelCopy.SUCCESS) {
                            pixelSamples++
                            val colors = IntArray(64 * 36)
                            bitmap.getPixels(colors, 0, 64, 0, 0, 64, 36)
                            if (colors.count { ((it shr 16) and 255) < 8 && ((it shr 8) and 255) < 8 && (it and 255) < 8 } > colors.size * 0.98) blackSamples++
                            val hash = colors.contentHashCode()
                            val now = SystemClock.elapsedRealtime()
                            if (pixelSamples == 1 || hash != previousPixels) pictureChangedAt = now
                            else maxUnchangedImageMs = maxOf(maxUnchangedImageMs, now - pictureChangedAt)
                            previousPixels = hash
                        }
                        done.countDown()
                    }, main)
                }
                assertTrue("PixelCopy callback", done.await(2, TimeUnit.SECONDS))
                bitmap.recycle()
                Thread.sleep(200)
            }
            val report = recovery.report()
            synchronized(recovery) { JSONObject().put("coordinated", coordinated).put("outageMs", outageMs).put("initialBufferMs", reserve)
                .put("errors", errors).put("rebuffers", buffering).put("renderedFrames", frames)
                .put("maxFrameGapMs", maxFrameGapMs).put("backwardPts", ptsBackwards)
                .put("pixelSamples", pixelSamples).put("blackSamples", blackSamples)
                .put("requestsDuringCase", source.requests.get() - before).put("http503", source.rejected.get())
                .put("retryDecisions", report.retries).put("maxUnchangedImageMs", maxUnchangedImageMs) }
        } finally {
            ui { watch.set(false); recovery.cancel(); player.release() }
        }
    }

    @Test fun compareOrdinaryAndCoordinatedRecoveryOnIdenticalTimedOutage() {
        val results = JSONArray()
        try {
            for (outage in listOf(9_000L, 25_000L)) {
                results.put(runCase(false, outage))
                val coordinated = runCase(true, outage); results.put(coordinated)
                assertEquals(coordinated.toString(), 0, coordinated.getInt("errors"))
                assertEquals(coordinated.toString(), 0, coordinated.getInt("backwardPts"))
                assertEquals(coordinated.toString(), 0, coordinated.getInt("blackSamples"))
                assertTrue(coordinated.toString(), coordinated.getInt("renderedFrames") > 200)
                if (outage < coordinated.getLong("initialBufferMs")) {
                    assertEquals(coordinated.toString(), 0, coordinated.getInt("rebuffers"))
                    assertTrue(coordinated.toString(), coordinated.getLong("maxFrameGapMs") < 1000)
                    assertTrue(coordinated.toString(), coordinated.getLong("maxUnchangedImageMs") < 1000)
                }
                // A longer outage necessarily exhausts the reserve. It must resume
                // in the same player, not end on a terminal source error.
            }
        } finally {
            instrumentation.targetContext.filesDir.resolve("live-bench-results.json").writeText(results.toString(2))
            android.util.Log.i("LiveBench", results.toString())
        }
    }
}
