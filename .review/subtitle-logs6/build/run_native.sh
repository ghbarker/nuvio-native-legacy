#!/bin/bash
set -euo pipefail
task_review="${NUVIO_SOURCE_PRELOAD_REVIEW:?Set review fixture directory}"
task_output="${NUVIO_SOURCE_PRELOAD_RESULTS:?Set result directory}"
task_case="${NUVIO_SOURCE_PRELOAD_CASE:?Set case ID}"
mkdir -p "$task_output"
python3 - "$task_review/build/plans.json" "$task_case" "$task_output" <<'PY'
import hashlib,json,subprocess,sys
from pathlib import Path
plan=json.load(open(sys.argv[1]));case=next(c for c in plan['cases'] if c['id']==sys.argv[2])
out=Path(sys.argv[3]);(out/'case.json').write_text(json.dumps({'phase':plan['validation_phase'],'case':case}))
records={}
for name in ['legendasui','legendasui_425','legmemoria','faixasmkv','faixasmkv_casar','linguas']:
 for suffix in ('.c','.sh'):
  rel='tests/'+name+suffix
  if not Path(rel).exists():
   assert name=='linguas' and suffix=='.sh';continue
  data=Path(rel).read_bytes();expected=subprocess.check_output(['git','show',case['base']+':'+rel])
  assert data==expected,('Original changed',rel)
  records[rel]=hashlib.sha256(data).hexdigest()
(out/'original-tests-verified.json').write_text(json.dumps(records,indent=2)+'\n')
PY
task_run() {
  local task_name="$1"
  shift
  "$@" 2>&1 | tee "$task_output/$task_name.log"
  printf '%s PASS\n' "$task_name" >> "$task_output/results.txt"
}

# Compile the actual complete app with both compile-time UI configurations.
for task_mode in tv mobile; do
  task_flags=()
  if [[ "$task_mode" == mobile ]]; then task_flags+=(-DNV_TOUCH_UI=1); fi
  cc src/*.c src/dts/*.c ${task_flags[@]+"${task_flags[@]}"} \
    -o "$task_output/app-$task_mode" -O1 \
    -DNV_SUPABASE_URL='""' -DNV_SUPABASE_ANON_KEY='""' -DNV_TV_LOGIN_BASE='""' \
    -DNV_TRAKT_CLIENT_ID='""' -DNV_TRAKT_CLIENT_SECRET='""' \
    -DNV_SIMKL_CLIENT_ID='""' -DNV_SIMKL_APP='""' -DNV_TMDB_API_KEY='""' \
    -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
    -L/opt/homebrew/lib -lSDL2 -lSDL2_image -lSDL2_ttf -lz \
    -framework OpenGL -Wno-deprecated-declarations \
    2>&1 | tee "$task_output/app-$task_mode-build.log"
done

for task_name in legendasui legendasui_425; do
  task_run "$task_name-original" env SANITIZE=0 bash "tests/$task_name.sh"
  task_run "$task_name-original-asan" env SANITIZE=1 ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 bash "tests/$task_name.sh"
done
for task_name in legmemoria faixasmkv faixasmkv_casar; do
  task_run "$task_name-original" bash "tests/$task_name.sh"
  task_run "$task_name-sanitized-replay" env ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 python3 "$task_review/build/run_original_sanitized.py" --name "$task_name" --output "$task_output/sanitized-originals"
done
for task_mode in normal asan; do
  task_flags=()
  if [[ "$task_mode" == asan ]]; then task_flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all); fi
  cc tests/linguas.c src/linguas.c -Isrc -O1 -g ${task_flags[@]+"${task_flags[@]}"} -o "$task_output/linguas-$task_mode"
  task_run "linguas-$task_mode" env ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 "$task_output/linguas-$task_mode"
done
task_run subtitle-title-fixtures env ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 python3 "$task_review/subtitle-titles/run.py" --root "$PWD" --output "$task_output/subtitle-titles" --mode both
if [[ "$task_case" == standalone-subtitles ]]; then
  task_run subtitle-visual-tv python3 "$task_review/subtitle-titles/run_visual.py" --root "$PWD" --output "$task_output/subtitle-visual-tv"
  task_run subtitle-visual-touch python3 "$task_review/subtitle-titles/run_visual.py" --root "$PWD" --output "$task_output/subtitle-visual-touch" --touch
fi

task_delivery="$(python3 -c 'import json,sys; print(str(json.load(open(sys.argv[1]))["case"]["delivery"]).lower())' "$task_output/case.json")"
if [[ "$task_delivery" == true ]]; then
  task_run original-log-source python3 "$task_review/test-local-logs/verify-original.py" "$PWD" --output "$task_output/original-log-source.json"
  task_run original-log-and-download-guards bash "$task_review/test-local-logs/run-guards.sh" "$PWD" "$task_review/controller/sdk"
  task_run original-log-and-download-guards-asan env SANITIZE=1 ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 bash "$task_review/test-local-logs/run-guards.sh" "$PWD" "$task_review/controller/sdk"
fi
python3 "$task_review/build/verify_native.py"
