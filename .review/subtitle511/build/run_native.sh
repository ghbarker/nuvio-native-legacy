#!/bin/bash
set -euo pipefail
task_review="${NUVIO_SUBTITLE_REVIEW:?}"
task_output="${NUVIO_SUBTITLE_RESULTS:?}"
mkdir -p "$task_output"
task_run() {
  local task_name="$1"
  shift
  "$@" 2>&1 | tee "$task_output/$task_name.log"
  printf '%s\n' "$task_name" >> "$task_output/passed-gates.txt"
}

for task_mode in tv mobile; do
  task_flags=()
  if [[ "$task_mode" == mobile ]]; then task_flags+=(-DNV_TOUCH_UI=1); fi
  task_run "full-app-$task_mode" cc src/*.c src/dts/*.c ${task_flags[@]+"${task_flags[@]}"} \
    -o "$task_output/app-$task_mode" -O1 \
    -DNV_SUPABASE_URL='""' -DNV_SUPABASE_ANON_KEY='""' -DNV_TV_LOGIN_BASE='""' \
    -DNV_TRAKT_CLIENT_ID='""' -DNV_TRAKT_CLIENT_SECRET='""' \
    -DNV_SIMKL_CLIENT_ID='""' -DNV_SIMKL_APP='""' -DNV_TMDB_API_KEY='""' \
    -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
    -L/opt/homebrew/lib -lSDL2 -lSDL2_image -lSDL2_ttf -lz \
    -framework OpenGL -Wno-deprecated-declarations
done
for task_name in legendasui legendasui_425; do
  task_run "$task_name-original" env SANITIZE=0 bash "tests/$task_name.sh"
  task_run "$task_name-original-asan" env SANITIZE=1 ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 bash "tests/$task_name.sh"
done
for task_name in legmemoria faixasmkv faixasmkv_casar; do
  task_run "$task_name-original" bash "tests/$task_name.sh"
  task_run "$task_name-sanitized-replay" env ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
    python3 "$task_review/build/run_original_sanitized.py" --name "$task_name" --output "$task_output/sanitized-originals"
done
for task_mode in normal asan; do
  task_flags=()
  if [[ "$task_mode" == asan ]]; then task_flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all); fi
  cc tests/linguas.c src/linguas.c -Isrc -O1 -g ${task_flags[@]+"${task_flags[@]}"} -o "$task_output/linguas-$task_mode"
  task_run "linguas-$task_mode" env ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 "$task_output/linguas-$task_mode"
done
task_run subtitle-title-fixtures env ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
  python3 "$task_review/subtitle-titles/run.py" --root "$PWD" --output "$task_output/subtitle-titles" --mode both
python3 "$task_review/build/verify_native.py" --output "$task_output"
