"""Exercise the actual smoke input helpers with inert ADB/XML/timing boundaries."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET

source = Path("tools/android-touch-smoke.sh").read_text(encoding="utf-8")
names = ("ime_mostrado", "abrir_campo_email", "aguardar_ime_pronto", "aguardar_campo_email", "digitar_fresco")
functions = {}
for name in names:
    match = re.search(rf"^{name}\(\) \{{\n.*?^\}}", source, re.M | re.S)
    assert match, name
    functions[name] = match.group(0)
assert "assert field.get('text', '') == 'touch-preview@example.invalid'" in source
assert "aguardar_campo_email\naguardar_ime_pronto\ndigitar_fresco 'touch-preview@example.invalid'" in source
bash = os.environ.get("BASH") or shutil.which("bash")
assert bash, "bash required"

with tempfile.TemporaryDirectory(prefix="nuvio-smoke-input-") as tmp:
    directory = Path(tmp)
    for name, focused, bounds in (("ready", "true", "[42,32][2298,137]"),
                                  ("unfocused", "false", "[42,32][2298,137]"),
                                  ("covered", "true", "[42,700][2298,805]")):
        (directory / f"{name}.xml").write_text(
            f'<hierarchy><node class="android.widget.EditText" '
            f'package="space.nuvio.nativelegacy" focused="{focused}" '
            f'bounds="{bounds}" text="" /></hierarchy>', encoding="utf-8")
    (directory / "absent.xml").write_text("<hierarchy />", encoding="utf-8")
    (directory / "other.xml").write_text(
        (directory / "ready.xml").read_text(encoding="utf-8").replace(
            "space.nuvio.nativelegacy", "example.other"), encoding="utf-8")
    (directory / "nonempty.xml").write_text(
        (directory / "ready.xml").read_text(encoding="utf-8").replace(
            'text=""', 'text="t"'), encoding="utf-8")
    environment = os.environ.copy()
    environment.update(TEST_OUT=directory.as_posix(), TEST_PYTHON=Path(sys.executable).as_posix())
    boundary = r'''
set -eu
OUT="$TEST_OUT"
PKG=space.nuvio.nativelegacy
python3() { "$TEST_PYTHON" "$@"; }
clock=0
typed=''
injections=0
last_timestamp=-1
stale=0
ready_after=3
pulls=0
xml=unfocused
ime=0
window_probes=0
window_ready_after=3
ime_missing=''
ime_ready=0
open_first=1
open_never=0
taps=''
sleep() { clock=$((clock+1)); }
tap_logico() {
  taps="$taps $2"
  if [ "$ime" = 1 ]; then typed="${typed}t"; fi
  if [ "$open_never" = 0 ] && { [ "$open_first" = 1 ] || [ "$2" = 406 ]; }; then ime=1; fi
}
adb() {
  case "$1 $2 $3" in
    'shell dumpsys input_method')
      window_probes=$((window_probes+1))
      visible=false;started=false;shown=false;ime_ready=0
      if [ "$window_probes" -ge "$window_ready_after" ]; then
        visible=true;started=true;shown=true;ime_ready=1
      fi
      case "$ime_missing" in
        window) visible=false;ime_ready=0 ;;
        started) started=false;ime_ready=0 ;;
        shown) shown=false;ime_ready=0 ;;
      esac
      printf 'Input Method Manager Service state:\n  mCurId=example.keyboard/example.keyboard.IME\n  mInputShown=%s\n\n' "$([ "$ime" = 1 ] && echo true || echo false)"
      printf 'Input method service state for example.keyboard.IME@123:\n  mWindowVisible=%s\n  mInputViewStarted=%s\n  mIsInputViewShown=%s\n  packageName=%s\n\n' "$visible" "$started" "$shown" "$PKG" ;;
    'shell uiautomator dump') : ;;
    'pull /sdcard/nuvio-touch-editor-ready.xml '*)
      pulls=$((pulls+1))
      if [ "$pulls" -ge "$ready_after" ]; then xml=ready; fi
      cp "$OUT/$xml.xml" "$3" ;;
    'shell input text')
      [ "$pulls" -ge "$ready_after" ] || { echo 'typing before editor ready' >&2; return 1; }
      [ "$ime_ready" = 1 ] || { echo 'typing before IME window ready' >&2; return 1; }
      injections=$((injections+1))
      timestamp=$clock
      [ "$timestamp" -gt "$last_timestamp" ] || { echo 'timestamps not fresh' >&2; return 1; }
      last_timestamp=$timestamp
      value=$4
      for ((k=0;k<${#value};k++)); do
        if [ "$((clock-timestamp))" -gt 10 ]; then stale=$((stale+1));
        else typed="$typed${value:k:1}"; fi
        clock=$((clock+3)) # slow first-boot IME, faster than an event's stale horizon
      done ;;
    *) echo "unexpected adb command: $*" >&2; return 1 ;;
  esac
}
'''
    actual = "\n".join(functions.values())
    checks = r'''
abrir_campo_email
[ "$taps" = ' 928' ] && [ "$typed" = '' ]
# First button may not open the editor; then only one field tap is needed.
taps='';ime=0;open_first=0
abrir_campo_email
[ "$taps" = ' 928 406' ] && [ "$typed" = '' ]
# Never opening an IME exhausts only the existing three candidate buttons.
taps='';ime=0;open_never=1
if abrir_campo_email; then echo 'missing keyboard accepted' >&2; exit 1; fi
[ "$taps" = ' 928 406 470 406 384 406' ]
open_never=0;ime=1
aguardar_campo_email
[ "$pulls" = 3 ]
window_probes=0
aguardar_ime_pronto
[ "$window_probes" = 3 ]
digitar_fresco 'touch-preview@example.invalid'
[ "$typed" = 'touch-preview@example.invalid' ]
[ "$injections" = 29 ] && [ "$stale" = 0 ]
# Requesting input is not enough: every actual IME service flag is required.
for ime_missing in window started shown; do
  window_probes=0;window_ready_after=1
  if aguardar_ime_pronto 2>/dev/null; then echo 'incomplete IME accepted' >&2; exit 1; fi
  [ "$window_probes" = 20 ]
done
ime_missing='';window_probes=0;window_ready_after=99
if aguardar_ime_pronto 2>/dev/null; then echo 'never-ready IME accepted' >&2; exit 1; fi
[ "$window_probes" = 20 ]
window_ready_after=3
# Covered, unfocused and absent XML must never be accepted as a ready field.
for invalid in covered unfocused absent other nonempty; do
  xml=$invalid;ready_after=99;pulls=0
  if aguardar_campo_email 2>/dev/null; then echo 'invalid editor accepted' >&2; exit 1; fi
  [ "$pulls" = 5 ]
done
'''
    subprocess.run([bash, "-c", boundary + actual + checks], env=environment, check=True)
    no_first_probe = actual.replace("    if ime_mostrado; then return 0; fi\n", "", 1)
    rejected = subprocess.run([bash, "-c", boundary + no_first_probe + checks],
                              env=environment, capture_output=True, text=True)
    assert rejected.returncode != 0, "extra tap must fail the regression"
    old_burst = functions["digitar_fresco"]
    without_fresh = actual.replace(old_burst, 'digitar_fresco() { adb shell input text "$1"; }')
    rejected = subprocess.run([bash, "-c", boundary + without_fresh + checks],
                              env=environment, capture_output=True, text=True)
    assert rejected.returncode != 0, "stale burst must fail the exact text check"
    early = actual.replace(functions["aguardar_campo_email"], 'aguardar_campo_email() { return 0; }')
    rejected = subprocess.run([bash, "-c", boundary + early + checks],
                              env=environment, capture_output=True, text=True)
    assert rejected.returncode != 0, "missing editor readiness must fail"
    early_ime = actual.replace(functions["aguardar_ime_pronto"], 'aguardar_ime_pronto() { return 0; }')
    rejected = subprocess.run([bash, "-c", boundary + early_ime + checks],
                              env=environment, capture_output=True, text=True)
    assert rejected.returncode != 0, "missing IME window readiness must fail"
    print("android_smoke_input: actual helpers wait for focused visible field, avoid keyboard tap, "
          "deliver fresh timestamped characters; strict text and negative controls PASS")

    ime_validator = re.search(r'python3 - "\$OUT/ime-ready\.txt" "\$PKG" <<\x27PY\x27\n(.*?)^PY$',
                              source, re.M | re.S).group(1)
    def ime_state(name="example.keyboard.IME", package="space.nuvio.nativelegacy",
                  window="true", started="true", shown="true"):
        return (f"Input method service state for {name}@123:\n"
                f"  mWindowVisible={window} mDecorViewVisible=true\n"
                f"  mInputViewStarted={started}\n"
                f"  mIsInputViewShown={shown}\n"
                f"  packageName={package}\n\n")
    manager = ("Input Method Manager Service state:\n"
               "  mCurId=example.keyboard/example.keyboard.IME\n  mInputShown=true\n\n")
    cold = ime_state(window="false", started="false", shown="false")
    ime_cases = [
        ("ready", manager + ime_state(), True),
        ("requested-only", manager + cold, False),
        ("window-not-visible", manager + ime_state(window="false"), False),
        ("view-not-started", manager + ime_state(started="false"), False),
        ("view-not-shown", manager + ime_state(shown="false"), False),
        ("wrong-editor", manager + ime_state(package="example.other"), False),
        ("wrong-service", manager + ime_state(name="example.other.IME"), False),
        ("another-ime-ready", manager + cold + ime_state(name="example.other.IME"), False),
        ("split-flags", manager + ime_state(started="false", shown="false") +
         ime_state(name="example.other.IME", window="false"), False),
        ("duplicate-active", manager + ime_state() + ime_state(), False),
        ("manager-hidden", manager.replace("mInputShown=true", "mInputShown=false") +
         "Input method client state for example.client:\n  mInputShown=true\n\n" + ime_state(), False),
        ("missing-current", manager.replace("mCurId=example.keyboard/example.keyboard.IME", "") + ime_state(), False),
    ]
    for label, dump, wanted in ime_cases:
        path = directory / ("ime-" + label + ".txt")
        path.write_text(dump)
        result = subprocess.run([sys.executable, "-c", ime_validator, str(path),
                                 "space.nuvio.nativelegacy"], capture_output=True, text=True)
        assert (result.returncode == 0) == wanted, label + ": " + result.stderr
    print("android_smoke_input: 12 actual IME readiness validators reject cold, incomplete, wrong-editor and unrelated-service states PASS")

    # Execute the actual inline XML validators from the shipped smoke driver.
    # The subprocesses use only controlled hierarchy files, not Android/IME.
    validators = {}
    for name in ("editor-ready", "editor"):
        found = re.findall(rf'python3 - "\$OUT/{name}\.xml" "\$PKG" <<\x27PY\x27\n(.*?)^PY$', source, re.M | re.S)
        assert len(found) == 1, name + " exactly one live validator"
        validators[name] = found[0]

    expected = "touch-preview@example.invalid"
    assert len(expected) == 29
    valid = {"class": "android.widget.EditText", "package": "space.nuvio.nativelegacy",
             "focused": "true", "bounds": "[42,32][2298,137]"}
    cases = []
    for name, text in (("editor-ready", ""), ("editor", expected)):
        cases.append((name, "valid", [dict(valid, text=text)], True))
        for label, changes in (("wrong-package", {"package": "example.other"}),
                               ("unfocused", {"focused": "false"}),
                               ("covered", {"bounds": "[42,700][2298,805]"}),
                               ("narrow", {"bounds": "[42,32][100,137]"}),
                               ("short", {"bounds": "[42,32][2298,50]"}),
                               ("invalid-bounds", {"bounds": "invalid"}),
                               ("wrong-class", {"class": "android.widget.TextView"})):
            cases.append((name, label, [dict(valid, text=text, **changes)], False))
        cases.append((name, "absent", [], False))
        cases.append((name, "duplicate-field", [dict(valid, text=text), dict(valid, text=text)], False))
    cases.extend((
        ("editor-ready", "nonempty-leading-t", [dict(valid, text="t")], False),
        ("editor-ready", "nonempty-expected-email", [dict(valid, text=expected)], False),
        ("editor", "prefix-duplicate", [dict(valid, text="t"+expected)], False),
        ("editor", "missing-internal-character", [dict(valid, text=expected[:8]+expected[9:])], False),
        ("editor", "repeated-internal-character", [dict(valid, text=expected[:9]+expected[8:])], False),
        ("editor", "suffix-junk", [dict(valid, text=expected+"t")], False),
    ))
    for name, label, nodes, wanted in cases:
        hierarchy = ET.Element("hierarchy")
        for attributes in nodes: ET.SubElement(hierarchy, "node", attributes)
        path = directory / (name+"-"+label+".xml")
        ET.ElementTree(hierarchy).write(path, encoding="utf-8", xml_declaration=True)
        result = subprocess.run([sys.executable, "-c", validators[name], str(path), 'space.nuvio.nativelegacy'],
                                capture_output=True, text=True)
        assert (result.returncode == 0) == wanted, name+" "+label+": "+result.stdout+result.stderr
    # Restore only the old suffix validator in a disposable code string. The
    # captured bb regression must be accepted by that old check and rejected
    # by the live exact check above, proving this negative control is useful.
    old_suffix = validators["editor"].replace(
        "field.get('text', '') == 'touch-preview@example.invalid'",
        "field.get('text', '').endswith('touch-preview@example.invalid')")
    assert old_suffix != validators["editor"]
    result = subprocess.run([sys.executable, "-c", old_suffix,
                             str(directory/"editor-prefix-duplicate.xml"), 'space.nuvio.nativelegacy'], capture_output=True, text=True)
    assert result.returncode == 0, "old suffix mutant must reproduce false acceptance"
    print(f"android_smoke_input: {len(cases)} extracted live empty-ready/exact-email validators "
          "PASS; old suffix control reproduces prefix false acceptance; no Android device run")

