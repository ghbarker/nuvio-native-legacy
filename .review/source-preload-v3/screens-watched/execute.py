#!/usr/bin/env python3
"""Compile actual pinned C source against portable review boundary fixtures."""
import argparse
import hashlib
import json
import os
import pathlib
import platform
import subprocess
import sys

PACKAGE = pathlib.Path(__file__).resolve().parent
CASES = [
    ("discovery", False, "watched_discovery.c"),
    ("catalog-ui-tv", False, "watched_catalog_ui.c"),
    ("catalog-ui-touch", True, "watched_catalog_ui.c"),
    ("draw-tv", False, "watched_draw.c"),
    ("draw-touch", True, "watched_draw.c"),
]

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def write_json(path, value):
    temporary = path.with_name(path.name + ".tmp")
    temporary.write_text(json.dumps(value, indent=2) + "\n")
    temporary.replace(path)

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--root", required=True)
parser.add_argument("--output", required=True)
parser.add_argument("--sdk", required=True)
parser.add_argument("--sanitize", choices=["0", "1"], default="0")
args = parser.parse_args()
source = pathlib.Path(args.root).resolve()
output = pathlib.Path(args.output).resolve()
sdk = pathlib.Path(args.sdk).resolve()
sanitized = args.sanitize == "1"
mode = "asan-ubsan" if sanitized else "normal"
mode_output = output / mode
mode_output.mkdir(parents=True, exist_ok=True)
pins = json.loads((PACKAGE / "source-pins.json").read_text())
proof = []
for relative, expected in sorted(pins["sha256"].items()):
    path = source / relative
    actual = digest(path) if path.is_file() else None
    proof.append({"path": relative, "expected_sha256": expected, "actual_sha256": actual,
                  "matches": actual == expected})
fixture_proof = []
for relative, expected in sorted(pins["fixture_sha256"].items()):
    path = PACKAGE / relative
    actual = digest(path) if path.is_file() else None
    fixture_proof.append({"path": relative, "expected_sha256": expected,
                          "actual_sha256": actual, "matches": actual == expected})
source_proof = {"root": str(source), "source_pins": proof, "fixture_pins": fixture_proof,
                "passes": all(row["matches"] for row in proof + fixture_proof)}
write_json(mode_output / "source-proof.json", source_proof)
if not source_proof["passes"]:
    for row in proof + fixture_proof:
        if not row["matches"]:
            print("Source pin mismatch:", row["path"], file=sys.stderr)
    print("No checks ran: supplied source or fixtures differ from the tested package.", file=sys.stderr)
    raise SystemExit(2)

host = platform.system()
if host not in ["Linux", "Darwin"]:
    raise SystemExit("Runner supports Linux and macOS only.")
common = [os.environ.get("CC", "cc"), "-std=gnu11", "-O1", "-g", "-DNV_TIZEN",
          "-DNV_GLES_NO_MAC", "-ffunction-sections", "-fdata-sections",
          "-I" + str(sdk), "-I" + str(source / "src"), "-I" + str(source / "tests"),
          "-include", str(PACKAGE / "tests/sdk-extra.h")]
link = ["-lm", "-lpthread", "-Wl,-dead_strip" if host == "Darwin" else "-Wl,--gc-sections"]
if sanitized:
    common += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
    if host == "Linux":
        common += ["-fno-pie", "-no-pie"]
environment = os.environ.copy()
if sanitized:
    environment.setdefault("ASAN_OPTIONS", "detect_leaks=0:halt_on_error=1")
    environment.setdefault("UBSAN_OPTIONS", "halt_on_error=1:print_stacktrace=1")

results = []
for name, touch, fixture in CASES:
    binary = mode_output / name
    fixture_path = PACKAGE / "tests" / fixture
    if fixture == "watched_discovery.c":
        sources = [source / "src/catalogo.c", source / "src/cwordem.c", fixture_path,
                   source / "src/js.c", source / "src/metaprov.c"]
    else:
        sources = [fixture_path, source / "src/descoberta.c", source / "src/catalogo.c"]
    sources.append(PACKAGE / "../watched/tests/session-test-adapter.c")
    command = common + (["-DNV_TOUCH_UI"] if touch else []) + list(map(str, sources)) + link + ["-o", str(binary)]
    build = subprocess.run(command, cwd=source, capture_output=True, text=True)
    build_log = mode_output / (name + ".build.log")
    build_log.write_text(build.stdout + build.stderr)
    record = {"name": name, "fixture": str(fixture_path), "touch_build": touch,
              "build_command": command, "build_exit": build.returncode, "build_log": str(build_log),
              "skipped": False, "cwd": str(source)}
    if build.returncode == 0:
        run_command = [str(binary)]
        try:
            run = subprocess.run(run_command, cwd=source, env=environment,
                                 capture_output=True, text=True, timeout=30)
            run_exit, run_output = run.returncode, run.stdout + run.stderr
        except subprocess.TimeoutExpired as error:
            run_exit = 124
            run_output = "Fixture exceeded its 30-second deadline.\n"
            for text in [error.stdout, error.stderr]:
                run_output += text.decode(errors="replace") if isinstance(text, bytes) else text or ""
        run_log = mode_output / (name + ".log")
        run_log.write_text(run_output)
        record.update(run_command=run_command, exit=run_exit, log=str(run_log))
    results.append(record)
    print(mode + "/" + name + ": " + ("PASS" if record.get("exit") == 0 else "FAIL"), flush=True)
    write_json(mode_output / "checks.json", results)

evidence = {"mode": mode, "root": str(source), "host": host, "checks_expected": 5,
            "checks_run": len(results), "checks_passed": sum(row.get("exit") == 0 for row in results),
            "no_skips": all(not row["skipped"] for row in results), "checks": results,
            "source_proof": str(mode_output / "source-proof.json"),
            "source_pin_sha256": digest(PACKAGE / "source-pins.json"),
            "sdk_header_sha256": {str(path.relative_to(sdk)): digest(path)
                                  for path in sdk.rglob("*.h")},
            "sanitizer_environment": {key: environment.get(key) for key in ["ASAN_OPTIONS", "UBSAN_OPTIONS"]}
                                      if sanitized else {},
            "limitations": ["Declaration-only SDL/GLES headers and typed graphics boundaries; no full application link or GPU pixels.",
                            "LeakSanitizer is disabled by default because managed ptrace blocks it.",
                            "Real profile-session suites are packaged separately; these isolated modules use their optional session boundary."]}
write_json(mode_output / "evidence.json", evidence)
combined = []
for prior_mode in ["normal", "asan-ubsan"]:
    path = output / prior_mode / "evidence.json"
    if path.is_file():
        combined.append(json.loads(path.read_text()))
aggregate = {"checks_expected": 10, "checks_run": sum(row["checks_run"] for row in combined),
             "checks_passed": sum(row["checks_passed"] for row in combined),
             "no_skips": all(row["no_skips"] for row in combined), "modes": combined}
aggregate["passes_all_10"] = len(combined) == 2 and aggregate["checks_run"] == aggregate["checks_passed"] == 10
write_json(output / "evidence.json", aggregate)
raise SystemExit(0 if evidence["checks_passed"] == 5 else 1)
