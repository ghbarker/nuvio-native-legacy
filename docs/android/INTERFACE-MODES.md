# Android TV and Mobile interface modes

The standard Android APK contains both interfaces. It starts in TV mode; choose **Settings > Appearance > Interface mode > Mobile** to use phone layouts and direct touch scrolling. Choose TV again to restore the remote-oriented layout. Both modes support the settings control by touch and by remote.

The selection is stored as `interfaceModoLocal` in this device's settings. Account synchronization and profile changes do not replace it. Switching mode preserves the installed package, account, settings, and library. It updates the native canvas and Android window in place; it does not restart the Activity or create a second SDL thread.

TV keeps the original 1920×1080 logical canvas and landscape policy. Mobile follows the drawable aspect ratio with a logical short edge of 1080, supports portrait/landscape browsing, protects keyboard input around cutouts, and switches full-screen playback to landscape. Returning to TV restores its window, buffer, and keyboard policy.

`NV_TOUCH_UI` is the compiled Android interface capability. `NV_TOUCH_PREVIEW` only identifies the isolated `.touch` test package: its update restrictions and test configuration must not affect the standard app. The production application ID and release signing configuration remain `space.nuvio.nativelegacy` and the existing release key. A debug test APK has a debug signature and does not replace a release-signed installation.

The mobile work is based on upstream 2.0.4 (`db6ae4ee`), preserving upstream subtitle synchronization, source recovery, guide pagination, and Spotlight fixes. The earlier touch preview package remains available for testing; the feature does not require shipping a separate phone app.

Validation includes native TV/Mobile canvas round trips, device-local setting persistence and account isolation, Android window/orientation/keyboard policy, existing touch regressions, and emulator smoke checks. Phone and TV hardware checks should cover mode switching, restart persistence, remote navigation, touch scrolling, keyboard dismissal, portrait/landscape browsing, and full-screen playback.

