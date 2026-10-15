# Logs17 compatible Android update

This closed recipe builds only the private test APK for ARM64 and ARM32. It reuses the proven Logs16 builder, dependency caches and signing hook. It verifies actual APK package, version, certificate, DEX/JNI contracts, both native compile databases and 16 KB native packaging.

The product source is the accepted Logs16 foundation plus only the three-file removed-addon Settings fix. The private overlay retains its existing 11 paths; only versionCode 20027 and versionName 2.0.27-test-logs17 change. Package space.nuvio.nativelegacy.touch and the persistent signing certificate remain unchanged.

The scoped source checks run the accepted 20-case actual C regression normally and under ASan/UBSan. Java, player and previous feature suites are unchanged and are not rerun or claimed as fresh passes.

All execution flags remain false until root approves exact source, recipe and payload pins. No signing key or build configuration is included in this kit.
