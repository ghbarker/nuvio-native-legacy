This update prioritizes Continue Watching while allowing collection rows and artwork to load at the same time. It also fixes the lifetime of addon URLs used during catalog loading.

Install this APK as **Update** over Nuvio TV/Mobile Test. It uses the same package and signing certificate as Logs14, so accounts and settings are retained.

Validation: [Android, macOS, sanitizers, 44 feature groups and 47 JVM/Media3 contracts](https://github.com/ghbarker/nuvio-native-legacy/actions/runs/38798595215).

Product source: `a570741e9482695c78158c52bbbb0e38b53b898a`. Test source: `da4306458d29a5e4c5f1fb89a24f956c79fe4568`.

1. Install the update.
2. Open Android App info for Nuvio TV/Mobile Test, select Force stop, then reopen the app.
3. Check that Continue Watching gets priority while the other Home rows and artwork also populate.
4. Immediately open a collection, scroll past the first rows, and open a title. Rows should load without a false “not installed” warning or a jump back to the top.
5. Switch profiles and check that Continue Watching belongs to the selected profile and artwork continues loading.
