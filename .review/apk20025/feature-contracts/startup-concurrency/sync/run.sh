#!/bin/bash
set -euo pipefail
source_root=${1:?source root}
sdk=${2:?typed declaration SDK}
fixture=$(cd "$(dirname "$0")" && pwd)
result_root=${NUVIO_SYNC_STARTUP_RESULTS:-$(mktemp -d "${TMPDIR:-/tmp}/nuvio-sync-startup.XXXXXXXX")}
mkdir -p "$result_root"
flags=(-std=gnu11 -D_GNU_SOURCE -O1 -g -ffunction-sections -fdata-sections -pthread -I"$source_root/src" -I"$source_root/tests" -I"$sdk")
gc=(-Wl,--gc-sections)
if [ "$(uname -s)" = Darwin ]; then gc=(-Wl,-dead_strip); fi
if [ "${NV_REVIEW_PRIVATE:-0}" = 1 ]; then flags+=(-DNV_TEST_LOCAL_LOGS=1); fi
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
if [ "${SANITIZE:-0}" = 1 ] && [ "$(uname -s)" != Darwin ]; then flags+=(-fno-pie); gc+=(-no-pie); fi
if [ "${EXPECT_BASELINE:-0}" = 1 ]; then flags+=(-DEXPECT_BASELINE=1); fi
"${CC:-cc}" "${flags[@]}" -include "$fixture/inject.h" -c "$source_root/src/sync.c" -o "$result_root/sync.o"
"${CC:-cc}" "${flags[@]}" "$result_root/sync.o" "$source_root/tests/stub_contapend.c" "$source_root/src/catordem.c" "$source_root/src/catordemcache.c" "$source_root/src/contacache.c" "$source_root/src/js.c" "$source_root/src/jsw.c" "$fixture/latch.c" "${gc[@]}" -o "$result_root/latch"
for scenario in concurrent stale thread-failure; do
  if [ "${EXPECT_BASELINE:-0}" = 1 ] && [ "$scenario" != concurrent ]; then continue; fi
  mkdir -p "$result_root/data-$scenario"
  NV_T_DIR="$result_root/data-$scenario" ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0:halt_on_error=1}" UBSAN_OPTIONS=halt_on_error=1 "$result_root/latch" "$scenario" > "$result_root/$scenario.log" 2>&1
  cat "$result_root/$scenario.log"
done
python3 - "$source_root" "$result_root" "${SANITIZE:-0}" "${EXPECT_BASELINE:-0}" <<'PY'
import hashlib, json, pathlib, sys
source, out = map(pathlib.Path, sys.argv[1:3])
record = {'passed': True, 'actual_sync': str(source/'src/sync.c'), 'source_sha256': hashlib.sha256((source/'src/sync.c').read_bytes()).hexdigest(), 'sanitize': bool(int(sys.argv[3])), 'baseline': bool(int(sys.argv[4])), 'scenarios': {p.stem: hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(out.glob('*.log'))}}
(out/'receipt.json').write_text(json.dumps(record, indent=2)+'\n')
print('sync-startup: PASS', json.dumps(record, sort_keys=True))
PY
