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

# Check the same settings prefix against the real pinned canonical worktree.
if task_enabled run_settings_proof; then
  task_baseline="${RUNNER_TEMP:?Set RUNNER_TEMP}/canonical-settings-baseline-$task_case"
  git worktree add --detach "$task_baseline" 17b7769dac4273557e1b07f5fb8734df91dd718f
  task_contract="$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["case"]["settings_contract"])' "$task_output/case.json")"
  task_run settings-source python3 "$task_review/build/verify_settings.py" \
    --baseline "$task_baseline" --candidate "$PWD" --contract "$task_review/build/$task_contract" \
    --output "$task_output/settings-source.json" --runtime-fixture "$task_output/settings-runtime.c"
  task_settings_sources=()
  for task_source in src/fontefalha.c src/fontefiltro.c; do
    if [[ -f "$task_source" ]]; then task_settings_sources+=("$task_source"); fi
  done
  cc -std=gnu11 -O1 -g -ffunction-sections -fdata-sections \
    -Isrc -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
    "$task_output/settings-runtime.c" "${task_settings_sources[@]+${task_settings_sources[@]}}" \
    "$task_review/watched/tests/language-test-stubs.c" -Wl,-dead_strip -lm -lpthread \
    -Wno-deprecated-declarations -o "$task_output/settings-runtime"
  task_run settings-runtime "$task_output/settings-runtime"
fi

task_run translations python3 tools/idiomas.py
cc tests/idioma.c src/idioma.c -Isrc -I/opt/homebrew/include -o "$task_output/idioma-native"
task_run native-i18n "$task_output/idioma-native"
task_run translation-keys python3 "$task_review/build/check_translation_keys.py" --source "$PWD" --case "$task_case" --output "$task_output"
task_run home-state bash tests/homeestado.sh
if task_enabled run_watched; then
  export NUVIO_WATCHED_REVIEW="$task_review/watched"
  export NUVIO_WATCHED_RESULTS="$task_output/watched"
  if task_enabled run_retained; then export NUVIO_WATCHED_COMBINED=1; fi
  task_run watched env SANITIZE=0 bash "$task_review/watched/build/run_regressions.sh"
  task_run watched-asan env SANITIZE=1 ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 bash "$task_review/watched/build/run_regressions.sh"
fi
if task_enabled run_standalone_epoch; then
  task_run standalone-epoch env ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 bash "$task_review/standalone-epoch/tests/run.sh" --root "$PWD"
fi
if task_enabled run_preload; then
  task_run controller bash "$task_review/controller/run.sh" "$PWD"
  task_run controller-asan env SANITIZE=1 ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 bash "$task_review/controller/run.sh" "$PWD"
  # NV_ANDROID boundary fixture runs actual src/player.c; platform I/O is controlled.
  # CPU SDK headers supply GLES declarations; actual full Mac builds above use real SDKs.
  task_run player-boundary env SANITIZE=0 PRELOAD_PLAYER_SDK="$task_review/controller/sdk" bash "$task_review/player/tests/preload_player.sh" --root "$PWD"
  task_run player-boundary-asan env SANITIZE=1 PRELOAD_PLAYER_SDK="$task_review/controller/sdk" ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 bash "$task_review/player/tests/preload_player.sh" --root "$PWD"
fi
if task_enabled run_original_full_source_asan; then
  # Execute each original script unchanged, including its full product source list.
  for task_name in fontepref fontepref_file player_retido; do
    task_run "$task_name-original-asan" env SANITIZE=1 ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 bash "tests/$task_name.sh"
  done
fi
if task_enabled run_cache_epoch; then
  task_run cache-epoch env SANITIZE=0 python3 "$task_review/cache-epoch/run.py" --source "$PWD" --output "$task_output/cache-epoch"
  task_run cache-epoch-asan env SANITIZE=1 ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 python3 "$task_review/cache-epoch/run.py" --source "$PWD" --output "$task_output/cache-epoch"
fi
if task_enabled run_origin_snapshots; then
  for task_origin in plugins plex; do
    task_run "origin-$task_origin" env SANITIZE=0 bash "$task_review/$task_origin/run.sh" --root "$PWD" --output "$task_output/origin-$task_origin"
    task_run "origin-$task_origin-asan" env SANITIZE=1 ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 bash "$task_review/$task_origin/run.sh" --root "$PWD" --output "$task_output/origin-$task_origin"
  done
fi
if task_enabled run_next; then
  task_run next-controller env SANITIZE=0 bash "$task_review/next/run.sh" "$PWD"
  task_run next-controller-asan env SANITIZE=1 ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 bash "$task_review/next/run.sh" "$PWD"
fi
if task_enabled run_screens_watched; then
  task_run screens-watched env SANITIZE=0 bash "$task_review/screens-watched/runner" --root "$PWD" --output "$task_output/screens-watched"
  task_run screens-watched-asan env SANITIZE=1 ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 bash "$task_review/screens-watched/runner" --root "$PWD" --output "$task_output/screens-watched"
fi
if task_enabled run_profiles; then
  export NUVIO_PROFILE_RESULTS="$task_output/profile"
  for task_mode in 0 1; do
    task_suffix=""
    if [[ "$task_mode" == 1 ]]; then task_suffix="-asan"; fi
    task_run "profile-ui$task_suffix" env RUN_ASAN="$task_mode" ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 bash "$task_review/profile/ui/run.sh" "$PWD" "$task_review/profile/sdk"
    task_run "profile-data$task_suffix" env SANITIZE="$task_mode" NUVIO_SYNTAX_SDK="$task_review/profile/sdk" ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 bash "$task_review/profile/data/run.sh" "$PWD"
    task_run "profile-live-tv$task_suffix" env SANITIZE="$task_mode" ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 bash "$task_review/profile/live-tv/run.sh" "$PWD"
  done
fi
if task_enabled run_retained; then
  for task_name in stream_parser streamfit_parser fonte_ordem_addon fontecache_vod fontefalha fontefalha_worker ajustes_failover; do
    task_run "$task_name" python3 "$task_review/build/run_retained.py" --source "$PWD" --name "$task_name" --adapter "$task_review/support/stable-profile-session.c"
  done
fi
if task_enabled run_automatic; then
  for task_fixture in "$task_review"/automatic/tests/*; do
    test ! -e "tests/$(basename "$task_fixture")"
    cp "$task_fixture" tests/
  done
  test ! -e tests/pr431_window_geometry.py
  cp "$task_review/window-fix/pr431_window_geometry.py" tests/pr431_window_geometry.py
  task_run automatic-runtime bash tests/pr431_runtime_bounded_review.sh
  task_run bash32 env BASH32=/bin/bash python3 tests/pr431_android_ffmpeg_bash.py
fi

python3 "$task_review/build/verify_native.py"
