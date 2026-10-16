Update for Nuvio TV/Mobile Test, using the same package and stable signing key as Logs20. Installing over a matching test build keeps accounts and settings.

Use Up at the top of a browsing screen to highlight the clock, then OK to open its existing island. Channel and touch controls keep their existing behavior.

Fixes failover consuming another link attempt while that source is still preparing subtitles, because an error from the previous video remained active. Errors from the current source and startup timeouts still advance normally. Includes the other changes from Logs20 and the existing optional local log controls.


Failover test on a phone:

1. Install this APK over Nuvio TV/Mobile Test and choose Update.
2. Enable provider failover, put TorBox before Premiumize in Provider order and limits, and set TorBox to 3 links.
3. Keep subtitles enabled and reopen the title that previously failed, using automatic source selection.
4. Each replacement must get its own opening result or startup timeout. Premiumize should follow only after the three allowed TorBox candidates genuinely fail; playback stops advancing as soon as a candidate works.
5. Download the log after the attempt so the candidate openings and provider transition can be checked.

The clock change is also included: Up beyond the browsing top highlights it; short OK opens its existing controls; Down or Back returns to the page. Emulator results are reported separately from this source-bound Android build.
