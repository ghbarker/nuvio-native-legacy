#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
    printf 'Usage: %s <product-root> <empty-workdir>\n' "$0" >&2
    exit 2
fi

here="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
product_root="$(cd -- "$1" && pwd)"
workdir="$2"

# Copy only the manifest's canonical harness files. Existing build output,
# runtime assets, and this diagnostic kit never enter the working copy.
python3 - "$here" "$product_root" "$workdir" <<'PY'
import hashlib
import json
from pathlib import Path
import shutil
import sys

kit = Path(sys.argv[1]).resolve()
product = Path(sys.argv[2]).resolve()
work = Path(sys.argv[3]).resolve()
canonical = kit.parent
manifest_path = canonical / "fixture-manifest.json"
expected_manifest = "569673adca17f3779713c2096ddcb4d0c55749d483f54d387c5bf054d800cc5d"

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

if work == canonical or canonical in work.parents:
    raise SystemExit("The working directory must be outside the frozen canonical harness.")
if work.exists() and (not work.is_dir() or any(work.iterdir())):
    raise SystemExit("The working directory must be absent or empty.")
if digest(manifest_path) != expected_manifest:
    raise SystemExit("The frozen canonical fixture manifest changed.")
manifest = json.loads(manifest_path.read_text())
diagnostic_test = kit / "StartupContractTest.kt"
if not diagnostic_test.is_file():
    raise SystemExit("StartupContractTest.kt is required beside this stage.sh.")

for entry in manifest["helper_files"]:
    source = canonical / entry["path"]
    if digest(source) != entry["sha256"]:
        raise SystemExit(f"Frozen helper changed: {entry['path']}")
for entry in manifest["staged_player_helpers"]:
    source = product / "android/app/src/main/java" / entry["path"]
    if digest(source) != entry["sha256"]:
        raise SystemExit(f"Product source does not match the frozen backend: {entry['path']}")

work.mkdir(parents=True, exist_ok=True)
for entry in manifest["helper_files"]:
    target = work / entry["path"]
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(canonical / entry["path"], target)
shutil.copy2(manifest_path, work / manifest_path.name)
test_target = work / "probe/src/androidTest/java/space/nuvio/preloadtest/StartupContractTest.kt"
test_target.parent.mkdir(parents=True, exist_ok=True)
shutil.copy2(diagnostic_test, test_target)
PY

bash "$workdir/stage.sh" "$product_root"

python3 - "$here" "$product_root" "$workdir" <<'PY'
import hashlib
import json
from pathlib import Path
import subprocess
import sys

kit, product, work = (Path(value).resolve() for value in sys.argv[1:])
manifest = json.loads((work / "fixture-manifest.json").read_text())
for entry in manifest["staged_player_helpers"]:
    path = work / "probe/src/player" / entry["path"]
    if hashlib.sha256(path.read_bytes()).hexdigest() != entry["sha256"]:
        raise SystemExit(f"Staged product source changed: {entry['path']}")

revision = subprocess.run(["git", "-C", str(product), "rev-parse", "HEAD"],
                          capture_output=True, text=True)
test = work / "probe/src/androidTest/java/space/nuvio/preloadtest/StartupContractTest.kt"
record = {
    "format": 1,
    "intended_source_commit": "65d3da65c6abbbef584efbf2b7f0bb1f63c9e7f1",
    "observed_product_git_head": revision.stdout.strip() if revision.returncode == 0 else None,
    "product_sources_verified": len(manifest["staged_player_helpers"]),
    "player_sha256": manifest["product"]["sha256"],
    "canonical_fixture_manifest_sha256": hashlib.sha256((work / "fixture-manifest.json").read_bytes()).hexdigest(),
    "startup_test_sha256": hashlib.sha256(test.read_bytes()).hexdigest(),
    "instrumentation_class": "space.nuvio.preloadtest.StartupContractTest",
}
(work / "startup-stage.json").write_text(json.dumps(record, indent=2) + "\n")
print(f"Staged {record['product_sources_verified']} frozen player sources and both test classes in {work}")
PY
