package space.nuvio.preloadtest

import android.app.Activity
import android.os.Bundle
import android.widget.FrameLayout
import space.nuvio.nativelegacy.NvPlayer
import space.nuvio.nativelegacy.NuvioActivity

object TestProbe {
    init { System.loadLibrary("preloadprobe") }
    external fun reset()
    external fun count(): Int
    external fun ready(): Int
    external fun errors(): Int
    external fun unexpected(): Int
    external fun frames(): Int
    external fun resumeGeneration(): Int
    external fun resumeAccepted(): Int
}

class ProbeActivity : NuvioActivity() {
    lateinit var layer: FrameLayout
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        TestProbe.reset()
        layer = FrameLayout(this)
        setContentView(layer)
        NvPlayer.iniciar(this, layer)
        TestProbe.reset()
    }
    override fun onDestroy() { NvPlayer.encerrar(); super.onDestroy() }
}
