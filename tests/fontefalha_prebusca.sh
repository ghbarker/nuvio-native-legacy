#!/bin/bash
set -eu
cd "$(dirname "$0")/.."
python3 tests/fontefalha_prebusca.py "$@"
