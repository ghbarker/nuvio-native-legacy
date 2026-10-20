#!/bin/bash
set -uo pipefail
cd "$(dirname "$0")/../.."
result=0
bash android/gradlew -p tests/android-live-bench connectedDebugAndroidTest --no-daemon --stacktrace \
  > live-bench-connected.log 2>&1 || result=$?
# Gradle uninstalls the target after instrumentation. Its final structured
# result is also in logcat, so do not try run-as against the removed package.
adb logcat -d -v threadtime > live-bench-logcat.txt || result=1
python3 - <<'PY' || result=1
import json,pathlib,re
found=[]
for line in pathlib.Path('live-bench-logcat.txt').read_text(errors='replace').splitlines():
    match=re.search(r'\bLiveBench\s*:\s*(\[.*\])\s*$',line)
    if match: found=json.loads(match.group(1))
pathlib.Path('live-bench-results.json').write_text(json.dumps(found,indent=2)+'\n')
assert len(found)==4,found
print(json.dumps(found,indent=2))
PY
if [ "$result" != 0 ]; then head -140 live-bench-connected.log; fi
exit "$result"
