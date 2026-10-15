#!/bin/bash
# Run the real profile session gate and production addon/cache workers offline.
set -euo pipefail
task_root="$PWD"
task_review="${NUVIO_PROFILE_REVIEW:-$task_root/.review/profile-session}"
task_out="${NUVIO_PROFILE_RESULTS:-$task_root/build/review/profile-session}"
mkdir -p "$task_out"
task_flags=(-O1 -g -std=gnu11 -ffunction-sections -fdata-sections -Isrc -I. -Wno-deprecated-declarations -Wno-macro-redefined)
task_link=(-lpthread -lm)
if [[ "$(uname -s)" == Darwin ]]; then
  task_flags+=(-I/opt/homebrew/include -I/opt/homebrew/include/SDL2)
  task_link+=(-Wl,-dead_strip)
else
  task_link+=(-Wl,--gc-sections)
  if [[ -n "${NUVIO_PROFILE_TEST_SDK:-}" ]]; then
    task_flags+=("-I$NUVIO_PROFILE_TEST_SDK")
    if [[ -n "${NUVIO_PROFILE_TEST_SDK_EXTRA:-}" ]]; then task_flags+=("-I$NUVIO_PROFILE_TEST_SDK_EXTRA"); fi
  elif command -v sdl2-config >/dev/null; then
    read -r -a task_sdl_flags <<< "$(sdl2-config --cflags)"
    task_flags+=("${task_sdl_flags[@]}")
  fi
fi
if [[ "${SANITIZE:-0}" == 1 ]]; then
  task_flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer)
  task_link+=(-fsanitize=address,undefined)
fi
task_retained_sources=()
# Latest upstream ships native failover without the preview's custom filters.
# Detect it by default; explicit modes keep older standalone/combined reviews usable.
task_native=${NUVIO_PROFILE_NATIVE_FAILOVER:-auto}
if [[ "$task_native" == auto ]]; then
  task_native=0
  if [[ -f src/fontefalha.c ]]; then task_native=1; fi
fi
if [[ "${NUVIO_PROFILE_COMBINED:-0}" == 1 ]]; then task_native=1; fi
if [[ "$task_native" == 1 ]]; then
  task_retained_sources+=(src/fontefalha.c)
  task_flags+=(-DNUVIO_PROFILE_NATIVE_FAILOVER)
fi
if [[ "${NUVIO_PROFILE_COMBINED:-0}" == 1 ]]; then
  task_retained_sources+=(src/fontefiltro.c)
  task_flags+=(-DNUVIO_PROFILE_COMBINED)
fi
task_fixture="${NUVIO_PROFILE_PLAYBACK_FIXTURE:-$task_review/tests/playback/profile_playback.c}"
cc "${task_flags[@]}" "$task_fixture" \
  src/addons.c src/addonstats.c src/fontesmortas.c src/perfis.c \
  src/linguas.c src/js.c src/legextras.c "${task_link[@]}" \
  ${task_retained_sources[@]+"${task_retained_sources[@]}"} \
  -o "$task_out/playback-profile"
"$task_out/playback-profile" | tee "$task_out/playback-profile.log"
task_channel_fixture="${NUVIO_PROFILE_CHANNEL_FIXTURE:-$task_review/tests/playback/channel_children.c}"
cc "${task_flags[@]}" "$task_channel_fixture" \
  src/fontesmortas.c src/perfis.c "${task_link[@]}" \
  ${task_retained_sources[@]+"${task_retained_sources[@]}"} \
  -o "$task_out/channel-children"
"$task_out/channel-children" | tee "$task_out/channel-children.log"
task_profile_fixture="${NUVIO_PROFILE_FETCH_FIXTURE:-$task_review/tests/playback/profile_fetch.c}"
cc "${task_flags[@]}" "$task_profile_fixture" \
  src/perfis.c src/js.c src/jsw.c "${task_link[@]}" \
  -o "$task_out/profile-fetch"
"$task_out/profile-fetch" | tee "$task_out/profile-fetch.log"
