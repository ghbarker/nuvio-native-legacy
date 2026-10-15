package android.app

import android.content.ContentResolver
import android.content.Intent
import java.util.concurrent.ConcurrentLinkedQueue

open class Activity {
    @Volatile var isFinishing = false
    @Volatile var isDestroyed = false
    val contentResolver = ContentResolver()
    val launches = mutableListOf<Pair<Intent, Int>>()
    val ui = ConcurrentLinkedQueue<Runnable>()
    var launchFailure: Exception? = null
    val uiThread = Thread.currentThread()
    fun startActivityForResult(intent: Intent, code: Int) {
        check(Thread.currentThread() === uiThread) { "Picker must open on the UI thread" }
        launchFailure?.let { throw it }
        launches.add(intent to code)
    }
    fun runOnUiThread(action: Runnable) { ui.add(action) }
    fun drain() {
        check(Thread.currentThread() === uiThread)
        while (true) (ui.poll() ?: return).run()
    }
    companion object {
        const val RESULT_OK = -1
        const val RESULT_CANCELED = 0
    }
}
