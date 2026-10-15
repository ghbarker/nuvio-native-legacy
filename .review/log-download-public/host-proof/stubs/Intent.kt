package android.content

import android.net.Uri
import java.io.OutputStream

class Intent(val action: String? = null) {
    val categories = linkedSetOf<String>()
    val extras = linkedMapOf<String, String>()
    var type: String? = null
    var data: Uri? = null
    fun addCategory(category: String): Intent { categories.add(category); return this }
    fun putExtra(key: String, value: String): Intent { extras[key] = value; return this }
    companion object {
        const val ACTION_CREATE_DOCUMENT = "android.intent.action.CREATE_DOCUMENT"
        const val CATEGORY_OPENABLE = "android.intent.category.OPENABLE"
        const val EXTRA_TITLE = "android.intent.extra.TITLE"
    }
}

open class ContentResolver {
    var handler: (Uri, String) -> OutputStream? = { _, _ -> error("Unexpected resolver access") }
    fun openOutputStream(uri: Uri, mode: String): OutputStream? = handler(uri, mode)
}
