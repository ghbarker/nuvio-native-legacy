#!/usr/bin/env python3
"""macOS-only actual GL captures, external fixture and unmodified product source."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import subprocess
import tempfile

p = argparse.ArgumentParser()
p.add_argument("--root", type=Path, required=True)
p.add_argument("--output", type=Path, required=True)
p.add_argument("--touch", action="store_true")
a = p.parse_args()
assert platform.system() == "Darwin", "GL capture requires the actual macOS/OpenGL build"
root = a.root.resolve(); output = a.output.resolve(); output.mkdir(parents=True, exist_ok=True)
here = Path(__file__).resolve().parent
sha = lambda path: hashlib.sha256(Path(path).read_bytes()).hexdigest()
source_files = sorted([p for p in (root/"src").glob("*.c") if p.name != "main.c"] + list((root/"src/dts").glob("*.c")))
watched = source_files + sorted((root/"src").glob("*.h")) + [root/"tests/legendas_shot.c"]
before = {str(p.relative_to(root)): sha(p) for p in watched}
binary = output/"title-visual"
command = [os.environ.get("CC", "cc"), "-DNV_SHOT_HOOKS"]
if a.touch:
    command += ["-DNV_TOUCH_UI"]
command += ["-DSOURCE_VISUAL_TEST=\""+str(root/"tests/legendas_shot.c")+"\""]
command += [str(p) for p in source_files] + [str(here/"tests/title_visual.c"), "-I"+str(root/"src"), "-o", str(binary), "-O1", "-g", "-I/opt/homebrew/include", "-I/opt/homebrew/include/SDL2", "-L/opt/homebrew/lib", "-lSDL2", "-lSDL2_image", "-lSDL2_ttf", "-lz", "-framework", "OpenGL", "-Wno-deprecated-declarations", "-Wno-macro-redefined"]
with (output/"compile.log").open("w") as log:
    compile = subprocess.run(command, cwd=root, stdout=log, stderr=subprocess.STDOUT, timeout=900)
report = {"root": str(root), "source_sha256": before, "fixture_sha256": sha(here/"tests/title_visual.c"), "compile_command": command, "compile_exit_code": compile.returncode, "touch": a.touch}
if compile.returncode == 0:
    with tempfile.TemporaryDirectory(prefix="nuvio-title-visual-") as data:
        env = os.environ.copy(); env["NUVIO_DADOS"] = data; env["NUVIO_TESTE_DIR"] = data
        with (output/"capture.log").open("w") as log:
            result = subprocess.run([str(binary), str(output)], cwd=root, env=env, stdout=log, stderr=subprocess.STDOUT, timeout=120)
        report["capture_exit_code"] = result.returncode
report["captures"] = {p.name: {"bytes": p.stat().st_size, "sha256": sha(p)} for p in output.glob("subtitle-titles-*.bmp")}
report["source_unchanged"] = before == {str(p.relative_to(root)): sha(p) for p in watched}
report["passed"] = report["compile_exit_code"] == 0 and report.get("capture_exit_code") == 0 and len(report["captures"]) == 3 and report["source_unchanged"]
(output/"visual-report.json").write_text(json.dumps(report, indent=2)+"\n")
raise SystemExit(0 if report["passed"] else 1)
