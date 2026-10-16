#!/bin/bash
# The Android installer applies stricter rules to package-visibility queries.
set -eu
cd "$(dirname "$0")/.."
python3 tests/android_manifest_queries.py "$@"
