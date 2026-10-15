import android.app.Activity
import android.content.Intent
import android.net.Uri
import android.widget.Toast
import space.nuvio.nativelegacy.BuildConfig
import space.nuvio.nativelegacy.RegistroDownload
import space.nuvio.nativelegacy.TestLocalLogs
import java.io.ByteArrayOutputStream
import java.io.File
import java.util.concurrent.TimeUnit

fun main() {
    BuildConfig.NV_TEST_LOCAL_LOGS = true
    val a = Activity()
    val data = File(a.filesDir, "dados").apply { mkdirs() }
    File(data, "nuvio-anterior.log").writeText("PRIOR_NATIVE_LINE\nhttps://u:private-secret@example.invalid/private-path?token=bad\n")
    File(data, "nuvio.log").writeText("CURRENT_NATIVE_LINE 日本語\napi_key=private-native-secret\n")
    val direct = TestLocalLogs(a).snapshot().toString(Charsets.UTF_8)
    check("2.0.18-test-logs8" in direct && "PRIOR_NATIVE_LINE" in direct && "CURRENT_NATIVE_LINE 日本語" in direct)
    check("APP_LOGCAT_LINE" in direct && "private-secret" !in direct && "private-path" !in direct && "private-native-secret" !in direct && "logcat-secret" !in direct)
    println("PASS private snapshot retains native current/previous and same-app diagnostics with public redaction")
    val args = File(System.getenv("LOGCAT_ARGS")).readText()
    check("--pid=4242" in args && "-d" in args && "-t 1000" in args && "-v threadtime" in args)
    println("PASS logcat is scoped to this application PID and bounded accepted arguments")
    val captureCount = File(System.getenv("LOGCAT_COUNT"))
    val beforeCancel = captureCount.readText().length
    val cancelled = RegistroDownload(a)
    cancelled.abrir(File(data,"nuvio.log").path,"saved","failed")
    check(a.launches.last().second == RegistroDownload.PEDIDO)
    check(cancelled.resultado(RegistroDownload.PEDIDO,Activity.RESULT_CANCELED,null))
    check(captureCount.readText().length == beforeCancel)
    println("PASS cancellation does not collect a private snapshot")
    val d = RegistroDownload(a)
    val bytes = ByteArrayOutputStream()
    a.contentResolver.handler = { _, _ -> bytes }
    d.abrir(File(data,"nuvio.log").path,"saved","failed")
    check(a.launches.size == 2)
    check(d.resultado(49117,Activity.RESULT_OK,Intent().apply{this.data=Uri.parse("content://test/private-log")}))
    val deadline=System.nanoTime()+TimeUnit.SECONDS.toNanos(5)
    while(a.ui.isEmpty() && System.nanoTime()<deadline) Thread.sleep(1)
    check(a.ui.isNotEmpty());a.drain()
    val exported=bytes.toString(Charsets.UTF_8)
    check("PRIOR_NATIVE_LINE" in exported && "CURRENT_NATIVE_LINE 日本語" in exported && "APP_LOGCAT_LINE" in exported)
    check("logcat-secret" !in exported && "private-native-secret" !in exported && !d.escolhendoArquivo)
    check(Toast.events.any { it.first === a && it.second=="saved" })
    println("PASS sole public chooser writes private opt-in snapshot and keeps translated lifecycle completion")
    File(data,"nuvio.log").writeText("CUT_SECRET".repeat(40000)+"\nCURRENT_END\n")
    val bounded=TestLocalLogs(a).snapshot().toString(Charsets.UTF_8)
    check("CUT_SECRET" !in bounded && "CURRENT_END" in bounded)
    println("PASS private snapshot retains accepted bounded tail and discards partial first-line secrets")
    File(data,"nuvio.log").delete();File(data,"nuvio-anterior.log").delete()
    val missing=TestLocalLogs(a).snapshot().toString(Charsets.UTF_8)
    check("(log unavailable)" in missing && "APP_LOGCAT_LINE" in missing)
    println("PASS missing native logs keep usable same-app diagnostic snapshot")
}
