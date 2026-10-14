# Startup diagnostics

This isolated review kit uses the player from source commit `65d3da65c6abbbef584efbf2b7f0bb1f63c9e7f1` (`NvPlayer.kt` SHA-256 `26728240f2f069b11b73f43cf1d1f67043a094c034d55914c0f52ceb46e6d7d9`). It preserves the canonical twelve helper files, eight original tests, and fixture manifest `569673adca17f3779713c2096ddcb4d0c55749d483f54d387c5bf054d800cc5d`. Staging verifies every canonical helper and all ten player sources before copying them. `startup-stage.json` records the supplied checkout's Git HEAD and the diagnostic test hash.

Run with Bash, Python 3, FFmpeg/ffprobe with libx264 and AAC encoding, JDK 17, Gradle 8.9, Android SDK 35, NDK `27.2.12479018`, CMake `3.22.1`, and a running API 35 emulator. Keep the destination absent or empty and outside the canonical `android` directory. The scripts use no SDK or network while staging and generating media.

```bash
kit=/workspace/review/source-preload-build/android/startup-diagnostics
product=/path/to/checkout-at-65d3da65c6abbbef584efbf2b7f0bb1f63c9e7f1
work=/tmp/nuvio-startup-probe
bash "$kit/stage.sh" "$product" "$work"
bash "$kit/generate-fixtures.sh" "$work"
gradle -p "$work" --console=plain :probe:connectedDebugAndroidTest \
  -Pandroid.testInstrumentationRunnerArguments.class=space.nuvio.preloadtest.StartupContractTest
```

Both test classes remain compiled. The class filter above runs only `StartupContractTest`; the original `PreloadContractTest` source stays byte-for-byte identical. A direct instrumentation run after installing both debug APKs uses the same filter:

```bash
adb shell am instrument -w -r \
  -e class space.nuvio.preloadtest.StartupContractTest \
  space.nuvio.preloadtest.test/androidx.test.runner.AndroidJUnitRunner
adb exec-out run-as space.nuvio.preloadtest \
  cat files/startup-diagnostics.jsonl > "$work/startup-diagnostics.jsonl"
```

The JSONL file lives in the tested app's private files directory. `adb pull` cannot read that directory directly; `run-as` above exports it. Collect it after the instrumentation command, including failed runs, before uninstalling or clearing the app. Gradle's XML results and HTML report are under `probe/build/outputs/androidTest-results/connected` and `probe/build/reports/androidTests/connected` in the working directory.

The generator first runs the untouched canonical generator to retain its MP4, ordinary HLS, and original HLS budget fixtures. It then adds:

- `startup-two-audio.mkv`: 20-second copied AVC video at approximately 3 Mbps; first/default English AAC at 48 kHz and 440 Hz; alternate Portuguese AAC at 48 kHz and 880 Hz.
- `startup-master.m3u8`: one shared AVC video rendition and separate English/default and Portuguese audio renditions. Master `CODECS` is derived from the actual AVC configuration and declares AAC. Playlists and chunks use the `startup-video`, `startup-eng`, and `startup-por` prefixes.
- `startup-budget.mp4`: four-second, fast-start AVC/AAC MP4 at 80 Mbps video, 640×360 and 24 fps. Its approximately 40 MB size reaches an 8–16 MiB byte ceiling before the 2.5-second time target while bounding the fixture's server memory.

`probe/src/androidTest/assets/startup-fixtures.json` records generated file hashes, stream languages/default flags, the AVC codec string, and measured budget duration/bitrate/size. Media generation validates those properties and fails if the expected streams or bitrate bounds change. The probe uses platform AVC/AAC decoders and the canonical Activity/JNI shims; full-product FFmpeg, SDL, focus, and physical-device validation remain separate.
