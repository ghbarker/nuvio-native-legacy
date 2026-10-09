#!/usr/bin/env python3
"""Exercise Activity's actual orientation/lifecycle methods on JVM doubles.

Android surface rotation and player entry/exit still need emulator/device
validation. This checks the policy, UI-thread queue and SDL callback races.
"""
import os
from pathlib import Path
import re
import subprocess
import tempfile
import xml.etree.ElementTree as ET

root = Path(__file__).resolve().parents[1]
activity = (root / 'android/app/src/main/java/space/nuvio/nativelegacy/NuvioActivity.kt').read_text()


def method(name):
    start = re.search(r'^    (?:private |override )?fun ' + name + r'\(', activity, re.M).start()
    brace = activity.index('{', start)
    end, depth = brace + 1, 1
    while depth:
        depth += (activity[end] == '{') - (activity[end] == '}')
        end += 1
    return activity[start:end]


state = re.search(r'^    @Volatile private var playerTelaCheiaTouch = false$', activity, re.M).group()
methods = '\n'.join(method(name) for name in (
    'aplicarOrientacaoTouch', 'orientarPlayer', 'setOrientationBis',
    'onResume', 'onWindowFocusChanged', 'onPause'))
android = '{http://schemas.android.com/apk/res/android}'
manifest = ET.parse(root / 'android/app/src/main/AndroidManifest.xml')
entry = next(a for a in manifest.findall('application/activity')
             if a.attrib[android + 'name'] == '.NuvioActivity')
assert entry.attrib[android + 'screenOrientation'] == '${nuvioOrientation}'
assert {'orientation', 'screenSize', 'smallestScreenSize'} <= set(
    entry.attrib[android + 'configChanges'].split('|'))
gradle = (root / 'android/app/build.gradle.kts').read_text()
assert 'manifestPlaceholders["nuvioOrientation"] = if (touchPreview) "fullUser" else "landscape"' in gradle
assert 'aplicarOrientacaoTouch()' in method('onCreate')

fixture = r'''
package space.nuvio.nativelegacy
object BuildConfig { var NUVIO_TOUCH_PREVIEW = true }
object ActivityInfo {
    const val SCREEN_ORIENTATION_FULL_USER = 13
    const val SCREEN_ORIENTATION_SENSOR_LANDSCAPE = 6
}
object SystemClock { fun elapsedRealtime() = 1L }
object NvPlayer { var pauses = 0; fun pausarPeloSistema() { pauses++ } }
object Log { fun i(tag: String, text: String) {} }
class Vigia { var front = false; fun frente(value: Boolean) { front = value } }
open class SdlFixture {
    var assignments = 0
    var requestedOrientation = 13
        set(value) { field = value; assignments++ }
    var sdlRequests = mutableListOf<List<Any>>()
    var resumes = 0; var pauses = 0; var focuses = mutableListOf<Boolean>()
    open fun setOrientationBis(w: Int, h: Int, resizable: Boolean, hint: String) {
        sdlRequests.add(listOf(w, h, resizable, hint))
        requestedOrientation = 99
    }
    open fun onResume() { resumes++ }
    open fun onPause() { pauses++ }
    open fun onWindowFocusChanged(hasFocus: Boolean) { focuses.add(hasFocus) }
}
class ActivityFixture : SdlFixture() {
STATE
    var vigia: Vigia? = Vigia()
    var fullscreenCalls = 0
    private val ui = mutableListOf<() -> Unit>()
    fun runOnUiThread(fn: () -> Unit) { ui.add(fn) }
    fun flush() { while (ui.isNotEmpty()) ui.removeAt(0)() }
    fun telaCheiaTouch() { fullscreenCalls++ }
    fun create() = aplicarOrientacaoTouch()
METHODS
}
fun main() {
    val a = ActivityFixture()
    // The launch manifest permits portrait even before SDL creates a window.
    a.create(); check(a.requestedOrientation == 13 && a.assignments == 0)
    a.setOrientationBis(1920, 1080, false, "LandscapeLeft LandscapeRight")
    a.flush(); check(a.requestedOrientation == 13 && a.sdlRequests.isEmpty())
    a.orientarPlayer(true)
    check(a.assignments == 0) // native thread only enqueues the UI mutation
    a.flush(); check(a.requestedOrientation == 6 && a.assignments == 1)
    repeat(10) {
        a.onResume(); a.onWindowFocusChanged(true)
        a.setOrientationBis(1080, 2340, true, "Portrait PortraitUpsideDown")
        a.orientarPlayer(true); a.flush()
    }
    check(a.requestedOrientation == 6 && a.assignments == 1)
    a.onPause(); check(a.requestedOrientation == 6 && a.pauses == 1)
    a.onWindowFocusChanged(false); a.onResume()
    check(a.requestedOrientation == 6 && a.vigia!!.front && NvPlayer.pauses == 1)
    // Exit/minimize release the full-screen flag; retained video cannot relock.
    a.orientarPlayer(false); a.flush()
    check(a.requestedOrientation == 13 && a.assignments == 2)
    a.onResume(); a.setOrientationBis(2340, 1080, false, "LandscapeLeft")
    a.flush(); check(a.requestedOrientation == 13)
    // The latest native state wins even when the UI queue was blocked.
    a.orientarPlayer(true); a.orientarPlayer(false); a.flush()
    check(a.requestedOrientation == 13 && a.assignments == 2)
    a.orientarPlayer(false); a.orientarPlayer(true)
    a.setOrientationBis(1080, 2340, false, "Portrait"); a.flush()
    check(a.requestedOrientation == 6 && a.assignments == 3)
    a.requestedOrientation = 13 // emulate a platform/SDL change before resume
    a.onResume(); check(a.requestedOrientation == 6)
    a.orientarPlayer(false); a.onResume(); a.flush()
    check(a.requestedOrientation == 13)
    // The official TV path still delegates SDL's original arguments unchanged.
    BuildConfig.NUVIO_TOUCH_PREVIEW = false
    val tv = ActivityFixture()
    tv.orientarPlayer(true); tv.create(); tv.onResume(); tv.flush()
    check(tv.requestedOrientation == 13 && tv.assignments == 0)
    tv.setOrientationBis(3840, 2160, false, "LandscapeLeft")
    check(tv.requestedOrientation == 99)
    check(tv.sdlRequests == listOf(listOf(3840, 2160, false, "LandscapeLeft")))
    println("android_orientation: browsing, full-screen, resume/SDL races, queued exit and TV delegation ok")
}
'''.replace('STATE', state).replace('METHODS', methods)

cache = Path.home() / '.gradle/caches/modules-2/files-2.1'


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
with tempfile.TemporaryDirectory(prefix='nuvio-orientation-') as work:
    work = Path(work)
    file = work / 'OrientationTest.kt'; file.write_text(fixture)
    target = work / 'classes'
    subprocess.run([java, '-cp', classpath, 'org.jetbrains.kotlin.cli.jvm.K2JVMCompiler',
                    '-nowarn', '-no-stdlib', '-no-reflect', '-classpath', stdlib,
                    '-jvm-target', '17', '-d', str(target), str(file)], check=True)
    subprocess.run([java, '-cp', os.pathsep.join([str(target), stdlib]),
                    'space.nuvio.nativelegacy.OrientationTestKt'], check=True)
