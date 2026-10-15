package space.nuvio.preloadtest

import android.app.Activity
import android.app.AlertDialog
import android.content.Intent
import android.net.Uri
import android.os.SystemClock
import android.view.View
import android.view.ViewGroup
import android.widget.EditText
import android.widget.ScrollView
import android.widget.TextView
import androidx.core.content.FileProvider
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import org.junit.After
import org.junit.Assert.*
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import space.nuvio.nativelegacy.TestLocalLogs
import java.io.File

@RunWith(AndroidJUnit4::class)
class LocalLogsContractTest {
    private val instrumentation = InstrumentationRegistry.getInstrumentation()
    private lateinit var activity: ProbeActivity
    private lateinit var logs: TestLocalLogs
    private lateinit var current: File
    private lateinit var previous: File

    @Before fun setup() {
        activity = instrumentation.startActivitySync(
            Intent(instrumentation.targetContext, ProbeActivity::class.java).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)) as ProbeActivity
        logs = TestLocalLogs(activity)
        val folder = File(activity.filesDir, "dados").apply { mkdirs() }
        current = File(folder, "nuvio.log")
        previous = File(folder, "nuvio-anterior.log")
    }

    @After fun finish() {
        current.delete(); previous.delete()
        File(activity.cacheDir, TestLocalLogs.SHARE_FOLDER).deleteRecursively()
        instrumentation.runOnMainSync { logs.close(); activity.finish() }
        instrumentation.waitForIdleSync()
    }

    @Test fun localLogRedactionRemovesProviderCredentialsAndKeepsNextState() {
        val sample = "https://user:password@example.test/key/secret?apikey=unsafe\n" +
            "{\"apiKey\":\"my-key\",\"password\":\"my-password\"}\nAuthorization: Bearer test-access\n" +
            "{\"Cookie\":\"session=private-cookie\"}\n{\"Authorization\":\"Basic private-basic\"}\n" +
            "magnet:?xt=urn:btih:private-hash\n[test-next] event=state current=1:1 next=1:2 real=0 auto=1 scene=0 seek=0 countdown_ms=0\n"
        val result = TestLocalLogs.redact(sample)
        for (secret in listOf("user:password", "/key/secret", "unsafe", "my-key", "my-password", "test-access", "private-cookie", "private-basic", "private-hash"))
            assertFalse("Leaked credential: $secret", result.contains(secret))
        assertTrue(result.contains("example.test"))
        assertTrue(result.contains("current=1:1 next=1:2 real=0 auto=1 scene=0 seek=0 countdown_ms=0"))
    }

    @Test fun localLogTailAndSnapshotPreserveCurrentAndPreviousSessions() {
        current.writeText("x".repeat(TestLocalLogs.MAX_TAIL + 10) + "\n[test-next] current-session\n")
        previous.writeText("previous-session https://provider.test/secret-token/file\n")
        val tail = TestLocalLogs.readTail(current)
        assertTrue(tail.toByteArray().size <= TestLocalLogs.MAX_TAIL)
        assertEquals("[test-next] current-session\n", tail)
        val snapshot = String(logs.snapshot(), Charsets.UTF_8)
        assertTrue(snapshot.contains("previous-session"))
        assertTrue(snapshot.contains("[test-next] current-session"))
        assertFalse(snapshot.contains("secret-token"))
    }

    @Test fun localLogDocumentRequestAndCancelKeepOtherActivityResultsAvailable() {
        val intent = TestLocalLogs.documentIntent("Nuvio-Test-Log.txt")
        assertEquals(Intent.ACTION_CREATE_DOCUMENT, intent.action)
        assertEquals("text/plain", intent.type)
        assertTrue(intent.hasCategory(Intent.CATEGORY_OPENABLE))
        assertEquals("Nuvio-Test-Log.txt", intent.getStringExtra(Intent.EXTRA_TITLE))
        assertFalse(logs.handleResult(123, Activity.RESULT_CANCELED, null))
        assertTrue(logs.handleResult(TestLocalLogs.REQUEST, Activity.RESULT_CANCELED, null))
        assertTrue(logs.handleResult(TestLocalLogs.SHARE_REQUEST, Activity.RESULT_CANCELED, null))
        assertFalse(logs.isChoosing)
    }

    @Test fun localLogMenuOffersViewShareAndSaveAndCanBeCancelled() {
        lateinit var dialog: AlertDialog
        instrumentation.runOnMainSync {
            dialog = logs.open() ?: throw AssertionError("Log menu did not open")
            assertTrue(dialog.isShowing)
            val adapter = dialog.listView.adapter
            assertEquals(listOf("View logs", "Share logs", "Save logs"),
                (0 until adapter.count).map { adapter.getItem(it).toString() })
            assertNull("Opening twice must not stack dialogs", logs.open())
            dialog.getButton(AlertDialog.BUTTON_NEGATIVE).performClick()
        }
        instrumentation.waitForIdleSync()
        assertFalse(dialog.isShowing)
        assertFalse(logs.isChoosing)
    }

    @Test fun localLogViewerIsScrollableSelectableAndReadOnly() {
        current.writeText("[test-next] view-session\napi_key=private-view-key\n")
        previous.writeText("[test-next] previous-view-session\n")
        val bytes = logs.snapshot()
        lateinit var dialog: AlertDialog
        instrumentation.runOnMainSync {
            dialog = TestLocalLogs.viewerDialog(activity, bytes)
            dialog.show()
            fun descendants(view: View): List<View> = listOf(view) +
                if (view is ViewGroup) (0 until view.childCount).flatMap { descendants(view.getChildAt(it)) } else emptyList()
            val views = descendants(dialog.window!!.decorView)
            val content = views.filterIsInstance<TextView>().single { it.text.contains("[test-next] view-session") }
            assertTrue(content.isTextSelectable)
            assertTrue(content.parent is ScrollView)
            assertFalse(content.text.contains("private-view-key"))
            assertTrue(content.text.contains("previous-view-session"))
            assertTrue("Viewer must not expose an editable log", views.none { it is EditText })
            dialog.getButton(AlertDialog.BUTTON_POSITIVE).performClick()
        }
        instrumentation.waitForIdleSync()
        assertFalse(dialog.isShowing)
    }

    @Suppress("DEPRECATION")
    @Test fun localLogShareUsesReadOnlyContentUriForFullRedactedFile() {
        current.writeText(("[test-next] " + "x".repeat(80) + "\n").repeat(1800) +
            "[test-next] share-session\napi_key=private-share-key\n")
        previous.writeText("previous-share-session https://provider.test/private-share-url/file\n")
        val chooser = logs.shareIntent()
        assertEquals(Intent.ACTION_CHOOSER, chooser.action)
        val send = chooser.getParcelableExtra<Intent>(Intent.EXTRA_INTENT)!!
        assertEquals(Intent.ACTION_SEND, send.action)
        assertEquals("text/plain", send.type)
        assertNull(send.`package`)
        assertNull(send.component)
        assertFalse("Full log must not be an oversized Binder text extra", send.hasExtra(Intent.EXTRA_TEXT))
        val uri = send.getParcelableExtra<Uri>(Intent.EXTRA_STREAM)!!
        assertEquals("content", uri.scheme)
        assertEquals("${activity.packageName}.atualizacao", uri.authority)
        for (intent in listOf(send, chooser)) {
            assertEquals(Intent.FLAG_GRANT_READ_URI_PERMISSION, intent.flags and Intent.FLAG_GRANT_READ_URI_PERMISSION)
            assertEquals(0, intent.flags and Intent.FLAG_GRANT_WRITE_URI_PERMISSION)
            assertEquals(uri, intent.clipData!!.getItemAt(0).uri)
        }
        val provider = activity.packageManager.resolveContentProvider(uri.authority!!, 0)!!
        assertFalse(provider.exported)
        assertTrue(provider.grantUriPermissions)
        val bytes = activity.contentResolver.openInputStream(uri)!!.use { it.readBytes() }
        assertTrue("Large snapshot must remain intact", bytes.size > 128 * 1024)
        val text = String(bytes, Charsets.UTF_8)
        assertTrue(text.contains("[test-next] share-session"))
        assertTrue(text.contains("previous-share-session"))
        assertFalse(text.contains("private-share-key"))
        assertFalse(text.contains("private-share-url"))
        assertEquals("text/plain", activity.contentResolver.getType(uri))
        try {
            FileProvider.getUriForFile(activity, uri.authority!!, File(activity.cacheDir, "unrelated-private-file"))
            fail("Provider must not expose unrelated cache files")
        } catch (_: IllegalArgumentException) { }
        assertTrue(logs.handleResult(TestLocalLogs.SHARE_REQUEST, Activity.RESULT_CANCELED, null))
        assertFalse(logs.isChoosing)
    }

    @Test fun localLogResultWritesRedactedFileThroughContentResolver() {
        current.writeText("[test-next] real=0 auto=1 scene=0\napi_key=private-key\n")
        previous.writeText("[test-next] previous-session\n")
        val output = File(activity.filesDir, "exported-test-log.txt").apply { delete() }
        val result = Intent().setData(Uri.fromFile(output))
        assertTrue(logs.handleResult(TestLocalLogs.REQUEST, Activity.RESULT_OK, result))
        val deadline = SystemClock.elapsedRealtime() + 10000
        while ((!output.exists() || !output.readText().contains("real=0 auto=1 scene=0")) && SystemClock.elapsedRealtime() < deadline)
            SystemClock.sleep(20)
        assertTrue("ContentResolver export did not finish", output.exists())
        val text = output.readText()
        assertTrue(text.contains("real=0 auto=1 scene=0"))
        assertTrue(text.contains("previous-session"))
        assertFalse(text.contains("private-key"))
        output.delete()
    }
}
