package space.nuvio.nativelegacy

import android.app.Activity
import java.io.File
import java.io.RandomAccessFile

/** Private test-only snapshot. The public RegistroDownload owns the sole file chooser. */
class TestLocalLogs(private val activity: Activity) {
    fun snapshot(): ByteArray {
        val version = try { activity.packageManager.getPackageInfo(activity.packageName, 0).versionName ?: "?" }
                      catch (_: Exception) { "?" }
        val out = StringBuilder("Nuvio TV/Mobile Test — local diagnostic log\nVersion: $version\n")
        for (name in listOf("nuvio-anterior.log", "nuvio.log")) {
            out.append("\n--- ").append(name).append(" ---\n")
            val current = File(activity.cacheDir, "logs/$name")
            out.append(readTail(if (current.exists()) current else File(activity.filesDir, "dados/$name")))
        }
        out.append("\n--- current app logcat ---\n")
        var process: java.lang.Process? = null
        try {
            process = ProcessBuilder("logcat", "-d", "--pid=${android.os.Process.myPid()}",
                "-t", "1000", "-v", "threadtime").redirectErrorStream(true).start()
            process.inputStream.reader(Charsets.UTF_8).use { reader ->
                val chars = CharArray(4096)
                var remaining = MAX_TAIL
                while (remaining > 0) {
                    val count = reader.read(chars, 0, minOf(chars.size, remaining))
                    if (count < 0) break
                    out.append(chars, 0, count)
                    remaining -= count
                }
            }
        } catch (_: Exception) { out.append("(logcat unavailable)\n") }
        finally { process?.destroy() }
        return RegistroDownload.limpar(out.toString()).toByteArray(Charsets.UTF_8)
    }

    companion object {
        const val MAX_TAIL = 200 * 1024

        fun readTail(file: File): String = try {
            RandomAccessFile(file, "r").use { source ->
                val start = maxOf(0L, source.length() - MAX_TAIL)
                source.seek(start)
                val data = ByteArray((source.length() - start).coerceAtMost(MAX_TAIL.toLong()).toInt())
                source.readFully(data)
                val text = String(data, Charsets.UTF_8)
                // A partial first line may contain a URL without its scheme.
                if (start == 0L) text else text.substringAfter('\n', "")
            }
        } catch (_: Exception) { "(log unavailable)\n" }
    }
}
