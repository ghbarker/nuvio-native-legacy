#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p src/androidTest/assets/hls
# Continuous numbered pictures and tone; pre-generated so tests never transcode.
ffmpeg -hide_banner -loglevel error -y -f lavfi -i testsrc2=size=320x180:rate=25 \
  -f lavfi -i sine=frequency=440:sample_rate=48000 -t 180 \
  -vf 'drawtext=fontfile=/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf:text=%{n}:x=10:y=10:fontsize=28:fontcolor=white:box=1:boxcolor=black' \
  -c:v libx264 -preset ultrafast -pix_fmt yuv420p -g 50 -keyint_min 50 -sc_threshold 0 \
  -b:v 600k -c:a aac -b:a 64k -f hls -hls_time 2 -hls_list_size 0 \
  -hls_segment_filename src/androidTest/assets/hls/seg%03d.ts src/androidTest/assets/hls/fixture.m3u8
