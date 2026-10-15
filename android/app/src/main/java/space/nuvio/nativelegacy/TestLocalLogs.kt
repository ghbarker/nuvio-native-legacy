package space.nuvio.nativelegacy

import android.app.Activity
import android.content.Intent
import android.widget.Toast
import java.io.File
import java.io.RandomAccessFile
import java.net.URI
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

/** Only the isolated test APK enables the Activity entrypoints to this class. */
class TestLocalLogs(private val activity: Activity) {
    private var choosing = false
    val isChoosing: Boolean get() = choosing

    fun close() {
        choosing = false
    }

    /** Adds Download beside the original native Send report and live View log actions. */
    fun open() {
        if (choosing || activity.isFinishing || activity.isDestroyed) return
        launch(documentIntent(logTitle()), REQUEST)
    }

    private fun launch(intent: Intent, request: Int) {
        try {
            choosing = true
            @Suppress("DEPRECATION")
            activity.startActivityForResult(intent, request)
        } catch (_: Exception) {
            choosing = false
            Toast.makeText(activity, "Could not open the selected action", Toast.LENGTH_LONG).show()
        }
    }

    fun handleResult(request: Int, result: Int, data: Intent?): Boolean {
        if (request != REQUEST) return false
        choosing = false
        val uri = data?.data
        if (result != Activity.RESULT_OK || uri == null) return true
        Thread({
            val message = try {
                val bytes = snapshot()
                activity.contentResolver.openOutputStream(uri, "wt")?.use { it.write(bytes) }
                    ?: throw IllegalStateException("No writable file")
                "Log downloaded."
            } catch (_: Exception) {
                "Could not save the log. Try another location."
            }
            activity.runOnUiThread { Toast.makeText(activity, message, Toast.LENGTH_LONG).show() }
        }, "nuvio-test-save-log").start()
        return true
    }

    fun snapshot(): ByteArray {
        val version = try { activity.packageManager.getPackageInfo(activity.packageName, 0).versionName ?: "?" }
                      catch (_: Exception) { "?" }
        val out = StringBuilder("Nuvio TV/Mobile Test — local diagnostic log\nVersion: $version\n")
        for (name in listOf("nuvio-anterior.log", "nuvio.log")) {
            out.append("\n--- ").append(name).append(" ---\n")
            out.append(readTail(File(activity.filesDir, "dados/$name")))
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
        return redact(out.toString()).toByteArray(Charsets.UTF_8)
    }

    companion object {
        const val REQUEST = 49117
        const val MAX_TAIL = 200 * 1024

        private fun logTitle() = "Nuvio-Test-Log-" +
            SimpleDateFormat("yyyyMMdd-HHmmss", Locale.US).format(Date()) + ".txt"

        fun documentIntent(title: String) = Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "text/plain"
            putExtra(Intent.EXTRA_TITLE, title)
        }

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

        fun redact(text: String): String {
            var value = text.replace("\\/", "/")
            value = Regex("(?i)https?://[^\\s<>\\\"']+").replace(value) { match ->
                try {
                    val url = URI(match.value)
                    "${url.scheme}://${url.host ?: "redacted"}/[redacted]"
                } catch (_: Exception) { "[url redacted]" }
            }
            value = Regex("(?i)magnet:\\?[^\\s<>\\\"']+").replace(value, "[magnet redacted]")
            value = Regex("(?im)[\\\"']?(authorization|cookie|set-cookie)[\\\"']?\\s*[:=].*$").replace(value, "[header redacted]")
            value = Regex("(?i)[\\\"']?\\b(api[_-]?key|access[_-]?token|refresh[_-]?token|password|passwd|secret|token|chave|senha)\\b[\\\"']?\\s*[:=]\\s*(?:\\\"[^\\\"]*\\\"|'[^']*'|[^\\s,;}]+)")
                .replace(value, "[credential redacted]")
            value = Regex("(?i)\\bBearer\\s+[^\\s,;]+").replace(value, "Bearer [redacted]")
            return value
        }
    }
}
