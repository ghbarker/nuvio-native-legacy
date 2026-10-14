package space.nuvio.nativelegacy

import android.app.Activity

// The source references the real SDL Activity only to restore UI focus after
// a surface change. The isolated Media3 probe deliberately has no SDL process.
open class NuvioActivity : Activity() {
    fun devolverFoco() { window.decorView.requestFocus() }
}
