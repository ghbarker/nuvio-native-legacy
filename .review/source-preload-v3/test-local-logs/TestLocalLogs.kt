package space.nuvio.nativelegacy

import android.app.Activity
import android.app.AlertDialog
import android.content.ClipData
import android.content.Context
import android.content.Intent
import android.graphics.Typeface
import android.view.ViewGroup
import android.widget.ScrollView
import android.widget.TextView
import android.widget.Toast
import androidx.core.content.FileProvider
import java.io.File
import java.io.RandomAccessFile
import java.net.URI
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

/** Only the isolated test APK enables the Activity entrypoints to this class. */
class TestLocalLogs(private val activity: Activity) {
    private var choosing = false
    private var menu: AlertDialog? = null
    private var viewer: AlertDialog? = null
    val isChoosing: Boolean get() = choosing

    fun close() {
        choosing = false
        menu?.dismiss()
        viewer?.dismiss()
        menu = null
        viewer = null
    }

    fun open(): AlertDialog? {
        if (choosing || menu?.isShowing == true || viewer?.isShowing == true) return null
        return AlertDialog.Builder(activity)
            .setTitle("Test logs")
            .setItems(arrayOf("View logs", "Share logs", "Save logs")) { _, action ->
                when (action) {
                    0 -> prepare("Could not open the log") {
                        val bytes = snapshot()
                        val show: () -> Unit = { viewer = viewerDialog(activity, bytes).also { it.show() } }
                        show
                    }
                    1 -> prepare("Could not share the log. Try saving it instead.") {
                        val intent = shareIntent()
                        val share: () -> Unit = { launch(intent, SHARE_REQUEST) }
                        share
                    }
                    2 -> launch(documentIntent(logTitle()), REQUEST)
                }
            }
            .setNegativeButton("Cancel", null)
            .create().also { dialog ->
                menu = dialog
                dialog.setOnDismissListener { if (menu === dialog) menu = null }
                dialog.show()
            }
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

    // Build snapshots off the UI thread. All three actions use the same redaction.
    private fun prepare(error: String, work: () -> (() -> Unit)) {
        choosing = true
        Thread({
            val action = try { work() } catch (_: Exception) { null }
            activity.runOnUiThread {
                choosing = false
                if (!activity.isFinishing && !activity.isDestroyed) {
                    if (action != null) action()
                    else Toast.makeText(activity, error, Toast.LENGTH_LONG).show()
                }
            }
        }, "nuvio-test-prepare-log").start()
    }

    /** A short content URI avoids sending the large log through an Intent extra. */
    fun shareIntent(): Intent {
        val folder = File(activity.cacheDir, SHARE_FOLDER).apply {
            if (!mkdirs() && !isDirectory) throw IllegalStateException("No log cache")
        }
        val cutoff = System.currentTimeMillis() - 24 * 60 * 60 * 1000L
        folder.listFiles()?.filter { it.isFile && it.lastModified() < cutoff }?.forEach { it.delete() }
        val file = File.createTempFile(logTitle().removeSuffix(".txt") + "-", ".txt", folder)
        try {
            file.writeBytes(snapshot())
            val uri = FileProvider.getUriForFile(activity, "${activity.packageName}.atualizacao", file)
            val send = Intent(Intent.ACTION_SEND).apply {
                type = "text/plain"
                putExtra(Intent.EXTRA_STREAM, uri)
                clipData = ClipData.newRawUri("Test log", uri)
                addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
            }
            return Intent.createChooser(send, "Share test log").apply {
                clipData = send.clipData
                addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
            }
        } catch (failure: Exception) {
            file.delete()
            throw failure
        }
    }

    fun handleResult(request: Int, result: Int, data: Intent?): Boolean {
        if (request != REQUEST && request != SHARE_REQUEST) return false
        choosing = false
        if (request == SHARE_REQUEST) return true
        val uri = data?.data
        if (result != Activity.RESULT_OK || uri == null) return true
        Thread({
            val message = try {
                val bytes = snapshot()
                activity.contentResolver.openOutputStream(uri, "wt")?.use { it.write(bytes) }
                    ?: throw IllegalStateException("No writable file")
                "Log saved. Attach the file in our chat."
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
        const val SHARE_REQUEST = 49118
        const val MAX_TAIL = 200 * 1024
        const val SHARE_FOLDER = "test-local-logs"

        private fun logTitle() = "Nuvio-Test-Log-" +
            SimpleDateFormat("yyyyMMdd-HHmmss", Locale.US).format(Date()) + ".txt"

        fun viewerDialog(context: Context, bytes: ByteArray): AlertDialog {
            val content = TextView(context).apply {
                text = String(bytes, Charsets.UTF_8)
                textSize = 12f
                typeface = Typeface.MONOSPACE
                setTextIsSelectable(true)
                val padding = (16 * resources.displayMetrics.density).toInt()
                setPadding(padding, padding, padding, padding)
            }
            val scroll = ScrollView(context).apply {
                addView(content, ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT))
                layoutParams = ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                    (resources.displayMetrics.heightPixels * 0.65f).toInt())
            }
            return AlertDialog.Builder(context).setTitle("Test logs")
                .setView(scroll).setPositiveButton("Close", null).create()
        }

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
