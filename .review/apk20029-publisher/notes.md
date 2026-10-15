Logs19 adds metadata addon cast portraits for #528. It reads names, character names and photo URLs from the metadata addon before optional TMDB enrichment. If TMDB is unavailable, those portraits remain available. All other Logs18 features and private log controls are retained.

Install over Nuvio TV/Mobile Test and choose Update to retain accounts and settings.

To test:
1. Install the APK and open the profile you normally use. Confirm your accounts are still present.
2. If you have AIOMetadata or another metadata addon that supplies cast photos, enable Prefer external metadata in Settings.
3. Open a movie or series supplied by that addon and scroll to Cast. Check the portraits and character names match the correct people. Repeat with a second title.
4. If portraits remain initials, use View logs or Download logs and share the result with the affected title.

You do not need to change DNS, use a VPN, or block TMDB to run the display check. The automated feature checks exercise failed TMDB responses separately.

Validation: [fresh complete ARM32/ARM64 Android build](https://github.com/ghbarker/nuvio-native-legacy/actions/runs/38824587500). The actual APK passed package, persistent signing key, JNI/DEX and 16 KB checks.

SHA-256: `bc3f99cad26cff7c6b58ffbf4769b6f10fff2c7593a4d0025d77f5a0c92c49e0`
