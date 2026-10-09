#!/usr/bin/env python3
"""Compile and exercise the actual preview viewport geometry on the JVM."""
import os
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = root / 'android/app/src/main/java/space/nuvio/nativelegacy/TouchViewport.kt'
fixture = r'''
package space.nuvio.nativelegacy

fun main() {
    fun expect(w: Int, h: Int, l: Int, t: Int, r: Int, b: Int,
               expected: TouchViewport.Bounds) {
        check(TouchViewport.fit(w, h, l, t, r, b) == expected)
    }
    expect(1920, 1080, 0, 0, 0, 0, TouchViewport.Bounds(0, 0, 1920, 1080))
    expect(2400, 1080, 0, 0, 0, 0, TouchViewport.Bounds(0, 0, 2400, 1080))
    expect(2560, 1600, 0, 0, 0, 0, TouchViewport.Bounds(0, 0, 2560, 1600))
    expect(2340, 1080, 100, 0, 20, 0, TouchViewport.Bounds(100, 0, 2220, 1080))
    expect(2400, 1080, 0, 0, 0, 80, TouchViewport.Bounds(0, 0, 2400, 1000))
    expect(2400, 1080, 80, 24, 0, 24, TouchViewport.Bounds(80, 24, 2320, 1032))
    // Hiding system bars removes their padding; the physical cutout stays protected.
    expect(2400, 1080, 80, 24, 0, 80, TouchViewport.Bounds(80, 24, 2320, 976))
    expect(2400, 1080, 80, 0, 0, 0, TouchViewport.Bounds(80, 0, 2320, 1080))
    check(TouchViewport.fit(0, 1080, 0, 0, 0, 0) == null)
    check(TouchViewport.fit(1920, 0, 0, 0, 0, 0) == null)
    check(TouchViewport.fit(100, 100, 50, 0, 50, 0) == null)
    check(TouchViewport.fit(100, 100, 0, 70, 0, 30) == null)
    for (w in listOf(801, 1921, 2341, 2561)) {
        for (h in listOf(601, 1081, 1601)) {
            val v = TouchViewport.fit(w, h, 31, 7, 19, 23)!!
            check(v.left == 31 && v.top == 7)
            check(v.left + v.width == w - 19 && v.top + v.height == h - 23)
        }
    }
    println("android_touch_viewport: full safe phone/tablet bounds, cutout, shown/hidden bars and odd sizes ok")
}
'''
cache = Path.home() / '.gradle/caches/modules-2/files-2.1'

# Exercise Activity's actual window policy and onMeasure implementation.
# Insets may protect the Android text field, but must not shrink the canvas.
activity = (source.parent / 'NuvioActivity.kt').read_text()
def activity_method(name):
    start = activity.index(f'    private fun {name}(')
    brace = activity.index('{', start)
    end, depth = brace + 1, 1
    while depth:
        depth += (activity[end] == '{') - (activity[end] == '}')
        end += 1
    return activity[start:end]

window_fixture = r'''
package space.nuvio.nativelegacy
object BuildConfig { var NUVIO_TOUCH_PREVIEW = true }
object Build { object VERSION { var SDK_INT = 36 } }
object Gravity { const val TOP = 48; const val LEFT = 3 }
object Color { const val BLACK = 0 }
object Log { fun i(tag: String, text: String) {} }
open class View {
    var parent: Any? = null
    var layoutParams: Any? = null
    object MeasureSpec { fun getSize(value: Int) = value }
}
open class ViewGroup : View() {
    open class LayoutParams(var width: Int, var height: Int) {
        companion object { const val MATCH_PARENT = -1 }
    }
    fun removeView(v: View) { v.parent = null }
    fun addView(v: View, lp: LayoutParams) { v.parent = this; v.layoutParams = lp }
}
open class FrameLayout(context: Any?) : ViewGroup() {
    class LayoutParams(w: Int, h: Int) : ViewGroup.LayoutParams(w, h) {
        var gravity = 0; var leftMargin = 0; var topMargin = 0
    }
    open fun onMeasure(w: Int, h: Int) {}
    fun setBackgroundColor(color: Int) {}
}
object WindowManager { object LayoutParams {
    const val FLAG_FULLSCREEN = 1024
    const val LAYOUT_IN_DISPLAY_CUTOUT_MODE_ALWAYS = 3
    const val LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES = 1
    const val SOFT_INPUT_ADJUST_NOTHING = 48
} }
class Attributes { var layoutInDisplayCutoutMode = 0 }
class Window {
    val decorView = View(); var flags = 0; var softInput = 0
    var assignments = 0
    var attributes = Attributes()
        set(value) { field = value; assignments++ }
    fun addFlags(value: Int) { flags = flags or value }
    fun setSoftInputMode(value: Int) { softInput = value }
}
class WindowInsetsControllerCompat {
    companion object { const val BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE = 2 }
    var systemBarsBehavior = 0; var hidden = 0
    fun hide(mask: Int) { hidden = hidden or mask }
}
object WindowCompat {
    var decorFits = true; val controller = WindowInsetsControllerCompat()
    fun setDecorFitsSystemWindows(w: Window, fit: Boolean) { decorFits = fit }
    fun getInsetsController(w: Window, v: View) = controller
}
data class Insets(val left: Int, val top: Int, val right: Int)
class WindowInsetsCompat(val bars: Insets, val cutout: Insets) {
    object Type { fun systemBars() = 1; fun displayCutout() = 2 }
    fun getInsets(mask: Int) = bars
    fun getInsetsIgnoringVisibility(mask: Int) = cutout
}
object ViewCompat {
    var listener: ((View, WindowInsetsCompat) -> WindowInsetsCompat)? = null
    fun setOnApplyWindowInsetsListener(v: View, fn: (View, WindowInsetsCompat) -> WindowInsetsCompat) { listener = fn }
    fun requestApplyInsets(v: View) {}
}
class ActivityFixture {
    val window = Window(); val mLayout = ViewGroup()
    var root: FrameLayout? = null
    var touchInsetLeft = 0; var touchInsetTop = 0; var touchInsetRight = 0
    var fieldPositions = 0
    fun posicionarCampoTouch() { fieldPositions++ }
    fun setContentView(v: FrameLayout) { root = v }
    fun prepare() = prepararViewportTouch()
    fun fullscreen() = telaCheiaTouch()
METHODS
}
fun main() {
    for (sdk in listOf(24, 28, 29, 30, 36)) {
        Build.VERSION.SDK_INT = sdk
        val a = ActivityFixture(); a.prepare()
        check(!WindowCompat.decorFits)
        check(a.window.flags and 1024 != 0 && a.window.softInput == 48)
        check(WindowCompat.controller.hidden == 1 && WindowCompat.controller.systemBarsBehavior == 2)
        check(a.window.attributes.layoutInDisplayCutoutMode == if (sdk >= 30) 3 else if (sdk >= 28) 1 else 0)
        val assignments = a.window.assignments
        repeat(10) { a.fullscreen() }
        check(a.window.assignments == assignments)
        for ((w, h) in listOf(2340 to 1080, 2560 to 1600, 2400 to 1080)) {
            for (bars in listOf(Insets(0, 0, 0), Insets(0, 24, 80), Insets(80, 24, 0))) {
                ViewCompat.listener!!(a.root!!, WindowInsetsCompat(bars, Insets(96, 0, 0)))
                a.root!!.onMeasure(w, h)
                val lp = a.mLayout.layoutParams as FrameLayout.LayoutParams
                check(lp.width == w && lp.height == h && lp.leftMargin == 0 && lp.topMargin == 0)
                check(a.touchInsetLeft == 96 && a.touchInsetRight == bars.right)
            }
        }
        check(a.fieldPositions == 9)
    }
    BuildConfig.NUVIO_TOUCH_PREVIEW = false
    val tv = ActivityFixture(); tv.fullscreen()
    check(tv.window.flags == 0 && tv.window.assignments == 0)
    println("android_touch_fullscreen: actual Activity launch/resize policy, cutout and bars PASS")
}
'''.replace('METHODS', '\n'.join(activity_method(n) for n in ('telaCheiaTouch', 'prepararViewportTouch')))


def jar(group, artifact, version):
    matches = list((cache / group / artifact / version).glob('*/*.jar'))
    if not matches:
        raise SystemExit(f'Missing cached {artifact}:{version}; run Android Gradle build first')
    return str(matches[0])


compiler_version = os.environ.get('NUVIO_TEST_KOTLIN_VERSION', '2.0.21')
stdlib = jar('org.jetbrains.kotlin', 'kotlin-stdlib', compiler_version)
classpath = os.pathsep.join([
    jar('org.jetbrains.kotlin', 'kotlin-compiler-embeddable', compiler_version), stdlib,
    jar('org.jetbrains.kotlin', 'kotlin-script-runtime', compiler_version),
    jar('org.jetbrains.kotlin', 'kotlin-reflect', '1.6.10'),
    jar('org.jetbrains.intellij.deps', 'trove4j', '1.0.20200330'),
    jar('org.jetbrains.kotlinx', 'kotlinx-coroutines-core-jvm', '1.6.4'),
    jar('org.jetbrains', 'annotations', '13.0'),
])
java_home = os.environ.get('JAVA_HOME')
java = str(Path(java_home) / 'bin/java') if java_home else 'java'
with tempfile.TemporaryDirectory(prefix='nuvio-touch-viewport-') as work:
    work = Path(work)
    file = work / 'TouchViewportTest.kt'
    file.write_text(fixture)
    target = work / 'classes'
    subprocess.run([java, '-cp', classpath, 'org.jetbrains.kotlin.cli.jvm.K2JVMCompiler',
                    '-no-stdlib', '-no-reflect', '-classpath', stdlib,
                    '-jvm-target', '17', '-d', str(target), str(source), str(file)], check=True)
    subprocess.run([java, '-cp', os.pathsep.join([str(target), stdlib]),
                    'space.nuvio.nativelegacy.TouchViewportTestKt'], check=True)
    file = work / 'ActivityWindowTest.kt'
    file.write_text(window_fixture)
    subprocess.run([java, '-cp', classpath, 'org.jetbrains.kotlin.cli.jvm.K2JVMCompiler',
                    '-nowarn', '-no-stdlib', '-no-reflect', '-classpath', stdlib,
                    '-jvm-target', '17', '-d', str(target), str(source), str(file)], check=True)
    subprocess.run([java, '-cp', os.pathsep.join([str(target), stdlib]),
                    'space.nuvio.nativelegacy.ActivityWindowTestKt'], check=True)
