#!/bin/bash
set -euo pipefail
task_review="${NUVIO_SOURCE_PRELOAD_REVIEW:?Set the review fixture directory}"
task_output="${NUVIO_SOURCE_PRELOAD_RESULTS:?Set the results directory}"
task_plan="$task_review/build/plans.json"
task_case="${NUVIO_SOURCE_PRELOAD_CASE:?Set the case ID}"
mkdir -p "$task_output"
python3 - "$task_plan" "$task_case" > "$task_output/case.json" <<'PY'
import json, sys
plan=json.load(open(sys.argv[1]))
case=next(c for c in plan['cases'] if c['id']==sys.argv[2])
print(json.dumps({'phase':plan['validation_phase'],'case':case}))
PY

# Match tools/build-local.sh's full app compile in both compile-time UI modes.
for task_mode in tv mobile; do
  task_flags=()
  if [[ "$task_mode" == mobile ]]; then task_flags+=(-DNV_TOUCH_UI=1); fi
  cc src/*.c src/dts/*.c "${task_flags[@]+${task_flags[@]}}" \
    -o "$task_output/app-$task_mode" -O1 \
    -DNV_SUPABASE_URL='""' -DNV_SUPABASE_ANON_KEY='""' -DNV_TV_LOGIN_BASE='""' \
    -DNV_TRAKT_CLIENT_ID='""' -DNV_TRAKT_CLIENT_SECRET='""' \
    -DNV_SIMKL_CLIENT_ID='""' -DNV_SIMKL_APP='""' -DNV_TMDB_API_KEY='""' \
    -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
    -L/opt/homebrew/lib -lSDL2 -lSDL2_image -lSDL2_ttf -lz \
    -framework OpenGL -Wno-deprecated-declarations \
    2>&1 | tee "$task_output/app-$task_mode-build.log"
done

task_phase="$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["phase"])' "$task_output/case.json")"
if [[ "$task_phase" == compile ]]; then
  printf '%s\n' 'Compile phase only; behavior tests and release publication are gated on a final run.' > "$task_output/phase.txt"
  exit 0
fi

task_enabled() {
  python3 -c 'import json,sys; sys.exit(0 if json.load(open(sys.argv[1]))["case"].get(sys.argv[2],False) else 1)' "$task_output/case.json" "$1"
}
task_run() {
  local task_name="$1"
  shift
  "$@" 2>&1 | tee "$task_output/$task_name.log"
  printf '%s PASS\n' "$task_name" >> "$task_output/results.txt"
}

task_run translations python3 tools/idiomas.py
task_run home-state bash tests/homeestado.sh
if task_enabled run_watched; then
  export NUVIO_WATCHED_REVIEW="$task_review/watched"
  export NUVIO_WATCHED_RESULTS="$task_output/watched"
  if task_enabled run_retained; then export NUVIO_WATCHED_COMBINED=1; fi
  task_run watched bash "$task_review/watched/build/run_regressions.sh"
fi
if task_enabled run_preload; then
  task_run controller bash "$task_review/controller/run.sh" "$PWD"
  task_run controller-asan env SANITIZE=1 ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 bash "$task_review/controller/run.sh" "$PWD"
fi
if task_enabled run_retained; then
  for task_name in stream_parser streamfit_parser fonte_ordem_addon fontecache_vod fontefalha fontefalha_worker ajustes_failover; do
    task_run "$task_name" bash "tests/$task_name.sh"
  done
fi
if task_enabled run_automatic; then
  for task_fixture in "$task_review"/automatic/tests/*; do
    test ! -e "tests/$(basename "$task_fixture")"
    cp "$task_fixture" tests/
  done
  task_run automatic-runtime bash tests/pr431_runtime_review.sh
  task_run bash32 env BASH32=/bin/bash python3 tests/pr431_android_ffmpeg_bash.py
fi
