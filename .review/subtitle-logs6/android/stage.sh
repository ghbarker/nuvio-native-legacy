#!/usr/bin/env bash
set -euo pipefail
root="${1:-/workspace/nuvio-source-preload}"
here="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
mkdir -p "$here/probe/src/player/space/nuvio/nativelegacy"
for f in TestLocalLogs NvPlayer AudioSyncSink AudioSyncTap CacheMidia CacheSessao GanhoAudioProcessor GanhoMath ParaleloDataSource PassivoMedidor RecriaHdr; do
    cp "$root/android/app/src/main/java/space/nuvio/nativelegacy/$f.kt" "$here/probe/src/player/space/nuvio/nativelegacy/$f.kt"
done
sha256sum "$root/android/app/src/main/java/space/nuvio/nativelegacy/NvPlayer.kt" > "$here/player-source.sha256"
if [[ -f "$here/startup-diagnostics/StartupContractTest.kt" ]]; then
    cp "$here/startup-diagnostics/StartupContractTest.kt" "$here/probe/src/androidTest/java/space/nuvio/preloadtest/StartupContractTest.kt"
fi
