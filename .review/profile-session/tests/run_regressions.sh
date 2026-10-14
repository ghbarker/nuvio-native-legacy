#!/bin/bash
# Run review-only fixtures against the current checkout; all HTTP is mocked.
# Keep empty-array expansions compatible with the macOS system Bash 3.2.
set -euo pipefail
profile_source="$PWD"
profile_review="${NUVIO_PROFILE_REVIEW:-$profile_source/.review/profile-session}"
profile_results="${NUVIO_PROFILE_RESULTS:-$profile_source/build/review/profile-session}"
mkdir -p "$profile_results"
: > "$profile_results/results.txt"
export ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}"
export RUN_ASAN="${SANITIZE:-0}"
profile_failed=0
for profile_suite in core data ui provider native-failover live-tv trackers; do
  profile_runner="$profile_review/tests/$profile_suite/run.sh"
  if [[ ! -f "$profile_runner" ]]; then
    echo "Missing frozen review runner: $profile_suite" >&2
    exit 1
  fi
  if bash "$profile_runner" "$profile_source" 2>&1 | tee "$profile_results/$profile_suite.log"; then
    echo "$profile_suite PASS" >> "$profile_results/results.txt"
  else
    echo "$profile_suite FAIL" >> "$profile_results/results.txt"
    profile_failed=1
  fi
done
if bash "$profile_review/tests/playback/run_playback.sh" 2>&1 | tee "$profile_results/playback.log"; then
  echo 'playback PASS' >> "$profile_results/results.txt"
else
  echo 'playback FAIL' >> "$profile_results/results.txt"
  profile_failed=1
fi
for profile_tracker_suite in existing auth-owned auth-existing; do
  if bash "$profile_review/tests/trackers/$profile_tracker_suite.sh" "$profile_source" 2>&1 | tee "$profile_results/tracker-$profile_tracker_suite.log"; then
    echo "$profile_tracker_suite PASS" >> "$profile_results/results.txt"
  else
    echo "$profile_tracker_suite FAIL" >> "$profile_results/results.txt"
    profile_failed=1
  fi
done
if [[ "$(uname -s)" == Darwin ]]; then
  python3 "$profile_review/build/prepare_http_tests.py"
  for profile_server in jellyfin plex emby; do
    if bash "tests/$profile_server.sh" 2>&1 | tee "$profile_results/server-$profile_server.log"; then
      echo "$profile_server HTTP PASS" >> "$profile_results/results.txt"
    else
      echo "$profile_server HTTP FAIL" >> "$profile_results/results.txt"
      profile_failed=1
    fi
  done
fi
cat "$profile_results/results.txt"
exit "$profile_failed"
