PR 487 review-only verification

Publish this review commit directly on the final three-file product commit,
which must directly descend from canonical upstream
17b7769dac4273557e1b07f5fb8734df91dd718f.
Replace @FEATURE@ in verify-source.py with that final product commit before
publication, then regenerate the local upload-plan.json.

The workflow runs on review/source-audio-subtitle-metadata-481-cleanup.
It verifies only src/stream_parse.c, src/streams.c and src/streams.h differ
between upstream and the pinned product commit. Every compiled source blob
must match the product commit; review changes can only add .review/ and
.github/workflows/ files. All original assets, tests and build tools come
from the canonical upstream tree. No copied product source is included here.

macOS prerequisites:
  brew install sdl2 sdl2_image sdl2_ttf ffmpeg ripgrep
From the checkout:
  python3 .review/pr487-cleanup/verify-source.py
  bash .review/pr487-cleanup/run-native-checks.sh

Metadata parser and TV/touch UI fixtures run normally and with ASan/UBSan.
They do not load dynamic SDL. Existing parser/cache/i18n tests and the
full-source Android-player link regression run normally. The UI fixture
checks actual source-row heights and filtering in TV and touch modes.
Homebrew SDL can block before main under ASan on macOS runners, so a full
application sanitizer startup is not claimed.

The Android job uses tools/android.sh unchanged, standard application ID,
both ARM architectures, and tools/android-apk-pages.py for 16 KiB checks.
It uses the existing NUVIO_TOUCH_CORE_PROPERTIES and
NUVIO_TOUCH_DEBUG_KEYSTORE_B64 repository secrets and prior dependency caches.
It produces only the standard debug APK; no custom package or release is
published. Proof JSON and logs are retained as workflow artifacts for 14 days.
