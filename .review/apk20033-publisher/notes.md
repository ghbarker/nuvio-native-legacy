Logs23 adds private diagnostics for season and episode rows that appear only after leaving a title and reopening it. It records bounded update/draw state to help identify the cause; this build does not claim to fix that display issue.

Install over Nuvio TV/Mobile Test using **Update** to keep accounts and settings. The package and signing key are unchanged. This build retains the existing playback, episode identity and logging features.

1. Update the existing test app, then open an affected series.
2. Wait 10 seconds without using Back. Check whether the season buttons and episode cards appear.
3. Take a screenshot while the rows are missing, if that occurs.
4. Use Back, reopen the same series and wait 10 seconds. Compare the rows with the first opening.
5. Download a fresh log immediately after this comparison.

The added diagnostics use title/source hashes and layout state, with at most 32 changed records per opening and a 250ms sampling interval. No raw title/source URLs, account identifiers or auth are added to these records.
