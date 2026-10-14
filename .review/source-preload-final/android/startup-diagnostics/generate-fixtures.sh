#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
    printf 'Usage: %s <staged-workdir>\n' "$0" >&2
    exit 2
fi
workdir="$(cd -- "$1" && pwd)"
assets="$workdir/probe/src/androidTest/assets"
for tool in ffmpeg ffprobe python3; do
    command -v "$tool" >/dev/null || { printf 'Missing tool: %s\n' "$tool" >&2; exit 1; }
done
if [[ ! -f "$workdir/startup-stage.json" || ! -f "$workdir/generate-fixture.sh" ]]; then
    printf 'Run startup-diagnostics/stage.sh into an isolated working directory first.\n' >&2
    exit 1
fi

# Preserve all original fixtures for the unchanged eight-test class.
bash "$workdir/generate-fixture.sh"

# One video stream and two independently encoded audio tracks. English is the
# first/default track; Portuguese has a distinguishable 880 Hz tone.
ffmpeg -hide_banner -loglevel error \
    -i "$assets/fixture.mp4" \
    -f lavfi -i sine=frequency=440:sample_rate=48000:duration=20 \
    -f lavfi -i sine=frequency=880:sample_rate=48000:duration=20 \
    -map 0:v:0 -map 1:a:0 -map 2:a:0 -t 20 \
    -c:v copy -c:a aac -b:a 96k \
    -metadata:s:a:0 language=eng -metadata:s:a:0 title=English \
    -metadata:s:a:1 language=por -metadata:s:a:1 title=Portuguese \
    -disposition:a:0 default -disposition:a:1 0 \
    -y "$assets/startup-two-audio.mkv"

# Separate MPEG-TS audio renditions give the player a real HLS default-language
# choice before playback; the video samples remain shared between renditions.
ffmpeg -hide_banner -loglevel error -i "$assets/startup-two-audio.mkv" \
    -map 0:v:0 -c copy -an -hls_time 2 -hls_list_size 0 -hls_playlist_type vod \
    -hls_segment_filename "$assets/startup-video%02d.ts" -y "$assets/startup-video.m3u8"
for rendition in eng por; do
    audio_index=0
    [[ "$rendition" == por ]] && audio_index=1
    ffmpeg -hide_banner -loglevel error -i "$assets/startup-two-audio.mkv" \
        -map "0:a:$audio_index" -c copy -vn \
        -hls_time 2 -hls_list_size 0 -hls_playlist_type vod \
        -hls_segment_filename "$assets/startup-${rendition}%02d.ts" -y "$assets/startup-${rendition}.m3u8"
done

# 4 seconds at 80 Mbps produces about 40 MB; 8-16 MiB is reached before the
# 2.5-second time target. This short fixture bounds the diagnostic server heap.
ffmpeg -hide_banner -loglevel error \
    -f lavfi -i testsrc2=size=640x360:rate=24 \
    -f lavfi -i sine=frequency=440:sample_rate=48000:duration=4 \
    -t 4 -map 0:v:0 -map 1:a:0 \
    -c:v libx264 -preset ultrafast -pix_fmt yuv420p \
    -b:v 80M -minrate 80M -maxrate 80M -bufsize 80M \
    -x264-params nal-hrd=cbr -g 24 -c:a aac -b:a 96k \
    -movflags +faststart -y "$assets/startup-budget.mp4"

# Derive the RFC 6381 AVC codec from the actual avcC bytes rather than guessing
# the profile/level. Validate fixtures before the Android build packages them.
python3 - "$assets" <<'PY'
import hashlib
import json
from pathlib import Path
import subprocess
import sys

assets = Path(sys.argv[1])
def probe(name):
    return json.loads(subprocess.check_output([
        "ffprobe", "-v", "error", "-show_streams", "-show_format", "-show_data",
        "-of", "json", str(assets / name)
    ], text=True))

standard = probe("fixture.mp4")
video = next(s for s in standard["streams"] if s["codec_type"] == "video")
hex_data = "".join(line.split(":", 1)[1].split("  ", 1)[0].replace(" ", "")
                   for line in video["extradata"].splitlines() if ":" in line)
avcc = bytes.fromhex(hex_data)
assert len(avcc) >= 4 and avcc[0] == 1, "Expected an AVCDecoderConfigurationRecord"
codec = "avc1." + avcc[1:4].hex()
(assets / "startup-master.m3u8").write_text(
    '#EXTM3U\n#EXT-X-VERSION:3\n'
    '#EXT-X-MEDIA:TYPE=AUDIO,GROUP-ID="startup-audio",NAME="English",'
    'LANGUAGE="en",DEFAULT=YES,AUTOSELECT=YES,URI="startup-eng.m3u8"\n'
    '#EXT-X-MEDIA:TYPE=AUDIO,GROUP-ID="startup-audio",NAME="Portuguese",'
    'LANGUAGE="pt",DEFAULT=NO,AUTOSELECT=YES,URI="startup-por.m3u8"\n'
    f'#EXT-X-STREAM-INF:BANDWIDTH=3200000,AVERAGE-BANDWIDTH=3096000,'
    f'CODECS="{codec},mp4a.40.2",RESOLUTION=640x360,FRAME-RATE=24.000,'
    'AUDIO="startup-audio"\nstartup-video.m3u8\n'
)

multi = probe("startup-two-audio.mkv")
audio = [s for s in multi["streams"] if s["codec_type"] == "audio"]
assert len(audio) == 2
assert [s.get("tags", {}).get("language") for s in audio] == ["eng", "por"]
assert [s["disposition"]["default"] for s in audio] == [1, 0]
assert all(s["codec_name"] == "aac" and s["sample_rate"] == "48000" for s in audio)
assert 19.9 <= float(multi["format"]["duration"]) <= 20.1

for name in ["startup-video.m3u8", "startup-eng.m3u8", "startup-por.m3u8"]:
    playlist = (assets / name).read_text()
    assert "#EXT-X-ENDLIST" in playlist
    segments = [line for line in playlist.splitlines() if line and not line.startswith("#")]
    assert segments and all((assets / segment).is_file() for segment in segments)
    stream_probe = probe(name)
    expected = "video" if name == "startup-video.m3u8" else "audio"
    assert all(s["codec_type"] == expected for s in stream_probe["streams"])

master_audio = [s for s in probe("startup-master.m3u8")["streams"] if s["codec_type"] == "audio"]
assert [s.get("tags", {}).get("language") for s in master_audio] == ["en", "pt"]
assert [s["disposition"]["default"] for s in master_audio] == [1, 0]
assert all(s["codec_name"] == "aac" and s["sample_rate"] == "48000" for s in master_audio)

budget = probe("startup-budget.mp4")
budget_video = next(s for s in budget["streams"] if s["codec_type"] == "video")
duration = float(budget["format"]["duration"])
size = (assets / "startup-budget.mp4").stat().st_size
bitrate = int(budget_video["bit_rate"])
assert budget_video["codec_name"] == "h264"
assert (budget_video["width"], budget_video["height"]) == (640, 360)
assert budget_video["r_frame_rate"] == "24/1"
assert 3.9 <= duration <= 4.1
assert 70_000_000 <= bitrate <= 90_000_000, f"Unexpected budget bitrate: {bitrate}"
assert 34_000_000 <= size <= 45_000_000, f"Unexpected budget size: {size}"
assert 16 * 1024 * 1024 * 8 / bitrate < 2.5

files = [assets / "fixture.mp4"] + sorted(file for file in assets.glob("startup-*")
                                         if file.name != "startup-fixtures.json")
record = {
    "format": 1,
    "generator": "startup-diagnostics/generate-fixtures.sh",
    "avc_codec": codec,
    "two_audio": [{"language": s["tags"]["language"],
                   "default": s["disposition"]["default"],
                   "sample_rate": int(s["sample_rate"])} for s in audio],
    "budget": {"duration_s": duration, "video_bitrate_bps": bitrate, "bytes": size,
               "seconds_for_16_mib_at_video_bitrate": 16 * 1024 * 1024 * 8 / bitrate},
    "files": [{"path": file.name, "bytes": file.stat().st_size,
               "sha256": hashlib.sha256(file.read_bytes()).hexdigest()} for file in files],
}
(assets / "startup-fixtures.json").write_text(json.dumps(record, indent=2) + "\n")
print(f"Validated startup fixtures: {codec}, English default / Portuguese alternate; "
      f"budget {size:,} bytes at {bitrate:,} video bps")
PY
