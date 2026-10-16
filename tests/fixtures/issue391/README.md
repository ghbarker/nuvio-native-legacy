# Issue 391 reported-title metadata

The issue photograph shows Star Trek: The Next Generation (1987), season 2 selected with an empty episode row. These public responses were downloaded on 2026-10-16 and retained byte for byte. They contain no account credentials.

- Nuvio: `https://catalog.nuvio.tv/%7B%22language%22%3A%22en-US%22%7D/meta/series/tt0092455.json`
  - SHA-256 `cba075694ce1f832defa8c86152ff7aed71d0b064024151a8030f180f6e94a97`
  - 176 episodes, seven seasons, 22 episodes in season 2.
- Cinemeta: `https://v3-cinemeta.strem.io/meta/series/tt0092455.json`
  - SHA-256 `d91f004a8106494d4f0e0f24c3ff981df52dfe198d557e7015135815cb13fd62`
  - 178 episodes, seven seasons, 22 episodes in season 2.

Each provider's own grouping is preserved: this test never merges their differing episode lists. The fixture verifies current parser/publication and catalog identity rebinding, not physical Tizen rendering or the unavailable original diagnostic log. It does not establish that the whole issue is fixed.
