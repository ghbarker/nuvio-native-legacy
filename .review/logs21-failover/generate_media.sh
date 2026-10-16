#!/bin/bash
set -eu
[ "$#" -eq 2 ] || { echo 'Usage: generate_media.sh MEDIA_DIR PROOF_DIR' >&2; exit 2; }
exec python3 "$(dirname "$0")/generate_media.py" --output "$1" --proof "$2"
