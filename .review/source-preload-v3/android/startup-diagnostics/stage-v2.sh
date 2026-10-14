#!/usr/bin/env bash
set -euo pipefail
product_source_root="$1"
probe_work_directory="$2"
diagnostic_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
canonical_root="$(cd -- "$diagnostic_root/.." && pwd)"
python3 - "$canonical_root" "$product_source_root" "$probe_work_directory" <<'PY'
from pathlib import Path
import hashlib,json,shutil,sys
base,product,destination=map(Path,sys.argv[1:])
base=base.resolve(); product=product.resolve(); destination=destination.resolve()
assert destination != base and base not in destination.parents, 'isolated output must be outside canonical harness'
assert not destination.exists() or not any(destination.iterdir()), 'isolated destination must be empty'
manifest=json.loads((base/'fixture-manifest.json').read_text())
for item in manifest['helper_files']:
    original=base/item['path']; data=original.read_bytes()
    assert hashlib.sha256(data).hexdigest()==item['sha256'],item['path']
    target=destination/item['path']; target.parent.mkdir(parents=True,exist_ok=True); shutil.copy2(original,target)
for item in manifest['staged_player_helpers']:
    original=product/'android/app/src/main/java'/item['path']
    assert hashlib.sha256(original.read_bytes()).hexdigest()==item['sha256'],item['path']
shutil.copy2(base/'fixture-manifest.json',destination/'fixture-manifest.json')
PY
bash "$probe_work_directory/stage.sh" "$product_source_root"
bash "$probe_work_directory/generate-fixture.sh"
