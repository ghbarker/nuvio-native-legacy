# Android TV and Mobile interface modes

The standard Android APK contains both interfaces. On a fresh installation, **Automatic** uses Android's TV classification: Android TV / Google TV devices use TV, while phones and tablets use Mobile. Screen size, orientation and connected input devices do not choose the interface.

Most users do not need to change a setting. To override detection, open **Settings > Appearance**, reveal the advanced options, and select **Interface mode > TV / Mobile**. Return to **Automatic** to follow the device classification again. This also supports Android boxes that report themselves as ordinary Android devices and phones used with the TV interface.

Existing explicit TV or Mobile choices are preserved on update. The selection is local to this device; account synchronization and profile changes do not replace it. Switching mode preserves the installed package, account, settings, and library. It updates the native canvas and Android window in place without restarting the Activity or creating a second SDL thread.

TV preserves the remote-oriented layouts, navigation and instructions, with a 1920×1080 logical canvas and landscape policy. Existing pointer and tap controls remain available, including the mode selector on a phone. Mobile follows the drawable aspect ratio with a logical short edge of 1080, supports touch scrolling and portrait/landscape browsing, protects keyboard input around cutouts, and switches full-screen playback to landscape. Returning to TV restores its layouts, window, buffer, keyboard policy and remote instructions.

The application ID remains `space.nuvio.nativelegacy`, and release builds use the existing release signing configuration. Updates signed with the same key preserve the installed app and its data. Debug builds use a debug signature and cannot replace a release-signed installation.

