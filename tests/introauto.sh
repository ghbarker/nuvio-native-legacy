#!/bin/bash
set -eu
cd "$(dirname "$0")/.."
flags=()
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
for test in introauto video_auto; do
  cc ${flags[@]+"${flags[@]}"} "tests/$test.c" -Isrc -O1 -g -Wall -Wextra -o "/tmp/nuvio-$test-tests"
  "/tmp/nuvio-$test-tests"
done
