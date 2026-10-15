#!/usr/bin/env python3
"""Run the actual native/browser artwork policy against a pinned source checkout."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import shlex
import subprocess


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--source", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--sanitize", choices=("0", "1"), default="0")
    ap.add_argument("--cc", default=os.environ.get("CC", "cc"))
    args = ap.parse_args()
    source, out = Path(args.source).resolve(), Path(args.out).resolve()
    out.mkdir(parents=True, exist_ok=True)
    fixture = Path(__file__).with_name("artehero-fixture.c")
    hashes = {p: sha(source / p) for p in ("src/artehero.c", "src/artehero.h", "src/catalogo.h")}
    commit = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=source, text=True).strip()
    env = os.environ.copy()
    env["ASAN_OPTIONS"] = ("detect_leaks=0" if platform.system() == "Darwin" else "detect_leaks=1") + ":halt_on_error=1"
    env["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
    results = []
    for browser in (False, True):
        name = "browser" if browser else "native"
        binary, compile_log, run_log = out / name, out / (name + "-compile.log"), out / (name + ".log")
        cmd = shlex.split(args.cc) + [str(source / "src/artehero.c"), str(fixture), "-I" + str(source / "src"),
                                     "-O1", "-g", "-Wall", "-Wextra", "-o", str(binary)]
        if browser:
            cmd += ["-D__EMSCRIPTEN__"]
        if args.sanitize == "1":
            cmd += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
            if platform.system() == "Linux":
                cmd += ["-no-pie"]
        proc = subprocess.run(cmd, capture_output=True, text=True, timeout=60)
        compile_log.write_text(proc.stdout + proc.stderr)
        assert proc.returncode == 0, compile_log
        proc = subprocess.run([str(binary)], capture_output=True, text=True, timeout=60, env=env)
        run_log.write_text(proc.stdout + proc.stderr)
        marker = "artehero (tizen): tudo ok" if browser else "artehero: tudo ok"
        assert proc.returncode == 0 and marker in proc.stdout, run_log
        assert "ok  Metahub: alta nativa" in proc.stdout, run_log
        results.append({"mode": name, "compile_exit": 0, "exit": proc.returncode,
                        "binary_sha256": sha(binary), "compile_log_sha256": sha(compile_log),
                        "log_sha256": sha(run_log), "case_groups": sum(line.startswith("ok  ") for line in proc.stdout.splitlines()),
                        "marker": marker})
        print("PASS artehero " + name + " sanitize=" + args.sanitize, flush=True)
    assert hashes == {p: sha(source / p) for p in hashes}, "source mutated during validation"
    receipt = {"success": True, "source_commit": commit, "source_hashes": hashes,
               "fixture_sha256": sha(fixture), "runner_sha256": sha(__file__),
               "sanitize": args.sanitize == "1", "runs": results}
    (out / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")


if __name__ == "__main__":
    main()
