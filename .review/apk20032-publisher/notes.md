Logs22 keeps Trakt history, Resume and Next aligned across different episode numbering when catalog metadata supplies stable episode IDs ([#512](https://github.com/iqui27/nuvio-native-legacy/issues/512)). Typed series lookups avoid matching a series to the wrong movie ([#524](https://github.com/iqui27/nuvio-native-legacy/issues/524)). Fixes a preload timeout that could expire immediately while waiting for subtitle headers.

Install over the existing Nuvio TV/Mobile Test app using **Update**. The package and signing key stay the same, preserving accounts and settings.

Automated validation passed: Android ARM32/ARM64 builds and APK checks;40 macOS test groups with130 executions plus full TV/touch links; and four native-provider failover scenarios on the unchanged APK in an Android emulator, including delayed subtitle-header loading.

1. Update the existing test app and open an affected series.
2. Wait for its episode list to finish loading, then check the title, season and episode names.
3. Mark an episode watched, leave the title and reopen it to verify the same episode remains marked.
4. Play that series, then use Resume and Next to check the selected episode against the visible list.
5. Keep subtitles enabled when retrying a failed source. Download a fresh log immediately if episode selection or failover behaves incorrectly.
