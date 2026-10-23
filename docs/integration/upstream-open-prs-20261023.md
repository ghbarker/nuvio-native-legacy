# Upstream and open-PR integration — 2026-10-23

Base: `1755d80cb8f33603ed4dbcd57dea49e5feded80d` (`iqui27/master`).
Branch: `integration/upstream-open-prs-20261023`.

This is a combined integration branch. The individual upstream PR branches are unchanged.
PR #615 (skip-marker fallback) is already merged upstream and is included through the base.

## Included open PRs

| PR | Change | Exact head |
| --- | --- | --- |
| [#573](https://github.com/iqui27/nuvio-native-legacy/pull/573) | Fonts: import TTF/OTF for subtitles and interface across Android and TVs | `385740c23aa9b0cd5825b236f45f10066802b6ea` |
| [#575](https://github.com/iqui27/nuvio-native-legacy/pull/575) | Sources: add persistent cached/uncached filtering | `a8ec86ac92c1379d9a9a1dc4930f354d399a7636` |
| [#576](https://github.com/iqui27/nuvio-native-legacy/pull/576) | Episodes: keep canonical identity after metadata updates | `a3c105f1cf200edf6fadaa008e0941bb786f4fed` |
| [#598](https://github.com/iqui27/nuvio-native-legacy/pull/598) | Details: show prepared-source specifications below Play | `2a74532a7d22a4695753d601d84a20de2ecbfbd3` |
| [#609](https://github.com/iqui27/nuvio-native-legacy/pull/609) | Backup: add encrypted Google Drive backup and restore for local settings | `5c532ca4dffb4eecbaf2b7fa132ec8623099d8f1` |
| [#611](https://github.com/iqui27/nuvio-native-legacy/pull/611) | Sources: match audio-language filters against release labels | `651bd8e2e73f4a6677b8aa11f70220fc30054787` |
| [#614](https://github.com/iqui27/nuvio-native-legacy/pull/614) | Episodes: publish season tabs with episode cards | `e9a56a4f06c270d27f1362244f670339d8db95ae` |
| [#619](https://github.com/iqui27/nuvio-native-legacy/pull/619) | Sports: add league browsing, favorites and IPTV event matching | `d98dab565a883b5557583c70ea40633e81b34d0e` |
| [#644](https://github.com/iqui27/nuvio-native-legacy/pull/644) | Friends: connect account profiles and fix Home card taps | `5157d7a1febafa25e0e2a95c356c25d4399d47d6` |

## Integration decisions

- Retain upstream's existing font import and interface renderer; add the remaining TV font-URL and WGT persistence support.
- Preserve upstream source recovery and the current source-list builder while applying cache and audio-language filters.
- Append new Settings entries without shifting upstream's positional option IDs, keys, or defaults.
- Keep upstream's Lite Android build option alongside Google authorization and backup encryption wiring.
- Preserve current profile ownership rules. Journal explicitly selected hardware values into both device and profile files where required, so a profile reload cannot undo an opted-in restore. An opted-out restore retains destination hardware choices.
- Back up the persisted language ID together with automatic/manual mode and its source; the picker index is not the stored language ID.
- Preserve Sports source sheets, cancellation, artwork-quality settings, sport defaults, stable ordering and the approved Panthers colour correction. Use the existing translated ended-event label.
- Keep all translation keys from both sides. Lint exemptions cover only reviewed filenames/internal identifiers; no untranslated UI labels are exempted.
- Isolate episode-selection fixtures from the persistent metadata cache and retain Kaiju, One Piece, Specials, and Game of Thrones season regressions.

## Validation

- Full C source syntax checks passed for touch and remote configurations, including Drive UI/encryption-enabled configurations. Final changed sources were checked again after integration corrections.
- 24 focused Linux shell suites passed: fonts/URL import, source cache filters/failover, episode identities/season publication, prepared-source metadata/layout, backup components/transport, Sports and account-profile Friends.
- 10 focused ASan/UBSan suites passed.
- Real full-application Friends touch fixture compiled/linked and passed across the existing styles and portrait/landscape layouts.
- Drive backup UI fixture compiled/linked and passed.
- Full native-owner backup integration passed: 206 local choices round-trip; 78 account-managed choices remain excluded; connection merging, service-login preservation, repeated restore, stale-session rejection, hardware-only restore, and opt-in/out behavior passed.
- TV font-URL registry fixture passed (111 checks per fixture font).
- 22 social-server identity/JWKS tests passed locally with SQLite and mocked network responses.
- Sports packaged-art checks passed for all 16 sports, including Low/High backgrounds and distinct fallback symbols.
- Translation lint: zero findings, matching upstream's zero-finding baseline. `git diff --check` passed.

## Limits

No new Android APK, LG IPK, Samsung TPK or WGT package was built for this branch, and no physical-device verification was performed for this integration. Host checks are not device-package builds.

Production Google OAuth registration remains release configuration. The matching Friends server implementation is included as source, but this task does not deploy the service.

Test-app branding, local log-export packaging, signing configuration and preview CI workflows were not added to this product integration branch.
