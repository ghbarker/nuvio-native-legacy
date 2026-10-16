Update Nuvio TV/Mobile Test with the reviewed changes from PRs #431, #538, #540, #541, #543 and #546, plus the separate clock-focus fix for #495.

Install this APK over the existing test app to preserve its accounts and settings. The package and persistent signing certificate remain unchanged.

The build uses the combined public product source with its regression tests retained, and the same private local-log layer as Logs19. Final validation records the exact source, one fresh ARM32/ARM64 build, package, certificate, DEX/JNI, source workspace and 16 KB page checks. Historical individual-PR results are not reported as fresh combined results.

Remote test steps:

1. Install this APK over Nuvio TV/Mobile Test and choose Update. Accounts and settings stay in the same app.
2. On Home with the clock visible, press Up to the hero, then Up again. The clock should have a white outline without opening anything.
3. Press and release OK. The existing island/resume or Saved panel should open once; the title underneath should stay closed.
4. Close the panel, focus the clock again, then press Down or Back. Focus should return to the page without leaving it.
5. Repeat from Search, Settings and an opened collection, with any dialog or keyboard closed.
6. With a movie retained using the existing Return to Home option, use Up, OK and Resume to return to it without a Channel button.

Full-screen player controls keep their existing behavior. Holding Up never opens the clock; holding OK opens it once on release. Existing rating prompts retain their Up action until resolved.

The separate failover correction identified from the 23:47 log is not included in Logs20.
