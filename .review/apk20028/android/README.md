# Logs18 compatible Android update

This closed recipe builds one private ARM64/ARM32 test APK. It keeps the proven
Logs17 builder, signing hook and actual APK package/version/certificate/DEX/JNI/
16 KB verification. Source is the accepted Logs17 foundation plus only the
reviewed native High-quality Metahub policy delta in the two public source files.
The existing private 11-path logging overlay remains private, with separate
selected-backdrop diagnostics and version 20028 / 2.0.28-test-logs18.

Fresh source checks run actual native/browser artwork policy normally and under
ASan/UBSan on Linux product/test and macOS product. Private diagnostics get their
own actual-function privacy/cache/lifecycle regressions; this narrow snapshot
test is not a full graphics cache runtime test. Unchanged Java/JVM, removed-addon
and earlier feature suites are not rerun or claimed as fresh passes.

All execution flags remain false until root approves exact source, recipe and
payload pins. No signing key or build configuration is included in the kit.
