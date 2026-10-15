# Nuvio TV/Mobile Test — Logs9 update

**Download:** [Nuvio-TV-Mobile-Test-Logs9.apk](https://github.com/ghbarker/nuvio-native-legacy/releases/download/tv-mobile-test-logs-v9/Nuvio-TV-Mobile-Test-Logs9.apk)

This update installs over **Nuvio TV/Mobile Test**, keeping its accounts and settings.

- **Player controls:** Previous and Next work for series even with automatic next episode Off. Reload reopens the same source, keeping position, pause and track choices. Mobile portrait uses two rows of larger controls.
- **Synopsis text:** **Settings → Appearance → Synopsis text size** changes descriptions separately from the rest of the interface. **More options → Read synopsis** opens the full title or episode description.
- **Cached sources:** Sources show **Cached** or **Not cached** when that status is known. Unknown metadata stays unlabeled. The **Cached** chip shows only confirmed cached sources; this adds no readiness probes.

Version **2.0.19-test-logs9** · Android versionCode **20019**.

Subtitle outline colors (#475) and the global collection view (#362) are reserved for the following update.

## Quick checks

1. With **Play the next episode automatically** Off, open a series and use **Next episode**, then **Previous episode**. They should open adjacent available episodes; the appropriate button is disabled at each endpoint.
2. Use **Reload** midway through playback, then while paused. The same source should return to the same position and retain its pause and audio/subtitle choices.
3. Open **Settings → Appearance → Synopsis text size → Large**, tap **Confirm**, and reopen the row. The choice should stay saved. On a title, open **More options → Read synopsis**, scroll, and use **Close** to return.
4. Open **Sources** and enable **Cached**. Only streams explicitly confirmed cached should remain. Turning the chip Off restores the normal list without changing provider order or link limits.
