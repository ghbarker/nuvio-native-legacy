#!/usr/bin/env python3
"""Verify the review checkout has the exact three-file PR 487 product change."""
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess

BASE = "17b7769dac4273557e1b07f5fb8734df91dd718f"
FEATURE = "e4fb8c085b978f3743dd80d19d3da9e6f2b12599"
EXPECTED = ["src/stream_parse.c", "src/streams.c", "src/streams.h"]


def git(*args):
    return subprocess.check_output(["git", *args], text=True).strip()


def paths(*args):
    return sorted(p for p in subprocess.check_output(
        ["git", "diff", "--name-only", "-z", *args]).decode().split("\0") if p)


def main():
    assert re.fullmatch(r"[0-9a-f]{40}", FEATURE), "Fill e4fb8c085b978f3743dd80d19d3da9e6f2b12599 before publication"
    assert git("rev-parse", FEATURE + "^@") == BASE, "Product commit must directly descend from canonical upstream"
    assert git("rev-parse", "HEAD^@") == FEATURE, "Review commit must directly descend from the product commit"
    changed = paths(BASE, FEATURE)
    assert changed == EXPECTED, (changed, EXPECTED)
    review_paths = paths(FEATURE, "HEAD")
    assert review_paths and all(p.startswith((".github/workflows/", ".review/")) for p in review_paths), review_paths
    assert not paths("HEAD", "--", "src", "tests", "tools", "android", "deploy"), "Product checkout must be unmodified"
    verified = []
    for relative in EXPECTED:
        path = Path(relative)
        data = path.read_bytes()
        actual = hashlib.sha1(b"blob " + str(len(data)).encode() + b"\0" + data).hexdigest()
        feature_blob = git("rev-parse", FEATURE + ":" + relative)
        head_blob = git("rev-parse", "HEAD:" + relative)
        mode = git("ls-tree", FEATURE, "--", relative).split()[0]
        assert mode == "100644", (relative, mode)
        assert actual == feature_blob == head_blob, (relative, actual, feature_blob, head_blob)
        assert not path.stat().st_mode & 0o111, (relative, "unexpected executable mode")
        verified.append({"path": relative, "sha": actual, "mode": mode})
    output = Path("build/review")
    output.mkdir(parents=True, exist_ok=True)
    proof = {"base": BASE, "feature_commit": FEATURE,
             "build_commit": os.environ.get("GITHUB_SHA", git("rev-parse", "HEAD")),
             "feature_changed_paths": changed, "review_changed_paths": review_paths,
             "files": verified, "verified": True}
    (output / "source-verified.json").write_text(json.dumps(proof, indent=2) + "\n")
    print("Verified canonical upstream, direct product/review parents and exact three source blobs")


if __name__ == "__main__":
    main()
