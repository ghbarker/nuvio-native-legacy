package android.widget

import android.app.Activity

class Toast(private val activity: Activity, private val text: String) {
    fun show() {
        check(Thread.currentThread() === activity.uiThread) { "Toast must run on UI thread" }
        events.add(activity to text)
    }
    companion object {
        const val LENGTH_LONG = 1
        val events = mutableListOf<Pair<Activity, String>>()
        fun makeText(activity: Activity, text: String, duration: Int): Toast {
            check(duration == LENGTH_LONG)
            return Toast(activity, text)
        }
    }
}
