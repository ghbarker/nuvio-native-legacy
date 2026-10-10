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
import sys
import tempfile

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

# Execute the persisted-mode helper for both APK identities. The inert boundary
# supplies app data and startup logs, so failures in the actual shell/JSON path
# cannot be hidden by a second implementation of preference editing.
mode_function = re.search(r"^definir_modo_gravado\(\) \{\n.*?^\}", source, re.M | re.S)
assert mode_function, "missing saved mode smoke command"
with tempfile.TemporaryDirectory(prefix="nuvio-smoke-mode-") as directory:
    env = os.environ.copy()
    env.update(TEST_DIR=directory, TEST_PYTHON=sys.executable)
    script = r'''
set -eu
OUT="$TEST_DIR"
DATA="$OUT/app-settings.txt"
TRANSFER="$OUT/transfer.txt"
sleep() { :; }
python3() { "$TEST_PYTHON" "$@"; }
adb() {
  case "$1 $2 ${3:-}" in
    'shell am force-stop') : ;;
    'shell run-as '*)
      [ "$3" = "$PKG" ]
      case "$4" in
        cat) cat "$DATA" ;;
        cp) cp "$TRANSFER" "$DATA" ;;
        *) return 1 ;;
      esac ;;
    'push '*) cp "$2" "$TRANSFER" ;;
    'shell chmod 644') : ;;
    'logcat -c ') : ;;
    'shell am start') : ;;
    'shell pidof '*) printf '123\n' ;;
    'logcat -d --pid=123')
      if [ "$(sed -n 's/^interfaceModoLocal //p' "$DATA")" = 1 ]; then
        echo '[interface] modo=mobile'
      else echo '[interface] modo=tv'; fi ;;
    *) echo "unexpected command: $*" >&2; return 1 ;;
  esac
}
''' + mode_function.group(0) + r'''
for PKG in space.nuvio.nativelegacy space.nuvio.nativelegacy.touch; do
  printf 'tema 2\ninterfaceModoLocal 0\ntamanhoUiLocal 1\n' > "$DATA"
  definir_modo_gravado 1
  [ "$PID" = 123 ]
  grep -qx 'tema 2' "$DATA"
  grep -qx 'tamanhoUiLocal 1' "$DATA"
  [ "$(sed -n 's/^interfaceModoLocal //p' "$DATA")" = 1 ]
  definir_modo_gravado 0
  [ "$(sed -n 's/^interfaceModoLocal //p' "$DATA")" = 0 ]
  if definir_modo_gravado 2; then echo 'invalid mode accepted' >&2; exit 1; fi
done
echo 'android_smoke_mode: actual preference command preserves other settings and restores both modes in one installed package PASS'
'''
    subprocess.run([bash, "-c", script], env=env, check=True)

