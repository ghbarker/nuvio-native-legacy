package space.nuvio.preloadtest

import android.content.Intent
import android.os.SystemClock
import android.view.View
import androidx.media3.common.C
import androidx.media3.common.Player
import androidx.media3.common.util.UnstableApi
import androidx.media3.exoplayer.ExoPlayer
import androidx.media3.exoplayer.analytics.AnalyticsListener
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import org.junit.After
import org.junit.Assert.*
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import space.nuvio.nativelegacy.NvPlayer
import java.io.BufferedInputStream
import java.net.ServerSocket
import java.net.Socket
import java.util.concurrent.CopyOnWriteArrayList
import java.util.concurrent.Executors
import java.util.concurrent.atomic.AtomicInteger

@androidx.annotation.OptIn(UnstableApi::class)
@RunWith(AndroidJUnit4::class)
class PreloadContractTest {
    private val instrumentation = InstrumentationRegistry.getInstrumentation()
    private lateinit var activity: ProbeActivity
    private lateinit var server: RangeServer
    private var nextToken = 10

    @Before fun setup() {
        val bytes = instrumentation.context.assets.open("fixture.mp4").use { it.readBytes() }
        val assets = instrumentation.context.assets
        val extra = assets.list("")!!.filter { it.endsWith(".ts") || it.endsWith(".m3u8") }
            .associate { "/$it" to assets.open(it).use { stream -> stream.readBytes() } }
        server = RangeServer(bytes, extra + ("/current.mp4" to bytes))
        activity = instrumentation.startActivitySync(Intent(instrumentation.targetContext, ProbeActivity::class.java)
            .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)) as ProbeActivity
        onMain { NvPlayer.buffer(0); NvPlayer.cache(0); TestProbe.reset() }
    }

    @After fun cleanup() {
        if (::activity.isInitialized) onMain { NvPlayer.encerrar(); activity.finish() }
        instrumentation.waitForIdleSync()
        if (::server.isInitialized) server.close()
    }

    private fun <T> onMain(block: () -> T): T {
        var value: T? = null
        var failure: Throwable? = null
        instrumentation.runOnMainSync { try { value = block() } catch (t: Throwable) { failure = t } }
        failure?.let { throw it }
        @Suppress("UNCHECKED_CAST") return value as T
    }
    private fun await(message: String, timeout: Long = 20_000, condition: () -> Boolean) {
        val deadline = SystemClock.elapsedRealtime() + timeout
        while (SystemClock.elapsedRealtime() < deadline) {
            if (condition()) return
            Thread.sleep(25)
        }
        fail("$message; requests=${server.requests}, natives=${TestProbe.count()}, errors=${TestProbe.errors()}")
    }
    private fun field(instance: Any, name: String): Any? = instance.javaClass.getDeclaredField(name)
        .also { it.isAccessible = true }.get(instance)
    private fun active(): ExoPlayer? = onMain { field(NvPlayer, "player") as ExoPlayer? }
    private fun prepared(): ExoPlayer? = onMain {
        val slot = field(NvPlayer, "preparo") ?: return@onMain null
        val resource = field(slot, "recursos") ?: return@onMain null
        field(resource, "p") as ExoPlayer
    }
    private fun prepare(position: Int = 0, fraction: Int = 0, headers: String = "X-Probe: expected", cache: Int = 0, url: String = server.url): Int {
        val token = nextToken++
        NvPlayer.preparar(url, headers, position, fraction, token, 0, cache)
        await("preparation did not reach READY") { NvPlayer.prepararEstado(token) == 2 }
        return token
    }

    @Test fun pausedNetworkLoadIsAdoptedWithoutResetOrResolverReplay() {
        val token = prepare()
        val p = prepared()!!
        val initialPosition = onMain { p.currentPosition }
        val item = onMain { p.currentMediaItem }
        val period = onMain { p.currentTimeline.getUidOfPeriod(0) }
        val resets = AtomicInteger()
        onMain {
            assertNull(field(NvPlayer, "player"))
            assertFalse(p.playWhenReady)
            assertFalse(p.isPlaying)
            assertEquals(0f, p.volume, 0f)
            assertTrue(p.bufferedPosition > p.currentPosition + 1000L)
            assertEquals(0, TestProbe.count()) // includes audio/cache/tracks/position/events
            for (i in 0 until activity.layer.childCount) assertEquals(View.GONE, activity.layer.getChildAt(i).visibility)
            p.addListener(object : Player.Listener {
                override fun onPlaybackStateChanged(state: Int) { if (state == Player.STATE_IDLE) resets.incrementAndGet() }
            })
        }
        Thread.sleep(300)
        onMain { assertEquals("paused preparation must not advance position", initialPosition, p.currentPosition) }
        assertTrue("HTTP bytes must be fetched before Play", server.sent.get() > 0)
        assertTrue(server.requests.any { it.path == "/media.mp4" && it.probe == "expected" })
        val resolves = server.requests.count { it.path == "/resolve" }
        val startsAtZero = server.requests.count { it.path == "/media.mp4" && it.start == 0L }
        NvPlayer.abrirPreparada(server.url, "X-Probe: expected", 0, 0, 90, token)
        await("adoption did not install the same player") { active() === p }
        await("adopted player did not play") { onMain { p.isPlaying } }
        onMain {
            assertSame(item, p.currentMediaItem)
            assertEquals(period, p.currentTimeline.getUidOfPeriod(0))
            assertTrue(p.currentPosition >= initialPosition)
            assertEquals("adoption must publish READY once", 1, TestProbe.ready())
            assertEquals(0, TestProbe.errors())
        }
        assertEquals(0, NvPlayer.prepararEstado(token))
        assertEquals("prepare/setMediaItem would reset this player to IDLE", 0, resets.get())
        await("normal source did not continue from the unread byte") {
            server.requests.count { it.path == "/media.mp4" && it.start > 0L } > 0
        }
        assertEquals("AIO redirect resolver must not be replayed", resolves, server.requests.count { it.path == "/resolve" })
        assertEquals("initial media bytes must not be fetched twice", startsAtZero,
            server.requests.count { it.path == "/media.mp4" && it.start == 0L })
        NvPlayer.cancelarPreparo(token)
        onMain { assertSame(p, field(NvPlayer, "player")) }
        NvPlayer.parar()
        await("stop did not clear active playback") { active() == null }
    }

    private fun startCurrent(): ExoPlayer {
        NvPlayer.abrirRetomada(server.base + "/current.mp4", "X-Probe: current", 0, 0, 100)
        await("current episode did not start") { active()?.let { p -> onMain { p.isPlaying } } == true && TestProbe.frames() > 0 }
        val p = active()!!
        onMain { TestProbe.reset() }
        return p
    }

    @Test fun nextEpisodeSamplesLoadWhileCurrentPlayerKeepsPlayingAndTransferOnTransition() {
        val current = startCurrent()
        val before = onMain { current.currentPosition }
        val token = nextToken++
        NvPlayer.prepararProximo(server.url, "X-Probe: next", 0, 0, token, 0, 0)
        await("next episode sample queues did not preload") { NvPlayer.prepararEstado(token) == 2 }
        val resource = onMain { field(field(NvPlayer, "preparo")!!, "recursos")!! }
        onMain {
            assertNull("next episode must not create another ExoPlayer", field(resource, "p"))
            assertSame(current, field(NvPlayer, "player"))
            assertTrue(current.isPlaying)
            assertTrue(current.currentPosition >= before)
            assertEquals("next preload must not publish playback/audio/cache/track natives", 0, TestProbe.unexpected())
        }
        val source = onMain { field(resource, "fontePrecarregada")!! }
        val pair = field(source, "preloadingMediaPeriodAndKey") as android.util.Pair<*, *>
        val warmedPeriod = pair.first
        val resolves = server.requests.count { it.path == "/resolve" }
        val zeros = server.requests.count { it.path == "/media.mp4" && it.start == 0L }
        assertTrue(server.requests.any { it.path == "/media.mp4" && it.probe == "next" })
        NvPlayer.pararMantendoPreparo(token)
        await("handoff did not close current playback") { active() == null }
        assertEquals("handoff must preserve prepared next token", 2, NvPlayer.prepararEstado(token))
        assertSame(source, field(resource, "fontePrecarregada"))
        NvPlayer.abrirPreparada(server.url, "X-Probe: next", 0, 0, 101, token)
        await("episode transition did not play") { active()?.let { it !== current && onMain { it.isPlaying } } == true }
        val next = active()!!
        onMain {
            assertSame(resource, field(NvPlayer, "recursosAtuais"))
            assertSame(source, field(resource, "fontePrecarregada"))
            val adopted = field(source, "playingPreloadedMediaPeriodAndId") as android.util.Pair<*, *>
            assertSame("transition must adopt the buffered MediaPeriod/sample queues", warmedPeriod, adopted.first)
        }
        assertEquals(resolves, server.requests.count { it.path == "/resolve" })
        assertEquals(zeros, server.requests.count { it.path == "/media.mp4" && it.start == 0L })
        assertEquals(0, TestProbe.errors())
        NvPlayer.buscar(12_000)
        await("skip/seek failed after next-source adoption") { active() === next && onMain { next.currentPosition >= 12_000L } }
    }

    @Test fun nextEpisodePercentageResumeAndCancellationPreserveCurrentSession() {
        val current = startCurrent()
        val stale = nextToken++
        NvPlayer.prepararProximo(server.url, "X-Probe: next", 0, 0, stale, 0, 0)
        NvPlayer.cancelarPreparo(stale)
        val token = nextToken++
        NvPlayer.prepararProximo(server.url, "X-Probe: next", 0, 5000, token, 0, 0)
        NvPlayer.cancelarPreparo(stale)
        await("next percentage resume did not preload exact range") { NvPlayer.prepararEstado(token) == 2 }
        val slot = onMain { field(NvPlayer, "preparo")!! }
        val resource = onMain { field(slot, "recursos")!! }
        onMain {
            assertSame(current, field(NvPlayer, "player"))
            assertTrue(current.isPlaying)
            assertNull(field(resource, "p"))
            assertTrue(field(slot, "posicaoFonteMs") as Long in 9_900L..10_100L)
            assertEquals(0, TestProbe.unexpected())
        }
        val source = field(resource, "fontePrecarregada")!!
        val warmed = (field(source, "preloadingMediaPeriodAndKey") as android.util.Pair<*, *>).first
        NvPlayer.abrirPreparada(server.url, "X-Probe: next", 0, 5000, 102, token)
        await("resumed next episode did not adopt") { active()?.let { it !== current && onMain { it.playbackState == Player.STATE_READY } } == true }
        onMain {
            val next = field(NvPlayer, "player") as ExoPlayer
            assertTrue(next.currentPosition >= 10_000L)
            assertSame(warmed, (field(source, "playingPreloadedMediaPeriodAndId") as android.util.Pair<*, *>).first)
            assertEquals(102, TestProbe.resumeGeneration())
            assertEquals(1, TestProbe.resumeAccepted())
        }
        assertEquals(0, TestProbe.errors())
    }

    @Test fun hlsPreparationKeepsManifestAndBufferedSegmentsOnAdoption() {
        val url = server.base + "/fixture.m3u8"
        val token = prepare(url = url)
        val p = prepared()!!
        val item = onMain { p.currentMediaItem }
        val period = onMain { p.currentTimeline.getUidOfPeriod(0) }
        val manifests = server.requests.count { it.path == "/fixture.m3u8" }
        val openingSegments = server.requests.count { it.path == "/segment00.ts" }
        assertEquals(1, manifests)
        assertEquals(1, openingSegments)
        assertEquals(0, TestProbe.count())
        onMain { assertFalse(p.playWhenReady); assertTrue(p.bufferedPosition > p.currentPosition + 1000L) }
        NvPlayer.abrirPreparada(url, "X-Probe: expected", 0, 0, 95, token)
        await("HLS did not adopt its existing player") { active() === p }
        await("HLS did not play") { onMain { p.isPlaying } }
        onMain { assertSame(item, p.currentMediaItem); assertEquals(period, p.currentTimeline.getUidOfPeriod(0)) }
        assertEquals(manifests, server.requests.count { it.path == "/fixture.m3u8" })
        assertEquals(openingSegments, server.requests.count { it.path == "/segment00.ts" })
        assertEquals(0, TestProbe.errors())
    }

    @Test fun cancelQueuedRequestsAndStaleTokensStaySilent() {
        val cancelled = nextToken++
        val live = nextToken++
        NvPlayer.preparar(server.url, "", 0, 0, cancelled, 0, 0)
        NvPlayer.cancelarPreparo(cancelled)
        NvPlayer.preparar(server.url, "", 0, 0, live, 0, 0)
        NvPlayer.cancelarPreparo(cancelled)
        await("new token did not supersede queued cancellation") { NvPlayer.prepararEstado(live) == 2 }
        assertEquals(0, NvPlayer.prepararEstado(cancelled))
        val p = prepared()!!
        assertNull(active())
        assertEquals(0, TestProbe.count())
        onMain { NvPlayer.pausarPeloSistema() }
        await("system pause retained hidden preload") { prepared() == null && NvPlayer.prepararEstado(live) == 0 }
        onMain { assertFalse(p.playWhenReady) }
        assertEquals(0, TestProbe.count())
        val failed = nextToken++
        NvPlayer.preparar(server.base + "/missing.mp4", "", 0, 0, failed, 0, 0)
        await("failed preparation should be owned by the app") { NvPlayer.prepararEstado(failed) == -1 }
        assertEquals("preload error must not become a playback/native error", 0, TestProbe.count())
        NvPlayer.cancelarPreparo(0)
    }

    @Test fun resumeMatchReusesPlayerAndMismatchStartsAtCorrectPosition() {
        val token = prepare(position = 8_000)
        val p = prepared()!!
        onMain { assertEquals(8_000L, p.currentPosition); assertFalse(p.playWhenReady) }
        NvPlayer.abrirPreparada(server.url, "X-Probe: expected", 8_000, 0, 91, token)
        await("resume match did not adopt") { active() === p }
        onMain {
            assertTrue(p.currentPosition >= 8_000L)
            assertEquals(91, TestProbe.resumeGeneration())
            assertEquals(1, TestProbe.resumeAccepted())
        }
        NvPlayer.parar()
        await("first playback did not stop") { active() == null }
        // Selection replacement waits for the old release without allocating two players.
        val mismatch = prepare(position = 4_000)
        val old = prepared()!!
        NvPlayer.abrirPreparada(server.url, "X-Probe: expected", 10_000, 0, 92, mismatch)
        await("resume mismatch did not create cold fallback") { active()?.let { it !== old } == true }
        val cold = active()!!
        onMain { assertEquals(10_000L, cold.currentPosition) }
        await("cold resume fallback did not reach READY") { onMain { cold.playbackState == Player.STATE_READY } }
        onMain { assertTrue(cold.currentPosition >= 10_000L); assertEquals(1, TestProbe.resumeAccepted()) }
    }

    @Test fun adoptedPlayerStillSupportsOrdinarySeekAndConfigMismatchFallsBack() {
        val token = prepare(position = 2_000)
        val p = prepared()!!
        NvPlayer.abrirPreparada(server.url, "X-Probe: expected", 2_000, 0, 96, token)
        await("episode preparation did not adopt") { active() === p }
        await("episode did not play") { onMain { p.isPlaying } }
        NvPlayer.buscar(15_000) // same operation used by the existing intro/credits skip
        await("ordinary skip/seek did not retain the adopted player") {
            active() === p && onMain { p.currentPosition >= 15_000L }
        }
        assertEquals(0, TestProbe.errors())
        NvPlayer.parar()
        await("playback did not stop") { active() == null }
        val mismatch = prepare()
        val old = prepared()!!
        NvPlayer.buffer(1)
        NvPlayer.abrirPreparada(server.url, "X-Probe: expected", 0, 0, 97, mismatch)
        await("buffer configuration mismatch did not fall back") { active()?.let { it !== old } == true }
        onMain { assertEquals(25_000, field(NvPlayer, "bufferAlvoMs")) }
        assertEquals(0, NvPlayer.prepararEstado(mismatch))
    }

    @Test fun percentageResumeAndCacheAreTransferredAndHeadersMustMatch() {
        onMain { NvPlayer.cache(256) }
        val token = prepare(fraction = 5000, cache = 256)
        val p = prepared()!!
        val cache = onMain { field(field(field(NvPlayer, "preparo")!!, "recursos")!!, "cache") }
        assertNotNull(cache)
        onMain { assertTrue(p.currentPosition in 9_900L..10_100L) }
        NvPlayer.abrirPreparada(server.url, "X-Probe: expected", 0, 5000, 93, token)
        await("percentage resume did not adopt") { active() === p }
        onMain { assertSame(cache, field(NvPlayer, "cacheMidia")); assertEquals(1, TestProbe.resumeAccepted()) }
        NvPlayer.parar()
        await("playback did not stop") { active() == null }
        val mismatch = prepare(cache = 256)
        val old = prepared()!!
        NvPlayer.abrirPreparada(server.url, "X-Probe: changed", 0, 0, 94, mismatch)
        await("header mismatch adopted untrusted preparation") { active()?.let { it !== old } == true }
        await("fallback did not send the changed headers") { server.requests.any { it.probe == "changed" } }
        assertEquals(0, TestProbe.errors())
    }
}

private data class Request(val path: String, val start: Long, val probe: String?)
private class RangeServer(private val bytes: ByteArray, private val extra: Map<String, ByteArray>) {
    private val socket = ServerSocket(0)
    private val workers = Executors.newCachedThreadPool()
    val requests = CopyOnWriteArrayList<Request>()
    val sent = java.util.concurrent.atomic.AtomicLong()
    val base = "http://127.0.0.1:${socket.localPort}"
    val url = "$base/resolve"
    init {
        workers.execute {
            try { while (!socket.isClosed) { val client = socket.accept(); workers.execute { serve(client) } } }
            catch (_: Exception) {}
        }
    }
    private fun serve(client: Socket) {
        try {
            client.use { s ->
                s.soTimeout = 10_000
                val input = BufferedInputStream(s.getInputStream())
                val header = StringBuilder()
                while (!header.endsWith("\r\n\r\n") && header.length < 16_384) {
                    val n = input.read(); if (n < 0) return
                    header.append(n.toChar())
                }
                val lines = header.toString().split("\r\n")
                val path = lines[0].split(' ').getOrElse(1) { "" }
                val range = lines.firstOrNull { it.startsWith("Range:", true) }?.substringAfter(':')?.trim()
                val auth = lines.firstOrNull { it.startsWith("X-Probe:", true) }?.substringAfter(':')?.trim()
                val start = range?.substringAfter("bytes=")?.substringBefore('-')?.toLongOrNull() ?: 0L
                val requestedEnd = range?.substringAfter('-')?.toLongOrNull()
                requests.add(Request(path, start, auth))
                val output = s.getOutputStream()
                if (path == "/resolve") {
                    output.write("HTTP/1.1 302 Found\r\nLocation: $base/media.mp4\r\nContent-Length: 0\r\nConnection: close\r\n\r\n".toByteArray())
                    output.flush(); return
                }
                val data = if (path == "/media.mp4") bytes else extra[path]
                if (data == null) {
                    output.write("HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n".toByteArray())
                    output.flush(); return
                }
                val end = requestedEnd?.coerceAtMost(data.size - 1L) ?: data.size - 1L
                if (start !in 0 until data.size.toLong() || end < start) return
                val mime = if (path.endsWith(".m3u8")) "application/vnd.apple.mpegurl"
                    else if (path.endsWith(".ts")) "video/mp2t" else "video/mp4"
                val status = if (range != null) "206 Partial Content" else "200 OK"
                output.write(("HTTP/1.1 $status\r\nContent-Type: $mime\r\nAccept-Ranges: bytes\r\n" +
                    "Content-Range: bytes $start-$end/${data.size}\r\nContent-Length: ${end - start + 1}\r\nConnection: close\r\n\r\n").toByteArray())
                var offset = start.toInt()
                while (offset <= end) {
                    val n = minOf(64 * 1024, end.toInt() - offset + 1)
                    output.write(data, offset, n); output.flush(); sent.addAndGet(n.toLong()); offset += n
                }
            }
        } catch (_: Exception) {} // canceled loader closes its connection
    }
    fun close() { socket.close(); workers.shutdownNow() }
}
