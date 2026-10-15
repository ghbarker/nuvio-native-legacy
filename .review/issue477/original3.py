#!/usr/bin/env python3
"""Replay original full-source scripts normally or sanitized with the proven SDL3 rpath repair."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import platform
import subprocess
import sys
from common import HERE, source_binding

SUITES = {
    "fontepref": {
        "c": "717d6226c8933a3fed0a744c861a72641493706c35c4522cd6c602f381a08780",
        "sh": "173a124acfc643af8e6312bcface4097fdd426743b8834ed726ebc6fa03a72e8",
    },
    "fontepref_file": {
        "c": "f973b5476af03cb3d962d36afb85ff7b52855dd213dc53dacb4568cdb682bd20",
        "sh": "171912b66103bb66d256274505f0f9487751f8b0b66426943d8fddb06793a5f1",
    },
    "player_retido": {
        "c": "7931a7e9af2c6572ab1e093720325eb27038f8b68bba2e2079097379b95a2eb1",
        "sh": "ac0581a7dddbd8777515b83776d8b2159a4715059153d11ddf6308c64be7cf78",
    },
}
WRAPPER_SHA256 = "bdcf2d60839e9445da53f70778aac662601af031745ca28b2dc3da2ed7c012b9"


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def git(source, *args):
    return subprocess.check_output(["git", "-C", str(source), *args], text=True).strip()


def verify(source, proof, suite, review_only_ok=False):
    assert proof["verified"] and proof["product_only"]
    assert not proof.get("review_only", False) or review_only_ok
    assert git(source, "rev-parse", "HEAD") == proof["source_commit"]
    assert git(source, "rev-parse", "HEAD^{tree}") == proof["tree"]
    assert not git(source, "status", "--porcelain")
    original = {}
    for suffix, expected in SUITES[suite].items():
        relative = "tests/" + suite + "." + suffix
        data = (source / relative).read_bytes()
        assert sha(source / relative) == expected, relative
        assert data == subprocess.check_output(
            ["git", "-C", str(source), "show", proof["base"] + ":" + relative]
        ), relative
        original[relative] = expected
    return original


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--source", type=Path, required=True)
    p.add_argument("--source-proof", type=Path, required=True)
    p.add_argument("--suite", choices=SUITES, required=True)
    p.add_argument("--wrapper", type=Path, required=True)
    p.add_argument("--output", type=Path, required=True)
    p.add_argument("--preflight-only", action="store_true")
    p.add_argument("--mode", choices=("normal", "asan"), required=True)
    a = p.parse_args()
    source = a.source.resolve()
    out = a.output.resolve()
    assert not out.exists(), "Use a new isolated output directory for every original suite"
    proof = json.loads(a.source_proof.read_text())
    original = verify(source, proof, a.suite, review_only_ok=a.preflight_only)
    assert sha(a.wrapper) == WRAPPER_SHA256, "Only the previously proven rpath wrapper is accepted"
    modules = sorted(str(p.relative_to(source)) for p in (source / "src").glob("*.c"))
    modules += sorted(str(p.relative_to(source)) for p in (source / "src/dts").glob("*.c"))
    assert modules and "src/sinopse.c" in modules
    if a.preflight_only:
        print(json.dumps({"outcome": "preflight-only", "source_commit": proof["source_commit"],
            "suite": a.suite, "original_test_sha256": original,
            "wrapper_sha256": WRAPPER_SHA256, "translation_units": len(modules)}))
        return
    source_binding(source, a.source_proof)
    assert platform.system() == "Darwin", "Actual original scripts require the real macOS SDK"
    compiler = Path(os.environ.get("NUVIO_REVIEW_REAL_CC", "/usr/bin/cc")).resolve()
    assert compiler.is_file()
    assert compiler == Path("/usr/bin/cc").resolve(), "Capture the system cc before adding the wrapper"
    sdl3 = Path(subprocess.check_output(["brew", "--prefix", "sdl3"], text=True).strip()) / "lib"
    assert sdl3.is_dir()
    out.mkdir(parents=True)
    cc_bin = out / "cc-bin"
    cc_bin.mkdir()
    wrapper = cc_bin / "cc"
    wrapper.write_bytes(a.wrapper.read_bytes())
    wrapper.chmod(0o755)
    compile_out = out / "compile"
    compile_out.mkdir()
    env = dict(os.environ, SANITIZE="1" if a.mode == "asan" else "0", ASAN_OPTIONS="detect_leaks=0:halt_on_error=1",
        UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1", REVIEW_REAL_CC=str(compiler),
        REVIEW_SDL3_LIBRARY_DIR=str(sdl3.resolve()), REVIEW_CC_OUTPUT=str(compile_out),
        PATH=str(cc_bin) + os.pathsep + os.environ["PATH"])
    logfile = out / (a.suite + ".log")
    with logfile.open("w") as log:
        result = subprocess.run(["/bin/bash", "tests/" + a.suite + ".sh"], cwd=source,
            stdout=log, stderr=subprocess.STDOUT, env=env, timeout=1200)
    if result.returncode:
        print("\n".join(logfile.read_text(errors="replace").splitlines()[-100:]), flush=True)
        raise SystemExit(a.suite + ": original " + a.mode + " script failed")
    records = [json.loads(p.read_text()) for p in sorted(compile_out.glob("*-invocation.json"))]
    assert records and all(r["returncode"] == 0 for r in records)
    for record in records:
        added = [record["added_link_rpath"]] if record["added_link_rpath"] else []
        assert record["effective_arguments"] == record["arguments"] + added
        assert ("-fsanitize=address,undefined" in record["arguments"]) == (a.mode == "asan")
    matches = [r for r in records if "tests/" + a.suite + ".c" in r["arguments"]]
    assert len(matches) == 1
    record = matches[0]
    omitted = {"src/main.c", "src/fontepref.c"} if a.suite != "player_retido" else {
        "src/main.c", "src/player.c", "src/video.c"}
    assert set(record["arguments"]) & set(modules) == set(modules) - omitted
    assert record["added_link_rpath"] == "-Wl,-rpath," + str(sdl3.resolve())
    if a.suite == "player_retido":
        for module, mandatory in [("src/player.c", "-Dsessao_usuario=nv_test_usuario"),
                ("src/video.c", "-Dvideo_tocar=nv_base_video_tocar")]:
            partial = [r for r in records if module in r["arguments"]]
            assert len(partial) == 1 and "-c" in partial[0]["arguments"]
            assert mandatory in partial[0]["arguments"]
    verify(source, proof, a.suite)
    receipt = {"outcome": "passed", "plan_sha256": sha(HERE / "plan.json"), "suite": a.suite, "script_unchanged": True,
        "source_binding": {k: proof[k] for k in ["source_commit", "tree", "review_commit", "manifest_sha256"]},
        "source_proof_sha256": sha(a.source_proof), "original_test_sha256": original,
        "mode": a.mode, "sanitizers": ["address", "undefined"] if a.mode == "asan" else [], "full_source_files_compiled": len(set(modules) - omitted),
        "original_flags_unchanged": True, "only_added_flag": record["added_link_rpath"],
        "rpath_wrapper_sha256": WRAPPER_SHA256, "log_sha256": sha(logfile),
        "compile_proofs": [{"file": str(p.relative_to(out)), "sha256": sha(p)}
            for p in sorted(compile_out.glob("*-invocation.json"))]}
    (out / ("original-" + a.mode + "-verified.json")).write_text(json.dumps(receipt, indent=2) + "\n")
    print("Original full-source passed:", proof["source_commit"], a.suite, a.mode, flush=True)


if __name__ == "__main__":
    main()
