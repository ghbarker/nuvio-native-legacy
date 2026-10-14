#!/usr/bin/env bash
set -euo pipefail
probe_root_path="$1"
report_directory="$2"
runtime_suite="${3:-startup}"
mkdir -p "$report_directory"
gradle_arguments=(-p "$probe_root_path" --console=plain)
if [[ "$runtime_suite" == startup ]]; then
    gradle_arguments+=(-Pandroid.testInstrumentationRunnerArguments.class=space.nuvio.preloadtest.StartupContractTest)
elif [[ "$runtime_suite" != all ]]; then
    echo 'suite must be startup or all' >&2
    exit 2
fi
# One shell invocation preserves collection after a failed assertion; emulator
# runner must call this script once, not pass its lines as separate commands.
set +e
gradle "${gradle_arguments[@]}" :probe:connectedDebugAndroidTest
runtime_result=$?
adb shell run-as space.nuvio.preloadtest cat files/startup-diagnostics.jsonl > "$report_directory/startup-diagnostics.jsonl" 2> "$report_directory/jsonl-collection.log"
adb logcat -d > "$report_directory/full-logcat.txt" 2> "$report_directory/logcat-collection.log"
set -e
exit "$runtime_result"
