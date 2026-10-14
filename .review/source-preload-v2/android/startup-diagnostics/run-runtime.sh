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
# Connected tests may uninstall the probe before Gradle exits. Keep the run-as
# error, and recover the exact JSON records also emitted by the test to logcat.
set +e
python3 - "$report_directory" "$runtime_result" <<'PY'
import json
import pathlib
import sys

directory = pathlib.Path(sys.argv[1])
output = directory / "startup-diagnostics.jsonl"
records = output.read_text().splitlines()
source = "run-as"
if not records:
    source = "logcat"
    records = [line.split("StartupProbe: ", 1)[1]
               for line in (directory / "full-logcat.txt").read_text(errors="replace").splitlines()
               if "StartupProbe: " in line]
    output.write_text("".join(record + "\n" for record in records))
observations = [json.loads(record) for record in records]
expected = {"opaque-mkv-title-audio-choice", "opaque-mkv-next-audio-choice",
            "separate-audio-hls-next-choice", "byte-limited-title-first-frame",
            "early-title-audio-en", "early-title-audio-pt",
            "early-title-manual-english-after-portuguese", "early-next-hls-portuguese",
            "audio-preference-mismatch-cold-default"}
actual = {record["case"] for record in observations}
complete = actual == expected and len(records) == len(expected)
(directory / "metrics-collection.json").write_text(json.dumps({
    "source": source, "records": len(records), "expected_records": len(expected),
    "complete": complete, "missing_cases": sorted(expected - actual),
    "unexpected_cases": sorted(actual - expected)
}, indent=2) + "\n")
if int(sys.argv[2]) == 0 and not complete:
    raise SystemExit("successful runtime suite did not produce every exact startup observation")
PY
collection_result=$?
set -e
if [[ "$runtime_result" -eq 0 ]]; then
    exit "$collection_result"
fi
exit "$runtime_result"
