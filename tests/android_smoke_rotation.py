"""Run the production smoke rotation command through an inert adb boundary.

UiAutomation can leave automatic rotation enabled; each explicit rotation
must restore the locked mode before setting its requested direction. This
test executes the actual shell function, without an emulator or APK.
"""
from pathlib import Path
import os
import re
import shutil
import subprocess

source = Path("tools/android-touch-smoke.sh").read_text(encoding="utf-8")
function = re.search(r"^pedir_rotacao\(\) \{\n.*?^\}", source, re.M | re.S)
assert function, "missing production rotation command"
calls = re.findall(r"^pedir_rotacao ([013])$", source, re.M)
assert calls == ["1", "0", "1", "3"], calls
assert "assert (x, y, w, h) == (0, 0, *expected)" in source
# No requested direction may bypass the production rotation function after
# the first uiautomator dump (initial setup before the dump is independent).
after_dump = source.split("adb shell uiautomator dump", 1)[1]
assert re.findall(r"^\s*adb shell settings put system user_rotation", after_dump.replace(function.group(0), ""), re.M) == []
bash = os.environ.get("BASH") or shutil.which("bash")
assert bash, "bash is required to execute the actual rotation function"
script = r'''
set -eu
mode=free
rotation=0
locks=0
requests=0
adb() {
  case "$*" in
    'shell settings put system accelerometer_rotation 0') mode=locked; locks=$((locks+1));;
    'shell settings put system user_rotation '*)
      [ "$mode" = locked ] || { echo 'rotation requested with sensor still controlling display' >&2; return 1; }
      rotation=${6}; requests=$((requests+1));;
    *) echo "unexpected adb command: $*" >&2; return 1;;
  esac
}
''' + function.group(0) + r'''
for direction in 1 0 1 3; do
  mode=free # the UiAutomation disconnect state observed in the failed smoke
  pedir_rotacao "$direction"
  [ "$rotation" = "$direction" ]
done
[ "$locks" = 4 ] && [ "$requests" = 4 ]
echo 'android_smoke_rotation: production command restores locked mode for every requested orientation; exact viewport assertion preserved PASS'
'''
subprocess.run([bash, "-c", script], check=True)
without_lock = script.replace("  adb shell settings put system accelerometer_rotation 0\n", "")
rejected = subprocess.run([bash, "-c", without_lock], capture_output=True, text=True)
assert rejected.returncode != 0 and "sensor still controlling" in rejected.stderr
