#!/usr/bin/env python3
"""Compile and run actual NV_ANDROID rede.c regressions; no network needed."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess
import time


def execute(root, output, mode, expected):
    output.mkdir(parents=True, exist_ok=True)
    fixture = Path(__file__).with_name("android_measured.c")
    compiler = os.environ.get("CC") or shutil.which("clang") or shutil.which("cc")
    source = root / "src/rede.c"
    exe = output / "android-measured"
    cmd = [compiler, "-std=gnu11", "-O1", "-g", "-Wall", "-Wextra", "-Wno-unused-function",
           "-ffunction-sections", "-fdata-sections",
           "-DNV_ANDROID=1", "-DACTUAL_REDE_SOURCE=" + json.dumps(str(source)),
           "-I" + str(root / "src"), str(fixture), str(root / "src/redeurl.c"), "-pthread"]
    if platform.system() == "Darwin":
        cmd.append("-Wl,-dead_strip")
    else:
        cmd += ["-ldl", "-Wl,--gc-sections"]
    if mode == "asan":
        cmd += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
    cmd += ["-o", str(exe)]
    start = time.monotonic()
    compiled = subprocess.run(cmd, text=True, capture_output=True, timeout=60)
    (output / "compile.log").write_text(compiled.stdout + compiled.stderr)
    if compiled.returncode:
        raise RuntimeError(f"Compilation failed ({compiled.returncode}): {output / 'compile.log'}")
    env = dict(os.environ)
    if mode == "asan":
        env.setdefault("ASAN_OPTIONS", "detect_leaks=" + ("0" if platform.system() == "Darwin" else "1") + ":abort_on_error=1")
        env.setdefault("UBSAN_OPTIONS", "halt_on_error=1:print_stacktrace=1")
    result = subprocess.run([str(exe)], text=True, capture_output=True, timeout=30, env=env)
    (output / "run.log").write_text(result.stdout + result.stderr)
    checks = [line for line in result.stdout.splitlines() if line.startswith("CHECK ")]
    bad = [line for line in checks if line.startswith("CHECK FAIL ")]
    if result.returncode != expected or len(checks) != 13:
        raise RuntimeError(f"Unexpected runtime result {result.returncode}, wanted {expected}; checks={len(checks)}: {output / 'run.log'}")
    if expected == 42 and not any("android-first-binary-length-and-data" in line for line in bad):
        raise RuntimeError("Baseline did not reproduce the uploaded log's measured binary-length regression")
    receipt = {"source_root": str(root), "rede_sha256": hashlib.sha256(source.read_bytes()).hexdigest(),
               "mode": mode, "compile_command": cmd, "runtime_command": [str(exe)],
               "exit_code": result.returncode, "expected_exit_code": expected,
               "checks": checks, "elapsed_seconds": round(time.monotonic() - start, 3)}
    (output / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print(json.dumps({"mode": mode, "root": str(root), "exit": result.returncode, "checks": len(checks), "failed_checks": len(bad)}))
    return receipt


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--root", required=True, type=Path)
    p.add_argument("--output", required=True, type=Path)
    p.add_argument("--baseline", type=Path)
    p.add_argument("--mode", choices=["normal", "asan", "both"], default="both")
    args = p.parse_args()
    modes = ["normal", "asan"] if args.mode == "both" else [args.mode]
    receipts = []
    for mode in modes:
        if args.baseline:
            receipts.append(execute(args.baseline.resolve(), args.output / "baseline" / mode, mode, 42))
        receipts.append(execute(args.root.resolve(), args.output / "candidate" / mode, mode, 0))
    args.output.mkdir(parents=True, exist_ok=True)
    (args.output / "report.json").write_text(json.dumps({"runs": receipts}, indent=2) + "\n")


if __name__ == "__main__":
    main()
