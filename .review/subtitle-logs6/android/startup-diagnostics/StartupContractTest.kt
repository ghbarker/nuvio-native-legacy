package space.nuvio.preloadtest

import android.content.Intent
import android.os.Handler
import android.os.SystemClock
import android.util.Log
import androidx.media3.common.C
import androidx.media3.common.Player
import androidx.media3.common.util.UnstableApi
import androidx.media3.exoplayer.ExoPlayer
import androidx.media3.exoplayer.analytics.AnalyticsListener
import androidx.media3.exoplayer.audio.AudioSink
import androidx.media3.exoplayer.audio.DefaultAudioSink
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import org.json.JSONArray
import org.json.JSONObject
import org.junit.After
import org.junit.Assert.*
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import space.nuvio.nativelegacy.NvPlayer
import java.net.ServerSocket
import java.net.Socket
import java.util.concurrent.CopyOnWriteArrayList
import java.util.concurrent.CountDownLatch
import java.util.concurrent.Executors
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicInteger
import java.util.concurrent.atomic.AtomicLong

/** Diagnostic baseline: these tests do not promise physical display latency. */
@androidx.annotation.OptIn(UnstableApi::class)
@RunWith(AndroidJUnit4::class)
class StartupContractTest {
    private val instrumentation = InstrumentationRegistry.getInstrumentation()
    private lateinit var activity: ProbeActivity
    private lateinit var server: StartupRangeServer
    private var token = 1000

    @Before fun setup() {
        val assets = instrumentation.context.assets
        val media = assets.list("")!!.filter { it.startsWith("startup-") || it == "fixture.mp4" }
            .associate { "/$it" to assets.open(it).use { input -> input.readBytes() } }
        server = StartupRangeServer(media)
        activity = instrumentation.startActivitySync(Intent(instrumentation.targetContext, ProbeActivity::class.java)
            .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)) as ProbeActivity
        onMain { NvPlayer.buffer(0); NvPlayer.cache(0); TestProbe.reset() }
    }

    @After fun cleanup() {
        if (::server.isInitialized) server.resumeBodies()
        if (::activity.isInitialized) onMain { NvPlayer.encerrar(); activity.finish() }
        instrumentation.waitForIdleSync()
        if (::server.isInitialized) server.close()
    }

    private fun <T> onMain(block: () -> T): T {
        if (android.os.Looper.myLooper() === android.os.Looper.getMainLooper()) return block()
        var result: T? = null
        var failure: Throwable? = null
        instrumentation.runOnMainSync { try { result = block() } catch (t: Throwable) { failure = t } }
        failure?.let { throw it }
        @Suppress("UNCHECKED_CAST") return result as T
    }
    private fun await(message: String, timeout: Long = 25_000, condition: () -> Boolean) {
        val deadline = SystemClock.elapsedRealtime() + timeout
        while (SystemClock.elapsedRealtime() < deadline) {
            if (condition()) return
            Thread.sleep(20)
        }
        fail("$message; requests=${server.requests}; natives=${TestProbe.count()}; errors=${TestProbe.errors()}")
    }
    private fun field(value: Any, name: String): Any? {
        var type: Class<*>? = value.javaClass
        while (type != null) {
            try { return type.getDeclaredField(name).also { it.isAccessible = true }.get(value) }
            catch (_: NoSuchFieldException) { type = type.superclass }
        }
        return null
    }
    private fun active(): ExoPlayer? = onMain { field(NvPlayer, "player") as? ExoPlayer }
    private fun resource(): Any? = onMain { field(NvPlayer, "preparo")?.let { field(it, "recursos") } }
    private fun prepared(): ExoPlayer? = resource()?.let { field(it, "p") as? ExoPlayer }
    private fun selectedLanguage(p: ExoPlayer): String = onMain {
        p.currentTracks.groups.firstOrNull { it.type == C.TRACK_TYPE_AUDIO && it.isSelected }
            ?.let { group -> (0 until group.length).firstOrNull { group.isTrackSelected(it) }
                ?.let { group.getTrackFormat(it).language } } ?: ""
    }
    private fun portuguese(language: String?) = language?.lowercase()?.let { it == "por" || it.startsWith("pt") } == true

    // Reproduce video_android.c's conditional choice: keep an already matching
    // audio, otherwise choose the first supported published matching track.
    private fun choosePortuguese(p: ExoPlayer) {
        if (portuguese(selectedLanguage(p))) return
        val index = onMain {
            @Suppress("UNCHECKED_CAST")
            val published = field(NvPlayer, "audios") as List<Pair<androidx.media3.common.Tracks.Group, Int>>
            published.indexOfFirst { (group, track) -> portuguese(group.getTrackFormat(track).language) }
        }
        assertTrue("Portuguese audio was not published", index >= 0)
        NvPlayer.escolher(0, index)
        await("automatic audio selection did not choose Portuguese") { portuguese(selectedLanguage(p)) }
    }

    private fun warmTitle(url: String, trace: StartupTrace, audio: String = ""): Pair<Int, ExoPlayer> {
        val id = token++
        NvPlayer.preparar(url, "X-Probe: startup", 0, 0, id, 0, 0, audio)
        await("hidden player was not constructed") { prepared() != null }
        val p = prepared()!!
        onMain { trace.attach(p) }
        await("hidden title did not reach sample READY") { NvPlayer.prepararEstado(id) == 2 }
        assertNull(active())
        assertEquals("hidden preparation published playback natives", 0, TestProbe.count())
        onMain { assertFalse(p.isPlaying); assertFalse(p.playWhenReady); assertEquals(0f, p.volume, 0f) }
        return id to p
    }
    private fun startCurrent(): ExoPlayer {
        NvPlayer.abrirRetomada(server.base + "/fixture.mp4", "X-Probe: current", 0, 0, 400)
        await("current episode did not start") { active()?.let { onMain { it.isPlaying } } == true }
        val p = active()!!
        await("current audio/surface startup did not finish") { onMain {
            field(NvPlayer, "estavelEmitido") == true &&
                field(field(NvPlayer, "recursosAtuais")!!, "audioFormato") == 1
        } }
        playback(p) { Unit }
        onMain { TestProbe.reset() }
        return p
    }
    private fun warmNext(url: String, audio: String = ""): Triple<Int, Any, Any> {
        val id = token++
        NvPlayer.prepararProximo(url, "X-Probe: startup", 0, 0, id, 0, 0, audio)
        await("next source did not reach sample READY") { NvPlayer.prepararEstado(id) == 2 }
        val r = resource()!!
        assertNull("next preload created a second player/decoder", field(r, "p"))
        val source = field(r, "fontePrecarregada")!!
        val period = (field(source, "preloadingMediaPeriodAndKey") as android.util.Pair<*, *>).first!!
        return Triple(id, r, period)
    }
    private fun <T> playback(p: ExoPlayer, block: () -> T): T {
        val done = CountDownLatch(1)
        var value: T? = null
        var failure: Throwable? = null
        Handler(p.playbackLooper).post { try { value = block() } catch (t: Throwable) { failure = t }; done.countDown() }
        assertTrue("playback snapshot did not complete", done.await(5, TimeUnit.SECONDS))
        failure?.let { throw it }
        @Suppress("UNCHECKED_CAST") return value as T
    }
    private fun snapshot(p: ExoPlayer): JSONObject {
        val renderers = onMain { (0 until p.rendererCount).map { p.getRenderer(it) } }
        val renderer = playback(p) {
            val video = renderers.first { it.trackType == C.TRACK_TYPE_VIDEO }
            val audio = renderers.first { it.trackType == C.TRACK_TYPE_AUDIO }
            var sink = field(audio, "audioSink")
            while (sink is AudioSink && sink !is DefaultAudioSink) sink = field(sink, "sink")
            val codec = field(video, "codec")
            JSONObject().put("videoCodecInitialized", codec != null)
                .put("videoCodecIdentity", codec?.let { System.identityHashCode(it) } ?: 0)
                .put("heldDecodedOutputIndex", field(video, "outputIndex") ?: -1)
                .put("audioTrackIdentity", sink?.let { field(it, "audioTrack") }?.let { System.identityHashCode(it) } ?: 0)
        }
        return onMain {
            val r = field(NvPlayer, "recursosAtuais")?.takeIf { active() === p } ?: resource()!!
            val load = field(r, "carga") as androidx.media3.exoplayer.DefaultLoadControl
            renderer.put("positionMs", p.currentPosition).put("bufferedDurationMs", p.totalBufferedDuration)
                .put("allocatedBytes", load.allocator.totalBytesAllocated)
                .put("preloadCeilingBytes", field(load, "tetoPreparo"))
                .put("state", p.playbackState).put("audioLanguage", selectedLanguage(p))
        }
    }
    private fun report(name: String, trace: StartupTrace, before: JSONObject, after: JSONObject, extra: JSONObject = JSONObject()) {
        val data = trace.json().put("case", name).put("before", before).put("after", after).put("extra", extra)
            .put("requests", JSONArray(server.requests.map { JSONObject().put("path", it.path).put("start", it.start).put("atMs", it.at) }))
        instrumentation.targetContext.openFileOutput("startup-diagnostics.jsonl", android.content.Context.MODE_APPEND)
            .use { it.write((data.toString() + "\n").toByteArray()) }
        Log.i("StartupProbe", data.toString())
    }

    @Test fun opaqueMultiAudioMkvTitleAutomaticChoiceRetainsPlayerButMeasuresSinkReset() {
        val trace = StartupTrace()
        val url = server.base + "/resolve-mkv"
        val (id, p) = warmTitle(url, trace)
        val before = snapshot(p)
        val item = onMain { p.currentMediaItem }
        val period = onMain { p.currentTimeline.getUidOfPeriod(0) }
        val initialRequests = server.requests.count { it.path == "/opaque" && it.start == 0L }
        val initialResolves = server.requests.count { it.path == "/resolve-mkv" }
        trace.play()
        NvPlayer.abrirPreparada(url, "X-Probe: startup", 0, 0, 401, id)
        await("title did not adopt its prepared player") { active() === p }
        choosePortuguese(p)
        await("chosen title audio did not play") { onMain { p.isPlaying } && portuguese(selectedLanguage(p)) }
        await("late automatic title audio choice did not demonstrate an AudioTrack release") { trace.audioTrackReleaseCount() > 0 }
        assertTrue("title did not render its first frame", trace.firstFrame.await(15, TimeUnit.SECONDS))
        val after = snapshot(p)
        onMain { assertSame(item, p.currentMediaItem); assertEquals(period, p.currentTimeline.getUidOfPeriod(0)) }
        assertEquals("opaque resolver was replayed after completed preparation", initialResolves, server.requests.count { it.path == "/resolve-mkv" })
        assertEquals("opaque media's initial bytes were replayed", initialRequests, server.requests.count { it.path == "/opaque" && it.start == 0L })
        assertEquals(0, TestProbe.errors())
        report("opaque-mkv-title-audio-choice", trace, before, after, JSONObject().put("resolverRequestsBeforePlay", initialResolves))
    }

    @Test fun nextMultiAudioMkvRetainsBufferedPeriodAndMeasuresFirstFrameAndAudioChoice() {
        val current = startCurrent()
        val url = server.base + "/resolve-mkv"
        val (id, r, period) = warmNext(url)
        onMain { assertSame(current, field(NvPlayer, "player")); assertTrue(current.isPlaying) }
        assertEquals("sample preload published current-session natives", 0, TestProbe.unexpected())
        val trace = StartupTrace()
        trace.play()
        NvPlayer.pararMantendoPreparo(id)
        await("old episode did not close") { active() == null }
        NvPlayer.abrirPreparada(url, "X-Probe: startup", 0, 0, 402, id)
        await("next player was not constructed") { active() != null }
        val p = active()!!
        onMain { trace.attach(p) }
        await("next source did not publish initial tracks/READY") { onMain { p.playbackState == Player.STATE_READY } }
        await("default next audio output did not initialize") { snapshot(p).getInt("audioTrackIdentity") != 0 }
        val before = snapshot(p)
        choosePortuguese(p)
        await("chosen next audio did not play") { onMain { p.isPlaying } && portuguese(selectedLanguage(p)) }
        await("late automatic next audio choice did not demonstrate an AudioTrack release") { trace.audioTrackReleaseCount() > 0 }
        assertTrue("next source did not render", trace.firstFrame.await(15, TimeUnit.SECONDS))
        val source = field(r, "fontePrecarregada")!!
        assertSame("next lost the warmed sample period", period, (field(source, "playingPreloadedMediaPeriodAndId") as android.util.Pair<*, *>).first)
        assertEquals(0, TestProbe.errors())
        report("opaque-mkv-next-audio-choice", trace, before, snapshot(p))
    }

    @Test fun nextSeparateAudioHlsMeasuresColdPreferredRenditionAfterWarmDefault() {
        startCurrent()
        val url = server.base + "/startup-master.m3u8"
        val (id, r, period) = warmNext(url)
        val portugueseBefore = server.requests.count { it.path.startsWith("/startup-por") && it.path.endsWith(".ts") }
        assertEquals("default preload unexpectedly requested Portuguese audio segments", 0, portugueseBefore)
        val trace = StartupTrace()
        trace.play()
        NvPlayer.pararMantendoPreparo(id)
        await("old HLS predecessor did not close") { active() == null }
        NvPlayer.abrirPreparada(url, "X-Probe: startup", 0, 0, 403, id)
        await("HLS next player was not constructed") { active() != null }
        val p = active()!!
        onMain { trace.attach(p) }
        await("default HLS audio did not become READY") { onMain { p.playbackState == Player.STATE_READY } }
        await("default HLS audio output did not initialize") { snapshot(p).getInt("audioTrackIdentity") != 0 }
        val before = snapshot(p)
        choosePortuguese(p)
        await("preferred HLS rendition did not request its audio media") {
            server.requests.any { it.path.startsWith("/startup-por") && it.path.endsWith(".ts") }
        }
        await("preferred HLS audio did not start") { onMain { p.isPlaying } && portuguese(selectedLanguage(p)) }
        assertTrue("HLS next did not render", trace.firstFrame.await(15, TimeUnit.SECONDS))
        val source = field(r, "fontePrecarregada")!!
        assertSame(period, (field(source, "playingPreloadedMediaPeriodAndId") as android.util.Pair<*, *>).first)
        assertEquals(0, TestProbe.errors())
        report("separate-audio-hls-next-choice", trace, before, snapshot(p), JSONObject().put("portugueseSegmentsBeforePlay", portugueseBefore))
    }

    @Test fun highBitrateTitleByteReadyMeasuresFirstFrameBeforeAdditionalNetwork() {
        val trace = StartupTrace()
        val url = server.base + "/startup-budget.mp4"
        val (id, p) = warmTitle(url, trace)
        val before = snapshot(p)
        assertTrue("fixture did not exercise byte-limited readiness", before.getInt("allocatedBytes") >= before.getInt("preloadCeilingBytes"))
        assertTrue("byte-limited fixture already held the ordinary startup duration", before.getLong("bufferedDurationMs") < 2500L)
        server.pauseNewBodies()
        trace.play()
        NvPlayer.abrirPreparada(url, "X-Probe: startup", 0, 0, 404, id)
        await("byte-limited title was not adopted") { active() === p }
        // This bounded observation records whether the held frame needs new
        // media; it is not an assertion of universal wall-clock latency.
        val frameWithoutNewBody = trace.firstFrame.await(1500, TimeUnit.MILLISECONDS)
        assertTrue("prepared decoded frame unnecessarily waited for a fresh media body", frameWithoutNewBody)
        await("byte-limited fixture did not rebuffer while fresh media was gated", 7000) { onMain { p.playbackState == Player.STATE_BUFFERING } }
        val duringGate = snapshot(p)
        server.resumeBodies()
        await("byte-limited title did not play after network release") { onMain { p.isPlaying } }
        assertTrue("byte-limited title did not render", trace.firstFrame.await(15, TimeUnit.SECONDS))
        assertEquals(0, TestProbe.errors())
        report("byte-limited-title-first-frame", trace, before, snapshot(p), JSONObject()
            .put("firstFrameBeforeFreshMediaBody", frameWithoutNewBody).put("duringNetworkGate", duringGate))
    }


    @Test fun earlyAudioTitleKeepsMatchingEnglishAndPrimesPortugueseWhileManualOverrideWorks() {
        for (audio in listOf("en", "pt")) {
            val trace = StartupTrace()
            val url = server.base + "/resolve-mkv"
            val (id, p) = warmTitle(url, trace, audio)
            // STATE_READY can precede the asynchronous decoder's first PCM
            // output. Compare actual warmed sinks only after one exists.
            await("silent preferred title did not initialize its audio output", 10_000) {
                snapshot(p).getInt("audioTrackIdentity") != 0
            }
            assertEquals("waiting for the hidden sink published playback natives", 0, TestProbe.count())
            onMain { assertFalse(p.isPlaying); assertFalse(p.playWhenReady); assertEquals(0f, p.volume, 0f) }
            val before = snapshot(p)
            assertTrue("requested language was not selected during silent preparation",
                if (audio == "pt") portuguese(selectedLanguage(p)) else selectedLanguage(p).startsWith("en"))
            val releasesBefore = trace.audioTrackReleaseCount()
            val resolvesBefore = server.requests.count { it.path == "/resolve-mkv" }
            trace.play()
            NvPlayer.abrirPreparada(url, "X-Probe: startup", 0, 0, 410, id, audio)
            await("early-audio title did not adopt its player") { active() === p }
            if (audio == "pt") choosePortuguese(p) // exact C policy leaves the matching selection untouched
            await("early-audio title did not play") { onMain { p.isPlaying } }
            assertTrue("early-audio title did not render", trace.firstFrame.await(15, TimeUnit.SECONDS))
            val after = snapshot(p)
            report("early-title-audio-" + audio, trace, before, after)
            assertEquals("matching automatic choice released the warmed audio output", releasesBefore, trace.audioTrackReleaseCount())
            assertEquals("matching automatic choice replaced the warmed AudioTrack", before.getInt("audioTrackIdentity"), after.getInt("audioTrackIdentity"))
            assertEquals("early-audio adoption repeated the resolver", resolvesBefore, server.requests.count { it.path == "/resolve-mkv" })
            if (audio == "pt") {
                val english = onMain {
                    @Suppress("UNCHECKED_CAST")
                    val tracks = field(NvPlayer, "audios") as List<Pair<androidx.media3.common.Tracks.Group, Int>>
                    tracks.indexOfFirst { (group, track) -> group.getTrackFormat(track).language?.startsWith("en") == true }
                }
                assertTrue(english >= 0)
                val releasesBeforeManual = trace.audioTrackReleaseCount()
                NvPlayer.escolher(0, english)
                await("captured preference prevented a manual English choice") { selectedLanguage(p).startsWith("en") && onMain { p.isPlaying } }
                await("manual language change did not exercise the real AudioTrack release counter") {
                    trace.audioTrackReleaseCount() > releasesBeforeManual
                }
                val position = onMain { p.currentPosition }
                await("manual audio choice did not survive continued playback") { onMain { p.currentPosition >= position + 250L } }
                assertTrue("early preference overrode the manual audio choice", selectedLanguage(p).startsWith("en"))
                report("early-title-manual-english-after-portuguese", trace, before, snapshot(p))
            }
            assertEquals(0, TestProbe.errors())
            NvPlayer.parar()
            await("early-audio title did not close") { active() == null }
            onMain { TestProbe.reset() }
        }
    }

    @Test fun earlyAudioNextHlsLoadsPreferredMediaBeforePlayAndRetainsItsAudioTrack() {
        startCurrent()
        val url = server.base + "/startup-master.m3u8"
        val (id, r, period) = warmNext(url, "pt")
        val openingBefore = server.requests.count { it.path == "/startup-por00.ts" }
        assertTrue("Portuguese audio was not fetched during next preparation", openingBefore > 0)
        val trace = StartupTrace()
        trace.play()
        NvPlayer.pararMantendoPreparo(id)
        await("previous episode did not close for early-audio handoff") { active() == null }
        NvPlayer.abrirPreparada(url, "X-Probe: startup", 0, 0, 411, id, "pt")
        await("preferred HLS player was not constructed") { active() != null }
        val p = active()!!
        onMain { trace.attach(p) }
        await("preferred HLS did not publish prepared audio") { portuguese(selectedLanguage(p)) && onMain { p.playbackState == Player.STATE_READY } }
        val before = snapshot(p)
        val releases = trace.audioTrackReleaseCount()
        choosePortuguese(p)
        await("preferred HLS did not play") { onMain { p.isPlaying } }
        assertTrue("preferred HLS did not render", trace.firstFrame.await(15, TimeUnit.SECONDS))
        assertEquals("matching preferred language released the next AudioTrack", releases, trace.audioTrackReleaseCount())
        assertEquals("preferred opening audio segment was fetched twice", openingBefore, server.requests.count { it.path == "/startup-por00.ts" })
        val source = field(r, "fontePrecarregada")!!
        assertSame(period, (field(source, "playingPreloadedMediaPeriodAndId") as android.util.Pair<*, *>).first)
        assertEquals(0, TestProbe.errors())
        report("early-next-hls-portuguese", trace, before, snapshot(p), JSONObject().put("openingPortugueseRequestsBeforePlay", openingBefore))
    }

    @Test fun audioPreferenceMismatchDiscardsStalePreparedPlayerAndKeepsColdDefaults() {
        val trace = StartupTrace()
        val url = server.base + "/resolve-mkv"
        val (id, stale) = warmTitle(url, trace, "pt")
        assertTrue(portuguese(selectedLanguage(stale)))
        val before = snapshot(stale)
        val coldTrace = StartupTrace()
        coldTrace.play()
        NvPlayer.abrirPreparada(url, "X-Probe: startup", 0, 0, 412, id, "en")
        await("changed audio preference reused stale prepared selection") { active()?.let { it !== stale } == true }
        val cold = active()!!
        onMain { coldTrace.attach(cold) }
        await("cold fallback did not retain the ordinary English default") { selectedLanguage(cold).startsWith("en") && onMain { cold.isPlaying } }
        assertEquals(0, NvPlayer.prepararEstado(id))
        assertEquals(0, TestProbe.errors())
        report("audio-preference-mismatch-cold-default", coldTrace, before, snapshot(cold), JSONObject()
            .put("stalePlayerIdentity", System.identityHashCode(stale)).put("coldPlayerIdentity", System.identityHashCode(cold)))
    }

}

private class StartupTrace {
    val firstFrame = CountDownLatch(1)
    private val events = CopyOnWriteArrayList<JSONObject>()
    private val started = AtomicLong()
    private val firstAt = AtomicLong()
    private val audioInits = AtomicInteger()
    private val audioReleases = AtomicInteger()
    private val videoInits = AtomicInteger()
    private val buffering = AtomicInteger()
    @Volatile private var frameObservation = "none"
    fun play() { started.set(SystemClock.elapsedRealtime()); event("play") }
    fun audioTrackReleaseCount() = audioReleases.get()
    private fun event(name: String, details: String = "") {
        events.add(JSONObject().put("event", name).put("atMs", SystemClock.elapsedRealtime()).put("details", details))
    }
    @androidx.annotation.OptIn(UnstableApi::class)
    fun attach(p: ExoPlayer) {
        p.addListener(object : Player.Listener {
            override fun onPlaybackStateChanged(state: Int) {
                if (state == Player.STATE_BUFFERING && started.get() > 0) buffering.incrementAndGet()
                event("state", "$state buffered=${p.totalBufferedDuration}")
            }
            override fun onIsPlayingChanged(playing: Boolean) { event("isPlaying", "$playing") }
        })
        p.addAnalyticsListener(object : AnalyticsListener {
            override fun onRenderedFirstFrame(t: AnalyticsListener.EventTime, output: Any, renderTimeMs: Long) {
                if (firstAt.compareAndSet(0, SystemClock.elapsedRealtime())) frameObservation = "analytics-callback"
                event("firstFrameReleased", "rendererTime=$renderTimeMs")
                firstFrame.countDown()
            }
            override fun onVideoDecoderInitialized(t: AnalyticsListener.EventTime, name: String, timestamp: Long, duration: Long) {
                videoInits.incrementAndGet(); event("videoDecoderInit", "$name duration=$duration")
            }
            override fun onAudioDecoderInitialized(t: AnalyticsListener.EventTime, name: String, timestamp: Long, duration: Long) {
                event("audioDecoderInit", "$name duration=$duration")
            }
            override fun onAudioDecoderReleased(t: AnalyticsListener.EventTime, name: String) { event("audioDecoderRelease", name) }
            override fun onAudioTrackInitialized(t: AnalyticsListener.EventTime, config: AudioSink.AudioTrackConfig) {
                audioInits.incrementAndGet(); event("audioTrackInit", config.toString())
            }
            override fun onAudioTrackReleased(t: AnalyticsListener.EventTime, config: AudioSink.AudioTrackConfig) {
                audioReleases.incrementAndGet(); event("audioTrackRelease", config.toString())
            }
        })
        // A newly created next player can release its first frame before this
        // external test gets its reference. Record the native polling upper
        // bound explicitly instead of missing the frame or claiming precision.
        if (TestProbe.frames() > 0 && firstAt.compareAndSet(0, SystemClock.elapsedRealtime())) {
            frameObservation = "native-poll-upper-bound"
            event("firstFrameAlreadyObservedViaNativePoll")
            firstFrame.countDown()
        }
    }
    fun json(): JSONObject = JSONObject().put("playAtMs", started.get()).put("firstFrameAtMs", firstAt.get())
        .put("firstFrameObservation", frameObservation)
        .put("playToFirstFrameReleaseMs", if (started.get() > 0 && firstAt.get() > 0) firstAt.get() - started.get() else -1)
        .put("audioTrackInitializations", audioInits.get()).put("audioTrackReleases", audioReleases.get())
        .put("videoDecoderInitializations", videoInits.get()).put("bufferingTransitionsAfterPlay", buffering.get())
        .put("events", JSONArray(events))
}

private data class StartupRequest(val path: String, val start: Long, val at: Long)
private class StartupRangeServer(private val media: Map<String, ByteArray>) {
    private val socket = ServerSocket(0)
    private val workers = Executors.newCachedThreadPool()
    val requests = CopyOnWriteArrayList<StartupRequest>()
    val base = "http://127.0.0.1:${socket.localPort}"
    @Volatile private var bodyGate: CountDownLatch? = null
    fun pauseNewBodies() { bodyGate = CountDownLatch(1) }
    fun resumeBodies() { bodyGate?.countDown(); bodyGate = null }
    init {
        workers.execute {
            try { while (!socket.isClosed) { val client = socket.accept(); workers.execute { serve(client) } } }
            catch (_: Exception) {}
        }
    }
    private fun serve(client: Socket) {
        try { client.use { s ->
            s.soTimeout = 15_000
            val reader = s.getInputStream().bufferedReader()
            val path = reader.readLine()?.split(' ')?.getOrNull(1)?.substringBefore('?') ?: return
            var range = ""
            while (true) { val line = reader.readLine() ?: break; if (line.isEmpty()) break
                if (line.startsWith("Range:", true)) range = line.substringAfter(':').trim() }
            val start = range.substringAfter("bytes=", "0").substringBefore('-').toLongOrNull() ?: 0L
            requests.add(StartupRequest(path, start, SystemClock.elapsedRealtime()))
            val output = s.getOutputStream()
            if (path == "/resolve-mkv") {
                output.write("HTTP/1.1 302 Found\r\nLocation: /opaque?auth=startup-test\r\nContent-Length: 0\r\nConnection: close\r\n\r\n".toByteArray()); output.flush(); return
            }
            val bytes = media[if (path == "/opaque") "/startup-two-audio.mkv" else path] ?: return
            val requestedEnd = range.substringAfter('-', "").toLongOrNull()
            val end = minOf(requestedEnd ?: (bytes.size - 1L), bytes.size - 1L)
            if (start >= bytes.size || end < start) return
            val length = end - start + 1L
            val type = if (path.endsWith(".m3u8")) "application/vnd.apple.mpegurl" else if (path.endsWith(".ts")) "video/mp2t" else "application/octet-stream"
            output.write(("HTTP/1.1 206 Partial Content\r\nContent-Type: $type\r\nAccept-Ranges: bytes\r\n" +
                "Content-Range: bytes $start-$end/${bytes.size}\r\nContent-Length: $length\r\nConnection: close\r\n\r\n").toByteArray())
            output.flush()
            val gate = bodyGate
            if (gate != null && !gate.await(20, TimeUnit.SECONDS)) return
            var at = start.toInt()
            while (at <= end) { val n = minOf(64 * 1024, (end - at + 1).toInt()); output.write(bytes, at, n); at += n }
            output.flush()
        } } catch (_: Exception) {}
    }
    fun close() { resumeBodies(); socket.close(); workers.shutdownNow() }
}
