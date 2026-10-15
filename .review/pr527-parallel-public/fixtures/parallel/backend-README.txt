Exact-public PR #527 backend regression fixtures

These external fixtures include the actual unmodified src/descoberta.c from the source argument and link the actual parser/profile/addon URL helpers. Network, read-only settings/addon/catalog lookup, i18n and Trakt artwork enrichment are controlled boundaries. No product code is copied or rewritten by a fixture. They are validation support, not public PR changes.

reader: 45 independent process cases. The original 39 cover independent addon/TMDB/Trakt requests, bounded watched-page supplements, raw addon skip, top100/top250, deduplication, structural JSON errors and whitespace-empty arrays, Trakt artwork compaction, profile invalidation and mixed three-provider overlap. Additional assertions verify manual next-cursor and terminal/error state. The six added cases pass the returned cursor to the actual existing desc_vertudo_filtro_desde / desc_vertudo_fonte_desde singleton functions and inspect their actual results: a 31-entry addon page, encoded genre plus raw cursor, transport failure retaining the retry cursor, the 60-item bounded all-watched batch followed by visible items, TMDB page 1 -> 2, and a full Trakt page followed by a short terminal page. Earlier singleton reader state must remain unchanged by each independent initial request.

reader invocation: python3 fixture/run.py --source SOURCE --out OUTPUT --sdk SDL_HEADER_DIRECTORY [--sanitize]
The runner requires all 45 cases plus compilation, verifies actual product source hashes are unchanged over validation, records binary/fixture/source/log hashes, and treats any timeout, compiler error, missing case or sanitizer finding as failure.

owned-url: exercises actual lerManifesto retaining two declaration base URLs after function return, a second parse, compaction, and later nv_addon_url use. The original public 16194 source has a recorded ASan stack-lifetime failure; the owned URL candidate must pass normally and under ASan/UBSan with stack-use-after-return enabled.

owned-url invocation: python3 owned-url/run.py --source SOURCE --support TYPED_SUPPORT_DIRECTORY --out OUTPUT [--sanitize]
Typed support comes from the independently pinned latch fixture only. Existing source is never edited. On macOS, leak detection is disabled for SDK/runtime allocation noise; ASan/UBSan remain enabled with halt_on_error. Linux leak detection remains enabled for reader cases.

Preflight is not a final frozen-source result. Final evidence must be rebound to the public source commit/tree selected by the root and rerun after any rebase/source change.
