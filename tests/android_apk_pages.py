#!/usr/bin/env python3
"""Exercise the real APK/ELF checker with program-header and ZIP fixtures."""
import importlib.util
import io
from pathlib import Path
import struct
import tempfile
import unittest
import zipfile

root = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('apk_pages', root / 'tools/android-apk-pages.py')
checker = importlib.util.module_from_spec(spec)
spec.loader.exec_module(checker)


def elf(align=16384, machine=183, address=0, relro=None, second_align=None):
    segments = [(1, 5, 0, address, address, 64, 64, align)]
    if second_align is not None:
        segments.append((1, 4, 0, 0, 0, 64, 64, second_align))
    if relro:
        segments.append((checker.PT_GNU_RELRO, 4, 0, relro[0], 0, 0, relro[1], 1))
    ident = b'\x7fELF' + bytes((2, 1, 1)) + bytes(9)
    return struct.pack('<16sHHIQQQIHHHHHH', ident, 3, machine, 1, 0, 64, 0, 0,
                       64, 56, len(segments), 0, 0, 0) + b''.join(
                           struct.pack('<IIQQQQQQ', *segment) for segment in segments)


class PageChecks(unittest.TestCase):
    def test_aligned_arm64_and_x86_64(self):
        self.assertEqual(checker.check_elf(elf(), 'arm64-v8a'), (1, 16384, []))
        self.assertEqual(checker.check_elf(elf(65536, 62), 'x86_64'), (1, 65536, []))

    def test_rejects_small_or_nonpower_alignment(self):
        for align in (0, 4096, 8192, 24576):
            with self.subTest(align=align):
                self.assertTrue(checker.check_elf(elf(align), 'arm64-v8a')[2])

    def test_rejects_incongruent_offsets(self):
        self.assertTrue(checker.check_elf(elf(address=4096), 'arm64-v8a')[2])

    def test_every_load_segment_is_checked(self):
        count, minimum, errors = checker.check_elf(elf(second_align=4096), 'arm64-v8a')
        self.assertEqual((count, minimum), (2, 4096))
        self.assertEqual(len(errors), 1)
        self.assertIn('LOAD[1]', errors[0])

    def test_relro_end(self):
        self.assertEqual(checker.check_elf(elf(relro=(4096, 12288)), 'arm64-v8a')[2], [])
        self.assertTrue(checker.check_elf(elf(relro=(4096, 4096)), 'arm64-v8a')[2])

    def test_invalid_elf(self):
        for data in (b'', elf()[:90], elf(machine=62), elf()[:64]):
            with self.subTest(size=len(data)):
                with self.assertRaises(ValueError):
                    checker.check_elf(data, 'arm64-v8a')

    def test_compressed_apk_checks_all_64bit_libraries(self):
        with tempfile.TemporaryDirectory() as work:
            apk = Path(work) / 'test.apk'
            with zipfile.ZipFile(apk, 'w', zipfile.ZIP_DEFLATED) as z:
                z.writestr('lib/arm64-v8a/libmain.so', elf())
                z.writestr('lib/x86_64/libSDL2.so', elf(machine=62))
                z.writestr('lib/armeabi-v7a/libmain.so', b'32-bit excluded')
                z.writestr('assets/ignored.so', b'not a packaged JNI library')
            output = io.StringIO()
            self.assertTrue(checker.check_apk(apk, output))
            self.assertIn('2 64-bit libraries, 0 failed, 1 other-ABI', output.getvalue())
            with zipfile.ZipFile(apk, 'a') as z:
                z.writestr('lib/arm64-v8a/libdependency.so', elf(4096))
            self.assertFalse(checker.check_apk(apk, io.StringIO()))


if __name__ == '__main__':
    unittest.main()
