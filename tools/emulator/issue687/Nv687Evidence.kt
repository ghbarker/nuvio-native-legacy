package space.nuvio.nativelegacy

import androidx.media3.common.C
import androidx.media3.common.audio.AudioProcessor
import androidx.media3.exoplayer.ExoPlayer
import org.json.JSONObject
import java.io.File
import java.io.FileOutputStream
import java.nio.ByteBuffer

// Observational test-only tap: duplicates decoded PCM without consuming or
// modifying the buffer which the normal gain processor passes to AudioTrack.
@androidx.annotation.OptIn(androidx.media3.common.util.UnstableApi::class)
object Nv687Evidence {
    private var dir: File? = null
    private var output: FileOutputStream? = null
    @Volatile private var captured = 0L
    private var lastWritten = 0L
    @Volatile var positionMs = 0L
    @Volatile var firstFrame = false
    fun initialize(files: File) { dir = File(files,"dados").also { it.mkdirs() } }
    fun observe(p: ExoPlayer) {
        positionMs = p.currentPosition
        val now = android.os.SystemClock.elapsedRealtime()
        if (now - lastWritten < 400) return
        lastWritten = now
        val audio = ArrayList<String>()
        for (g in p.currentTracks.groups) if (g.type == C.TRACK_TYPE_AUDIO)
            for (i in 0 until g.length) if (g.isTrackSelected(i)) audio.add(g.getTrackFormat(i).language ?: "unknown")
        val j = JSONObject().put("playing",p.isPlaying).put("state",p.playbackState)
            .put("position_ms",p.currentPosition).put("duration_ms",p.duration)
            .put("audio_languages",org.json.JSONArray(audio)).put("audio_mime",p.audioFormat?.sampleMimeType ?: "")
            .put("width",p.videoSize.width).put("height",p.videoSize.height)
            .put("rendered_video_frames",p.videoDecoderCounters?.renderedOutputBufferCount ?: 0)
            .put("rendered_audio_buffers",p.audioDecoderCounters?.renderedOutputBufferCount ?: 0)
            .put("volume",p.volume.toDouble()).put("first_frame",firstFrame).put("pcm_bytes",captured)
        try { File(dir,"issue687-media3.json").writeText(j.toString()) } catch (_: Exception) {}
    }
    @Synchronized fun capture(buffer: ByteBuffer, format: AudioProcessor.AudioFormat) {
        if (positionMs < 18000 || captured >= 8_000_000 || !buffer.hasRemaining()) return
        try {
            if (output == null) {
                File(dir,"issue687-pcm-format.json").writeText(JSONObject().put("sample_rate",format.sampleRate)
                    .put("channels",format.channelCount).put("encoding",format.encoding).toString())
                output = FileOutputStream(File(dir,"issue687-audio.pcm"))
            }
            val data = ByteArray(buffer.remaining());buffer.duplicate().get(data)
            output!!.write(data);captured += data.size
        } catch (_: Exception) {}
    }
}
