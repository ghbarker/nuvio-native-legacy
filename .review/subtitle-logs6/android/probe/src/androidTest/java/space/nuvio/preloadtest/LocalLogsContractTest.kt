package space.nuvio.preloadtest

import android.app.Activity
import android.app.Instrumentation
import android.content.IntentFilter
import android.content.Intent
import android.net.Uri
import android.os.SystemClock
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
        if (this::current.isInitialized) current.delete()
        if (this::previous.isInitialized) previous.delete()
        if (this::logs.isInitialized || this::activity.isInitialized) {
            instrumentation.runOnMainSync {
                if (this::logs.isInitialized) logs.close()
                if (this::activity.isInitialized && !activity.isFinishing) activity.finish()
            }
            instrumentation.waitForIdleSync()
        }
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
        assertFalse(logs.isChoosing)
    }

    @Test fun localDownloadUsesDocumentPickerAndPreventsStackedRequests() {
        // Intercept the picker: no external app, upload worker or network call is started.
        val filter = IntentFilter(Intent.ACTION_CREATE_DOCUMENT).apply {
            addDataType("text/plain")
            addCategory(Intent.CATEGORY_OPENABLE)
        }
        assertTrue("The monitor must match the actual download intent",
            filter.match(activity.contentResolver,
                TestLocalLogs.documentIntent("Nuvio-Test-Log.txt"), false, "NuvioLogTest") >= 0)
        val monitor = instrumentation.addMonitor(filter,
            Instrumentation.ActivityResult(Activity.RESULT_CANCELED, null), true)
        try {
            instrumentation.runOnMainSync {
                logs.open()
                logs.open()
            }
            instrumentation.waitForIdleSync()
            assertEquals("Only the requested document picker should launch", 1, monitor.hits)
            assertTrue(logs.isChoosing)
            assertTrue(logs.handleResult(TestLocalLogs.REQUEST, Activity.RESULT_CANCELED, null))
            assertFalse(logs.isChoosing)
        } finally { instrumentation.removeMonitor(monitor) }
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
