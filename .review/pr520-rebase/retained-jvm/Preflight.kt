import android.app.Activity
import android.content.Intent
import android.net.Uri
import android.widget.Toast
import space.nuvio.nativelegacy.RegistroDownload
import java.io.ByteArrayOutputStream
import java.io.File
import java.io.IOException
import java.io.OutputStream
import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit

private val root = File(System.getProperty("preflight.data")).apply { mkdirs() }
private var count = 0
private fun test(name: String, block: () -> Unit) {
    Toast.events.clear()
    block()
    count++
    println("PASS $name")
}
private fun file(name: String, text: String) = File(root, name).apply { writeText(text) }
private fun result() = Intent().apply { data = Uri.parse("content://test/chosen-log") }
private fun waitUi(activity: Activity) {
    val end = System.nanoTime() + TimeUnit.SECONDS.toNanos(5)
    while (activity.ui.isEmpty() && System.nanoTime() < end) Thread.sleep(1)
    check(activity.ui.isNotEmpty()) { "Worker did not dispatch completion" }
}
private fun messages(activity: Activity) = Toast.events.filter { it.first === activity }.map { it.second }
private fun newDownload(activity: Activity, f: File): RegistroDownload = RegistroDownload(activity).also {
    it.abrir(f.path, "saved", "failed")
}

fun main() {
    test("SAF action, MIME, openable category and bounded filename") {
        val i = RegistroDownload.documento()
        check(i.action == Intent.ACTION_CREATE_DOCUMENT)
        check(i.type == "text/plain")
        check(i.categories == setOf(Intent.CATEGORY_OPENABLE))
        check(i.extras[Intent.EXTRA_TITLE]!!.matches(Regex("Nuvio-Log-[0-9]{8}-[0-9]{6}\\.txt")))
        check(RegistroDownload.MAX_BYTES == 200 * 1024)
    }
    test("short UTF-8 log is preserved byte-for-byte") {
        val text = "Log sessão 日本語\nsecond line\n"
        check(RegistroDownload.lerFim(file("short.txt", text)) == text)
        check(RegistroDownload.lerFim(file("empty.txt", "")) == "")
    }
    test("exact 200 KiB limit keeps whole first line") {
        val text = "a".repeat(RegistroDownload.MAX_BYTES)
        check(RegistroDownload.lerFim(file("exact.txt", text)) == text)
    }
    test("oversize read discards partial first line and truncated UTF-8") {
        val tail = "safe line\n".repeat(100)
        val bytes = ByteArray(RegistroDownload.MAX_BYTES + 17) { 'x'.code.toByte() }
        // The bounded window starts in the middle of this multibyte character.
        val split = "é".toByteArray()
        bytes[16] = split[0]
        bytes[17] = split[1]
        val ending = ("discarded fragment\n" + tail).toByteArray()
        ending.copyInto(bytes, bytes.size - ending.size)
        val f = File(root, "oversize.txt").apply { writeBytes(bytes) }
        val actual = RegistroDownload.lerFim(f)
        check(actual == tail)
        check(actual.toByteArray().size <= RegistroDownload.MAX_BYTES)
        check('\uFFFD' !in actual)
    }
    test("oversize unbroken secret is entirely discarded") {
        check(RegistroDownload.lerFim(file("no-newline.txt", "SECRET".repeat(40000))) == "")
    }
    test("URL credentials, paths, queries and escaped URLs are removed") {
        val text = "https://alice:url-secret@example.com:8443/path-secret?api_key=query-secret\n" +
            "http:\\/\\/media.example\\/private-secret?token=url-token\n" +
            "ftp://download.example/file-secret\nstremio://install.example/install-secret"
        val actual = RegistroDownload.limpar(text)
        for (secret in listOf("alice", "url-secret", "path-secret", "query-secret", "private-secret", "url-token", "file-secret", "install-secret"))
            check(secret !in actual) { "URL secret survived: $secret" }
        check("https://example.com/[…]" in actual)
        check("http://media.example/[…]" in actual)
        check("ftp://download.example/[…]" in actual)
        check("stremio://install.example/[…]" in actual)
    }
    test("HTTP header credentials are removed, case-insensitively") {
        val text = "Authorization: Bearer auth-secret\nCOOKIE=session=cookie-secret; other=value\n\"set-cookie\": \"set-secret\"\nnormal line"
        val actual = RegistroDownload.limpar(text)
        check("auth-secret" !in actual && "cookie-secret" !in actual && "set-secret" !in actual)
        check(actual.endsWith("normal line"))
    }
    test("quoted and unquoted credential fields are removed") {
        val names = listOf("api_key", "api-key", "apikey", "access_token", "accessToken", "refresh-token", "password", "passwd", "secret", "token", "chave", "senha", "username")
        val text = names.mapIndexed { i, n -> "\"$n\": \"credential-$i with spaces\"" }.joinToString(", ") +
            "\nsenha='single quoted credential'\nAPIKEY=unquoted-credential"
        val actual = RegistroDownload.limpar(text)
        check("credential" !in actual)
    }
    test("standalone Bearer, JWT and magnet links are removed") {
        val text = "Bearer bearer-secret\neyJhbGciOiJIUzI1NiJ9.eyJzdWIiOiIxMjMifQ.signature-secret\nmagnet:?xt=urn:btih:magnet-secret&dn=movie\n"
        val actual = RegistroDownload.limpar(text)
        check("bearer-secret" !in actual && "eyJhbGci" !in actual && "magnet-secret" !in actual)
        check("Bearer […]" in actual && "[magnet]" in actual)
    }
    test("ordinary playback diagnostics are preserved") {
        val text = "decoder prepared in 22 ms\nsubtitles English\nframe 123, network 7.2 Mbps\n"
        check(RegistroDownload.limpar(text) == text)
    }
    test("picker busy guard and unrelated result isolation") {
        val a = Activity()
        val f = file("busy.txt", "hello")
        val d = newDownload(a, f)
        check(a.launches.size == 1 && d.escolhendoArquivo)
        d.abrir(f.path, "again", "again-fail")
        check(a.launches.size == 1)
        check(!d.resultado(12345, Activity.RESULT_OK, result()))
        check(d.escolhendoArquivo)
        check(d.resultado(RegistroDownload.PEDIDO, Activity.RESULT_CANCELED, null))
        check(!d.escolhendoArquivo && messages(a).isEmpty())
        d.abrir(f.path, "saved", "failed")
        check(a.launches.size == 2)
        d.fechar()
    }
    test("cancel, null data and null URI never write or notify") {
        for ((rc, data) in listOf(Activity.RESULT_CANCELED to result(), Activity.RESULT_OK to null, Activity.RESULT_OK to Intent())) {
            val a = Activity()
            val d = newDownload(a, file("cancel.txt", "hello"))
            check(d.resultado(RegistroDownload.PEDIDO, rc, data))
            check(!d.escolhendoArquivo && a.ui.isEmpty() && messages(a).isEmpty())
            d.abrir(File(root, "cancel.txt").path, "saved", "failed")
            check(a.launches.size == 2)
            d.fechar()
        }
        val a = Activity()
        check(RegistroDownload(a).resultado(RegistroDownload.PEDIDO, Activity.RESULT_OK, result()))
        check(a.ui.isEmpty() && messages(a).isEmpty())
    }
    test("background write uses chosen URI, truncation mode and closes stream") {
        val a = Activity()
        val f = file("write.txt", "hello https://example.com/private-token\n")
        val output = object : ByteArrayOutputStream() {
            var closed = false
            override fun close() { closed = true; super.close() }
        }
        a.contentResolver.handler = { uri, mode ->
            check(Thread.currentThread() !== a.uiThread)
            check(Thread.currentThread().name == "nuvio-baixar-registro")
            check(uri.value == "content://test/chosen-log" && mode == "wt")
            output
        }
        val d = newDownload(a, f)
        check(d.resultado(RegistroDownload.PEDIDO, Activity.RESULT_OK, result()))
        check(!d.escolhendoArquivo)
        waitUi(a)
        check(messages(a).isEmpty())
        d.abrir(f.path, "again", "again-fail")
        check(a.launches.size == 1) // Still busy until UI completion is applied.
        a.drain()
        check(output.closed)
        check(output.toString("UTF-8") == "hello https://example.com/[…]\n")
        check(messages(a) == listOf("saved"))
        d.abrir(f.path, "again", "again-fail")
        check(a.launches.size == 2)
        d.fechar()
    }
    test("missing source reports failure and releases busy state") {
        val a = Activity()
        val f = File(root, "does-not-exist.txt").apply { delete() }
        val d = newDownload(a, f)
        d.resultado(RegistroDownload.PEDIDO, Activity.RESULT_OK, result())
        waitUi(a); a.drain()
        check(messages(a) == listOf("failed"))
        d.abrir(f.path, "saved", "failed")
        check(a.launches.size == 2)
        d.fechar()
    }
    test("null resolver output reports failure") {
        val a = Activity()
        a.contentResolver.handler = { _, _ -> null }
        val d = newDownload(a, file("null-output.txt", "hello"))
        d.resultado(RegistroDownload.PEDIDO, Activity.RESULT_OK, result())
        waitUi(a); a.drain()
        check(messages(a) == listOf("failed"))
    }
    test("I/O write failure closes stream and reports failure") {
        val a = Activity()
        var closed = false
        a.contentResolver.handler = { _, _ -> object : OutputStream() {
            override fun write(value: Int) { throw IOException("fixture write failed") }
            override fun close() { closed = true }
        } }
        val d = newDownload(a, file("failure.txt", "hello"))
        d.resultado(RegistroDownload.PEDIDO, Activity.RESULT_OK, result())
        waitUi(a); a.drain()
        check(closed && messages(a) == listOf("failed"))
    }
    test("picker launch failure notifies and allows retry") {
        val a = Activity()
        a.launchFailure = IllegalStateException("picker unavailable")
        val f = file("picker.txt", "hello")
        val d = newDownload(a, f)
        check(!d.escolhendoArquivo && messages(a) == listOf("failed"))
        a.launchFailure = null
        d.abrir(f.path, "saved", "failed")
        check(a.launches.size == 1 && d.escolhendoArquivo)
        d.fechar()
    }
    test("finishing, destroyed and closed activities reject new picker") {
        for (state in 0..2) {
            val a = Activity()
            val d = RegistroDownload(a)
            if (state == 0) a.isFinishing = true
            if (state == 1) a.isDestroyed = true
            if (state == 2) d.fechar()
            d.abrir(File(root, "short.txt").path, "saved", "failed")
            check(a.launches.isEmpty() && !d.escolhendoArquivo && messages(a).isEmpty())
        }
    }
    test("close during picker clears pending result without I/O") {
        val a = Activity()
        val d = newDownload(a, file("close-picker.txt", "hello"))
        d.fechar()
        check(!d.escolhendoArquivo)
        check(d.resultado(RegistroDownload.PEDIDO, Activity.RESULT_OK, result()))
        check(a.ui.isEmpty() && messages(a).isEmpty())
    }
    test("destroy, finish or close during background write suppress completion toast") {
        for (state in 0..2) {
            val a = Activity()
            val entered = CountDownLatch(1)
            val release = CountDownLatch(1)
            a.contentResolver.handler = { _, _ ->
                entered.countDown()
                check(release.await(5, TimeUnit.SECONDS))
                ByteArrayOutputStream()
            }
            val d = newDownload(a, file("destroy-$state.txt", "hello"))
            d.resultado(RegistroDownload.PEDIDO, Activity.RESULT_OK, result())
            check(entered.await(5, TimeUnit.SECONDS))
            if (state == 0) a.isDestroyed = true
            if (state == 1) a.isFinishing = true
            if (state == 2) d.fechar()
            release.countDown()
            waitUi(a); a.drain()
            check(messages(a).isEmpty())
            d.abrir(File(root, "short.txt").path, "saved", "failed")
            check(a.launches.size == 1)
        }
    }
    longSecretPreflight()
    malformedSecretPreflight()
    count += 6
    println("REGISTRO_DOWNLOAD_PREFLIGHT_OK $count")
}
