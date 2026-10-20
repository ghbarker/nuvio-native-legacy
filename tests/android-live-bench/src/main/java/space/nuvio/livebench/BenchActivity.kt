package space.nuvio.livebench

import android.app.Activity
import android.os.Bundle
import android.view.SurfaceView
import android.view.WindowManager

class BenchActivity : Activity() {
    lateinit var video: SurfaceView
    override fun onCreate(state: Bundle?) {
        super.onCreate(state)
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        video = SurfaceView(this)
        setContentView(video)
    }
}
