Rows inside an opened Showcase collection now load independently, with up to three requests at once. A slow row no longer holds up the following rows. Source order, scroll position, title selection, and collection/profile ownership are preserved.

The Continue Watching and Home startup improvements from Logs15 are retained.

Install this APK over **Nuvio TV/Mobile Test** and choose **Update**. It uses the same package and signing certificate, so your accounts and settings stay in place. No uninstall is needed.

Validation: [all eight CI jobs passed on attempt 1](https://github.com/ghbarker/nuvio-native-legacy/actions/runs/38805446252), including 48 focused feature groups, 47 JVM/Media3 contracts, four macOS application links, all six original macOS normal/ASan/UBSan modes, and both Android APKs with ARM32/ARM64, signing, JNI/DEX, and 16 KB checks.

Source product: `0edc358e5dded17ec3e130947bd8139ab0520d47`. Compatible test overlay: `aef9146623d6c10bd64bec82a5312b0f7dc79ce7`. Version: `2.0.26-test-logs16` (20026).

To check the fix:

1. In Android Settings → Apps → Nuvio TV/Mobile Test, tap **Force stop**, then open the app.
2. Open a collection in **Showcase** that contains at least three source rows. Confirm several rows start loading without waiting for the first one to finish.
3. Scroll while rows load, then tap a title in a completed row. Confirm the position stays stable and the correct title opens.
4. Go back, open another collection, and repeat after switching profiles. Confirm old results stay out of the new view and Continue Watching still loads quickly.
