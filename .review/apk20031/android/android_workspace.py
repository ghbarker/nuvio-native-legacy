"""Capture Android workspace evidence; remove only a newly created .kotlin cache."""
from pathlib import Path, PurePosixPath
import argparse
import hashlib
import json
import os
import shutil
import stat
import subprocess
import sys

CACHE = "android/.kotlin"


def digest(path):
    h = hashlib.sha256()
    with Path(path).open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def git(source, *args):
    return subprocess.check_output(["git", "-C", str(source), *args])


def binding(source):
    return {"path": str(source), "head": git(source, "rev-parse", "HEAD").decode().strip(),
            "tree": git(source, "rev-parse", "HEAD^{tree}").decode().strip()}


def checked_parts(relative):
    p = PurePosixPath(relative)
    if p.is_absolute() or not p.parts or any(x in ("", ".", "..") for x in p.parts):
        raise RuntimeError("Git returned an unsafe path")
    return p.parts


def metadata(source, relative):
    current = source
    parts = checked_parts(relative)
    for index, part in enumerate(parts):
        current = current / part
        try:
            mode = current.lstat().st_mode
        except FileNotFoundError:
            return {"path": relative, "kind": "missing", "sha256": None}
        if stat.S_ISLNK(mode):
            return {"path": relative, "kind": "symlink", "sha256": None,
                    "symlink_at": "/".join(parts[:index + 1]),
                    "link_target_sha256": hashlib.sha256(os.fsencode(os.readlink(current))).hexdigest()}
    if stat.S_ISREG(mode):
        return {"path": relative, "kind": "file", "bytes": current.stat().st_size,
                "sha256": digest(current)}
    return {"path": relative, "kind": "directory" if stat.S_ISDIR(mode) else "special",
            "sha256": None}


def status(source):
    raw = git(source, "status", "--porcelain=v1", "-z", "--untracked-files=all")
    fields = raw.split(b"\0")
    entries = []
    index = 0
    while index < len(fields) and fields[index]:
        record = fields[index]
        if len(record) < 4 or record[2:3] != b" ":
            raise RuntimeError("Malformed git porcelain record")
        code = record[:2].decode("ascii")
        relative = os.fsdecode(record[3:])
        entry = {"status": code, **metadata(source, relative)}
        if "R" in code or "C" in code:
            index += 1
            if index >= len(fields) or not fields[index]:
                raise RuntimeError("Missing git rename source")
            entry["original_path"] = os.fsdecode(fields[index])
            checked_parts(entry["original_path"])
        entries.append(entry)
        index += 1
    return {"raw_sha256": hashlib.sha256(raw).hexdigest(), "entries": entries}


def cache_state(source):
    tracked = [os.fsdecode(x) for x in git(source, "ls-files", "-z", "--", CACHE).split(b"\0") if x]
    root = metadata(source, CACHE)
    files = []
    if root["kind"] == "directory":
        for directory, dirs, names in os.walk(source / CACHE, followlinks=False):
            for name in sorted(dirs + names):
                files.append(metadata(source, (Path(directory) / name).relative_to(source).as_posix()))
    elif root["kind"] != "missing":
        files.append(root)
    return {"present": root["kind"] != "missing", "root": root,
            "tracked_paths": tracked, "entries": sorted(files, key=lambda x: x["path"])}


def run(source, output, phase):
    source = Path(source).resolve(strict=True)
    output = Path(output).resolve()
    if output == source or source in output.parents:
        raise RuntimeError("Evidence directory must be outside the source checkout")
    output.mkdir(parents=True, exist_ok=True)
    receipt_path = output / (phase + ".json")
    if receipt_path.exists():
        raise RuntimeError("Refusing to overwrite workspace evidence")
    receipt = {"schema": 1, "phase": phase, "outcome": "failed", "known_cache": CACHE,
               "known_cache_removed": False, "script_sha256": digest(__file__)}
    try:
        receipt["source"] = binding(source)
        receipt["source_commit"] = receipt["source"]["head"]
        receipt["source_tree"] = receipt["source"]["tree"]
        receipt["porcelain_before"] = status(source)
        receipt["cache_before_cleanup"] = cache_state(source)
        state = receipt["cache_before_cleanup"]
        if phase == "before":
            if receipt["porcelain_before"]["entries"] or git(source, "status", "--porcelain"):
                raise RuntimeError("Source must be strictly clean before the build")
            if state["present"] or state["tracked_paths"]:
                raise RuntimeError("Known cache must be absent and have no tracked paths before the build")
        else:
            prior_path = output / "before.json"
            receipt["before_sha256"] = digest(prior_path)
            prior = json.loads(prior_path.read_text())
            if (prior.get("outcome") != "passed" or prior.get("phase") != "before"
                    or prior.get("script_sha256") != receipt["script_sha256"]
                    or prior.get("source") != receipt["source"]
                    or prior.get("porcelain_before", {}).get("entries")
                    or prior.get("cache_before_cleanup", {}).get("present") is not False
                    or prior.get("cache_before_cleanup", {}).get("tracked_paths") != []):
                raise RuntimeError("Before-build absence, clean source and identical source/script bindings are required")
            entries = receipt["porcelain_before"]["entries"]
            if state["tracked_paths"]:
                raise RuntimeError("Known cache contains tracked paths; preserving all dirt")
            if any(x["status"] != "??" or not x["path"].startswith(CACHE + "/") for x in entries):
                raise RuntimeError("Unexpected tracked or untracked dirt; preserving all dirt")
            if state["present"]:
                if state["root"]["kind"] != "directory" or any(x["kind"] not in ("file", "directory") for x in state["entries"]):
                    raise RuntimeError("Known cache contains a symlink or special entry; preserving all dirt")
                if entries:
                    # Cleanup is allowed only when this is the observed untracked dirt.
                    shutil.rmtree(source / CACHE)
                    receipt["known_cache_removed"] = True
                # A clean empty/ignored build directory needs no cleanup. The strict
                # complete worktree check below remains authoritative.
        receipt["porcelain_after"] = status(source)
        receipt["strict_clean_after"] = not receipt["porcelain_after"]["entries"] and not git(source, "status", "--porcelain")
        if not receipt["strict_clean_after"]:
            raise RuntimeError("Strict git-clean verification failed")
        if binding(source) != receipt["source"]:
            raise RuntimeError("Source HEAD or tree changed during workspace verification")
        receipt["outcome"] = "passed"
    except Exception as exc:
        receipt["error"] = {"type": type(exc).__name__, "message": str(exc)}
    receipt_path.write_text(json.dumps(receipt, indent=2, sort_keys=True) + "\n")
    if receipt["outcome"] != "passed":
        raise RuntimeError(receipt["error"]["message"])
    print("PASS Android workspace", phase, "cache_removed=" + str(receipt["known_cache_removed"]))
    return receipt


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--phase", choices=("before", "after"), required=True)
    args = parser.parse_args()
    try:
        run(args.source, args.output, args.phase)
    except Exception as exc:
        print("STOP:", exc, file=sys.stderr)
        sys.exit(1)
