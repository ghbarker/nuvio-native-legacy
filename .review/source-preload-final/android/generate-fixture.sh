#!/usr/bin/env bash
set -euo pipefail
here="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
mkdir -p "$here/probe/src/androidTest/assets"
ffmpeg -hide_banner -loglevel error -f lavfi -i testsrc2=size=640x360:rate=24 -f lavfi -i sine=frequency=440:sample_rate=48000 -t 20 -c:v libx264 -preset ultrafast -pix_fmt yuv420p -b:v 3M -minrate 3M -maxrate 3M -bufsize 3M -x264-params nal-hrd=cbr -g 24 -c:a aac -b:a 96k -movflags +faststart -y "$here/probe/src/androidTest/assets/fixture.mp4"
ffmpeg -hide_banner -loglevel error -i "$here/probe/src/androidTest/assets/fixture.mp4" -c copy -hls_time 2 -hls_list_size 0 -hls_segment_filename "$here/probe/src/androidTest/assets/segment%02d.ts" -y "$here/probe/src/androidTest/assets/fixture.m3u8"
# A 6 s HLS chunk exceeds the 16 MB preload ceiling. No high-bitrate decoder
# is started by the budget test: it validates actual sample-queue allocations.
ffmpeg -hide_banner -loglevel error -f lavfi -i testsrc2=size=640x360:rate=24 -f lavfi -i sine=frequency=440:sample_rate=48000 -t 10 -c:v libx264 -preset ultrafast -pix_fmt yuv420p -b:v 32M -minrate 32M -maxrate 32M -bufsize 32M -x264-params nal-hrd=cbr -g 24 -c:a aac -b:a 96k -hls_time 6 -hls_list_size 0 -hls_segment_filename "$here/probe/src/androidTest/assets/budget%02d.ts" -y "$here/probe/src/androidTest/assets/budget.m3u8"
