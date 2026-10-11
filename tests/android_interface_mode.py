#!/usr/bin/env python3
"""Compile the production detector/parser on the JVM, without Android mocks."""
import os
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = root / "android/app/src/main/java/space/nuvio/nativelegacy/InterfaceMode.kt"
viewport = source.with_name("TouchViewport.kt")
fixture = r'''
import space.nuvio.nativelegacy.InterfaceMode
import java.io.File

fun main(args: Array<String>) {
    val p = InterfaceMode
    check(p.TV == 0 && p.MOBILE == 1 && p.AUTO == 2)
    for (bits in 0..7) {
        val mobile = p.automaticMobile(bits and 1 != 0, bits and 2 != 0, bits and 4 != 0)
        check(mobile == (bits == 0))
        check(!p.resolve(p.TV, mobile))
        check(p.resolve(p.MOBILE, mobile))
        check(p.resolve(p.AUTO, mobile) == mobile)
        check(p.resolve(-1, mobile) == mobile)
        check(p.resolve(3, mobile) == mobile)
    }
    check(p.preference(emptySequence()) == p.AUTO)
    for (choice in 0..2) {
        check(p.preference(sequenceOf("interfaceModoLocal $choice")) == choice)
        check(p.preference(sequenceOf("  interfaceModoLocal\t$choice  ")) == choice)
        check(p.preference(sequenceOf("interfaceModoLocal $choice", "interfaceModoLocal invalid")) == choice)
        check(p.preference(sequenceOf("interfaceModoLocal $choice", "interfaceModoLocal 3")) == choice)
        check(p.preference(sequenceOf("interfaceModoLocal ${choice}suffix")) == choice)
    }
    check(p.preference(sequenceOf("idioma 1", "interfaceModoLocal", "interfaceModoLocal -1", "interfaceModoLocal invalid")) == p.AUTO)
    check(p.preference(sequenceOf("interfaceModoLocal 0", "interfaceModoLocal 1", "interfaceModoLocal 2")) == p.AUTO)
    check(p.preference(sequenceOf("interfaceModoLocal 2", "interfaceModoLocal 0")) == p.TV)
    check(p.preference(sequenceOf("interfaceModoLocal 0", "interfaceModoLocal 99")) == p.TV)
    check(p.preference(sequenceOf("interfaceModoLocal 1", "interfaceModoLocal 99")) == p.MOBILE)
    check(p.preference(sequenceOf("interfaceModoLocal 999999999999999999999999999999999999")) == p.AUTO)
    check(p.preference(sequenceOf("interfaceModoLocal -999999999999999999999999999999999999")) == p.AUTO)
    check(p.preference(sequenceOf("interfaceModoLocal +2suffix")) == p.AUTO)
    val oversized = "interfaceModoLocal " + "0".repeat(100) + "1"
    check(p.preference(sequenceOf(oversized)) == p.AUTO)
    check(p.preference(sequenceOf("interfaceModoLocal 0", oversized)) == p.TV)
    check(p.preference(sequenceOf(oversized, "interfaceModoLocal 1")) == p.MOBILE)
    val payload94 = "interfaceModoLocal " + "0".repeat(74) + "1"
    val payload95 = "interfaceModoLocal " + "0".repeat(75) + "1"
    check(payload94.toByteArray().size == 94 && payload95.toByteArray().size == 95)
    check(p.preference(sequenceOf(payload94)) == p.MOBILE)
    check(p.preference(sequenceOf(payload95)) == p.AUTO)
    check(p.preference(sequenceOf(payload94 + "\r")) == p.MOBILE)
    for (fullScreen in listOf(false, true)) {
        check(p.orientation(false, fullScreen) == InterfaceMode.Orientation.LANDSCAPE)
    }
    check(p.orientation(true, false) == InterfaceMode.Orientation.FULL_USER)
    check(p.orientation(true, true) == InterfaceMode.Orientation.SENSOR_LANDSCAPE)
    for ((width, height) in listOf(1080 to 2340, 1920 to 1080, 1600 to 1000, 1080 to 1080)) {
        val mobile = p.viewport(true, width, height)!!
        check(mobile.left == 0 && mobile.top == 0 && mobile.width == width && mobile.height == height)
        check(p.viewport(false, width, height) == null) // Restore MATCH_PARENT with zero margins.
        check(p.viewport(true, width, height) == mobile)
    }
    check(p.viewport(true, 0, 1080) == null && p.viewport(true, 1080, -1) == null)
    val file = File(args[0], "ajustes.txt")
    check(p.readPreference(file) == p.AUTO)
    file.writeText("qualidade 1\n")
    check(p.readPreference(file) == p.AUTO)
    for (choice in 0..2) {
        file.writeText("idioma 1\ninterfaceModoLocal $choice\n")
        check(p.readPreference(file) == choice)
    }
    file.writeText("interfaceModoLocal bogus\ninterfaceModoLocal 5\n")
    check(p.readPreference(file) == p.AUTO)
    file.writeText(oversized + "\ninterfaceModoLocal 1\n")
    check(p.readPreference(file) == p.MOBILE)
    file.delete();file.mkdir()
    check(p.readPreference(file) == p.AUTO)
    println("Android interface: all TV signals, Auto, explicit overrides, legacy IDs and missing/corrupt settings PASS")
}
'''
with tempfile.TemporaryDirectory(prefix="nuvio-interface-") as work:
    work = Path(work)
    test = work / "InterfaceModeTest.kt"
    test.write_text(fixture)
    jar = work / "interface.jar"
    subprocess.run([os.environ.get("KOTLINC", "kotlinc"), str(source), str(viewport), str(test),
                    "-include-runtime", "-d", str(jar)], check=True)
    subprocess.run([os.environ.get("JAVA", "java"), "-jar", str(jar), str(work)], check=True)
