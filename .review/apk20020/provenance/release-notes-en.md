APK20020 adds two changes to the accepted APK20019 test foundation:

- Subtitle Style gains **Outline color** for supported plain-text subtitles, with saved color, immediate updates and the existing Restore default behavior. Authored ASS styling and native/bitmap track limitations retain their existing policy. The color palettes and font picker adapt to phone orientation and interface scaling.
- **Settings → Home screen → Collections → Default collection view** offers Automatic, Grid, Timeline, Ranking, Saga, Decades, Mural and Showcase. Each collection can keep an explicit Display choice or use Default to inherit the active profile’s setting. Existing saved collection choices and single-catalog See all behavior are preserved.

The private test package keeps the existing application ID and signing certificate, with version code 20020 and version name 2.0.20-test-logs10. Update compatibility is a required actual-APK verification gate; no APK or CI pass is claimed by these preparation notes. Private local diagnostics remain outside product PRs.
