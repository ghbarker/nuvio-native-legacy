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
    expect(2400, 1080, 0, 0, 0, 0, TouchViewport.Bounds(240, 0, 1920, 1080))
    expect(2560, 1600, 0, 0, 0, 0, TouchViewport.Bounds(0, 80, 2560, 1440))
    expect(2340, 1080, 100, 0, 20, 0, TouchViewport.Bounds(250, 0, 1920, 1080))
    expect(2400, 1080, 0, 0, 0, 80, TouchViewport.Bounds(311, 0, 1777, 1000))
    expect(2400, 1080, 80, 24, 0, 24, TouchViewport.Bounds(323, 24, 1834, 1032))
    check(TouchViewport.fit(0, 1080, 0, 0, 0, 0) == null)
    check(TouchViewport.fit(1920, 0, 0, 0, 0, 0) == null)
    check(TouchViewport.fit(100, 100, 50, 0, 50, 0) == null)
    check(TouchViewport.fit(100, 100, 0, 70, 0, 30) == null)
    for (w in listOf(801, 1921, 2341, 2561)) {
        for (h in listOf(601, 1081, 1601)) {
            val v = TouchViewport.fit(w, h, 31, 7, 19, 23)!!
            check(v.left >= 31 && v.top >= 7)
            check(v.left + v.width <= w - 19 && v.top + v.height <= h - 23)
            check(kotlin.math.abs(v.width * 9 - v.height * 16) < 16)
        }
    }
    println("android_touch_viewport: TV, wide phone, tablet, cutout, system bars and odd sizes ok")
}
'''
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
