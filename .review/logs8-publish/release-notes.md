# Nuvio TV/Mobile Test — Logs8 update

**Download:** [Nuvio-TV-Mobile-Test-Logs8.apk](https://github.com/ghbarker/nuvio-native-legacy/releases/download/tv-mobile-test-logs-v8/Nuvio-TV-Mobile-Test-Logs8.apk)

This build includes the Mobile Settings fixes, shared source exclusions before provider link limits, an audio-language filter that follows your preference, clearer cached-source indicators, artwork improvements and earlier Continue Watching updates. The accepted preloading, profile isolation, subtitle track-name and episode-source fixes are still included.

Your save failure without a floating video is still unconfirmed. This build makes the existing save-error message visible and adds local diagnostic events so we can identify what happens if it persists.

## First: confirm Settings save correctly

1. Open **Settings → Home screen → Home layout**. Tap **Dynamic**; its full name is **Dynamic (Apple TV style)**.
2. Tap **Confirm** at the bottom. Expected: the editor closes and the row keeps Dynamic. Highlighting a choice only changes the pending selection.
3. Return to Home, then reopen **Settings → Home screen → Home layout**. Expected: Dynamic remains selected and Home uses that layout. Close and reopen the app once to confirm it persists.
4. Choose a different layout, tap **Cancel**, and reopen the row. Expected: Dynamic remains saved.
5. If it still fails, note whether **Could not save. The previous value was kept.** appears. Open **Settings → About and help → Download log**, save the file using Android's file picker and share it here. **View the log on screen** is beside it if you want to inspect it first.

## Short checks during normal use

1. **Exclusions and provider budget:** open **Settings → Sources and add-ons → Source filters → Resolution**. Temporarily check **2160p**; these checkboxes save immediately. Reopen Sources for a title with known 2160p results. Expected: those results disappear; uncheck it afterward. In **Provider order and limits**, confirm your existing order and link limits remain saved. Excluded links do not count toward the limit: a limit of 3 means up to 3 different eligible links for that provider across addons before advancing on failure. No provider-failure test is needed.
2. **Audio language:** with **Settings → Languages and subtitles → Audio language** set to English, open Sources and enable **Audio: EN**. Expected: results declaring English audio remain; non-English-only and undeclared-audio results disappear. Turning the chip off restores them. Subtitle languages alone do not count as audio metadata.
3. **Artwork:** open a new title through Recommendations, then another through a cast member's Filmography. Expected: a temporary poster upgrades when a genuine backdrop arrives, and an available logo appears. A title without supplied artwork keeps its poster/title fallback; already selected custom artwork stays selected.
4. **Cinema opening:** choose **Settings → Playback → Video opening screen → Cinema (clear artwork)** and tap **Confirm**. If a loading screen appears during normal playback, expect the selected title artwork without blur. **Cinema** keeps the blurred version; restore your preferred choice afterward.
5. **Cached sources:** in Sources, look for the lightning icon beside results already confirmed ready by AIOStreams or your native provider. Unknown or uncached results should not receive the positive icon; the build does not add readiness probes.
6. **Continue Watching:** reopen the app with your existing account. Expected: the correct resumable titles can appear while the remaining Home content finishes loading. Opening speed still depends on the services responding.

Keep your existing accounts and normal playback settings for these checks. There is no need to remove accounts, force provider failures, spend Premiumize quota or work through every Settings option.
