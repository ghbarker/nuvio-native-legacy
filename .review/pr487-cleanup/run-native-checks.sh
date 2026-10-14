#!/bin/bash
set -uo pipefail
task_root="${1:-$PWD}"
cd "$task_root"
task_support="${NUVIO_REVIEW_SUPPORT:-.review/pr487-cleanup}"
mkdir -p build/review
export NV_TMP="${RUNNER_TEMP:-${TMPDIR:-/tmp}}/nuvio-pr487-cleanup"
mkdir -p "$NV_TMP"
failed=0
run_check() {
  local name="$1"
  shift
  if "$@" 2>&1 | tee "build/review/$name.log"; then
    echo "$name PASS" >> build/review/results.txt
  else
    echo "$name FAIL" >> build/review/results.txt
    failed=1
  fi
}
: > build/review/results.txt
# These fixtures retain only the parser / row renderer and do not load SDL.
for sanitize in 0 1; do
  export SANITIZE="$sanitize"
  export ASAN_OPTIONS=detect_leaks=0
  run_check "metadata-parser-sanitize-$sanitize" python3 "$task_support/run-parser-checks.py" --root "$PWD" --output-dir "$PWD/build/review" --log-prefix "metadata-parser-sanitize-$sanitize-fixture"
  run_check "metadata-ui-tv-touch-sanitize-$sanitize" bash "$task_support/run-ui-checks.sh" --root "$PWD"
done
# Homebrew SDL cannot start under ASan on macOS runners; application regressions
# use their normal build, including the full-source Android-player link test.
export SANITIZE=0
for name in stream_parser streamfit_parser fontecache_vod i18n player_android_retomada; do
  run_check "$name" bash "tests/$name.sh"
done
cat build/review/results.txt
exit "$failed"
