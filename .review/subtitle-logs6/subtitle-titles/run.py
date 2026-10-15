#!/usr/bin/env python3
"""External source-bound subtitle-title regressions; never edits product/tests."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import subprocess
import time


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def production_function(text, marker):
    start = text.index(marker)
    opening = text.index("{", start)
    # Production C closes these file-scope functions in column zero. Nested
    # blocks are indented; retain the exact function including comments.
    end = text.index("\n}", opening) + 2
    return text[start:end] + "\n"


def run(command, log, timeout=90):
    started = time.monotonic()
    env = os.environ.copy()
    env.setdefault("ASAN_OPTIONS", "detect_leaks=1:halt_on_error=1")
    env.setdefault("UBSAN_OPTIONS", "halt_on_error=1:print_stacktrace=1")
    result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            text=True, timeout=timeout, env=env)
    Path(log).write_text(result.stdout)
    return {"command": command, "exit_code": result.returncode,
            "duration_seconds": round(time.monotonic() - started, 3),
            "log": str(log), "log_sha256": sha(log)}


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--root", type=Path, required=True)
    p.add_argument("--output", type=Path, required=True)
    p.add_argument("--mode", choices=("normal", "asan", "both"), default="both")
    p.add_argument("--sdk", type=Path)
    p.add_argument("--baseline-root", type=Path)
    args = p.parse_args()
    root, output = args.root.resolve(), args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    here = Path(__file__).resolve().parent
    watched = ["src/video.h", "src/video_android.c", "src/legendasui.h",
               "src/legendasui.c", "src/linguas.c", "src/audioinfo.c",
               "src/bidi.c", "src/legauto.c", "tests/legendasui.c",
               "tests/legendasui.sh", "tests/legendasui_425.c",
               "tests/legendasui_425.sh"]
    hashes = {s: sha(root/s) for s in watched}
    if args.baseline_root:
        for name in watched:
            if name.startswith("tests/"):
                assert sha(args.baseline_root/name) == hashes[name], name
    bridge = output/"production-bridge.inc"
    android = (root/"src/video_android.c").read_text()
    fragments = [production_function(android, "static void deJString("),
                 production_function(android, "JNIEXPORT void JNICALL Java_space_nuvio_nativelegacy_NvPlayer_nativeFaixa(")]
    bridge.write_text("\n".join(fragments))
    cc = os.environ.get("CC", "cc")
    flags = ["-std=gnu11", "-g", "-ffunction-sections", "-fdata-sections",
             "-Wall", "-Wextra", "-Wno-unused-function", "-Wno-macro-redefined",
             "-I"+str(root/"src")]
    if args.sdk:
        flags += ["-I"+str(args.sdk.resolve())]
    if platform.system() == "Darwin":
        flags += ["-I/opt/homebrew/include", "-I/opt/homebrew/include/SDL2", "-Wl,-dead_strip"]
    else:
        flags += ["-Wl,--gc-sections"]
    modes = ("normal", "asan") if args.mode == "both" else (args.mode,)
    report = {"root": str(root), "host": platform.platform(), "source_sha256": hashes,
              "fixture_sha256": {p.name: sha(p) for p in (here/"tests").glob("*.c")},
              "production_bridge_sha256": sha(bridge), "records": []}
    for mode in modes:
        extra = [] if mode == "normal" else ["-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-sanitize-recover=all"]
        for case in ("title_rows", "android_names", "upstream_425"):
            binary = output/(case+"-"+mode)
            command = [cc] + flags + extra
            if case == "title_rows":
                command += ["-DSOURCE_SELECTOR_TEST=\""+str(root/"tests/legendasui.c")+"\"",
                            str(here/"tests/title_rows.c"), str(root/"src/linguas.c"), str(root/"src/bidi.c")]
            elif case == "android_names":
                command += ["-DSOURCE_BRIDGE_FUNCTIONS=\""+str(bridge)+"\"",
                            str(here/"tests/android_names.c"), str(root/"src/linguas.c"), str(root/"src/audioinfo.c")]
            else:
                command += [str(root/"tests/legendasui_425.c"), str(root/"src/linguas.c"), str(root/"src/bidi.c")]
            command += ["-o", str(binary)]
            if platform.system() != "Darwin":
                command += ["-lm"]
            record = {"case": case, "mode": mode,
                      "compile": run(command, output/(case+"-"+mode+"-compile.log"))}
            if record["compile"]["exit_code"] == 0:
                cases = ("lista", "direita", "segurar", "voltar", "atraso", "ponteiro") if case == "upstream_425" else (None,)
                record["runs"] = [run([str(binary)] + ([c] if c else []),
                                       output/(case+"-"+mode+("-"+c if c else "")+".log")) for c in cases]
            record["passed"] = record["compile"]["exit_code"] == 0 and all(r["exit_code"] == 0 for r in record.get("runs", []))
            report["records"].append(record)
            print(case, mode, "PASS" if record["passed"] else "FAIL", flush=True)
    assert hashes == {s: sha(root/s) for s in watched}, "Product source changed while testing"
    report["source_unchanged"] = True
    report["passed"] = all(r["passed"] for r in report["records"])
    (output/"report.json").write_text(json.dumps(report, indent=2)+"\n")
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
