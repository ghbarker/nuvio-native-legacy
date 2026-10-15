#include "streams.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int cases;
static void check(const char *label, const char *body, const char *provider, int ready, int outside) {
  char json[9000];
  snprintf(json, sizeof json, "{\"streams\":[%s]}", body);
  Stream *s = NULL;
  assert(stream_extrair(json, provider, &s) == 1);
#ifdef NV_CACHE_FIX
  if (s->emCache != ready || s->foraCache != outside) {
    fprintf(stderr, "%s: cached=%d outside=%d expected=%d/%d\n", label, s->emCache, s->foraCache, ready, outside);
    abort();
  }
#else
  (void)ready;
  assert(s->foraCache == outside);
#endif
  assert(!strcmp(s->provedor, provider));
  free(s); cases++;
}

int main(void) {
  check("TorBox +", "{\"url\":\"https://x.invalid/a\",\"name\":\"[TB+] Torrentio\\n1080p\"}", "TorBox addon", 1, 0);
  check("Real-Debrid +", "{\"url\":\"https://x.invalid/a\",\"name\":\"[RD+] Torrentio\"}", "RD addon", 1, 0);
  check("Premiumize +", "{\"url\":\"https://x.invalid/a\",\"name\":\"[PM+] Torrentio\"}", "PM addon", 1, 0);
  check("AllDebrid +", "{\"url\":\"https://x.invalid/a\",\"name\":\"[ad+] Torrentio\"}", "AD addon", 1, 0);
  check("AIOStreams lightning", "{\"url\":\"https://x.invalid/a\",\"name\":\"⚡️ UHD\"}", "AIOStreams", 1, 0);
  check("Description metadata", "{\"url\":\"https://x.invalid/a\",\"description\":\"⚡ cached\"}", "AIOStreams", 1, 0);
  check("Title metadata", "{\"url\":\"https://x.invalid/a\",\"title\":\"⚡ cached\"}", "AIOStreams", 1, 0);
  check("Structured hash true", "{\"clientResolve\":{\"infoHash\":\"0123\",\"isCached\":true}}", "AIOStreams", 1, 0);
  check("Structured URL true", "{\"url\":\"https://x.invalid/a\",\"clientResolve\":{\"isCached\":true}}", "AIOStreams", 1, 0);
  check("Structured hash false", "{\"clientResolve\":{\"infoHash\":\"0123\",\"isCached\":false}}", "AIOStreams", 0, 1);
  check("Structured URL false", "{\"url\":\"https://x.invalid/a\",\"name\":\"⚡ UHD\",\"clientResolve\":{\"isCached\":false}}", "AIOStreams", 0, 0);
  check("Plain URL unknown", "{\"url\":\"https://x.invalid/a\",\"name\":\"1080p\"}", "TorBox", 0, 0);
  check("Hash unknown", "{\"infoHash\":\"0123\"}", "Premiumize", 0, 0);
  check("Provider name not evidence", "{\"url\":\"https://x.invalid/a\"}", "[TB+] ⚡ AIOStreams", 0, 0);
  check("URL not evidence", "{\"url\":\"https://torbox.invalid/⚡/[TB+]/cached.mp4\"}", "TorBox", 0, 0);
  check("Filename not evidence", "{\"url\":\"https://x.invalid/a\",\"filename\":\"Movie.⚡.[TB+].mkv\"}", "AIOStreams", 0, 0);
  check("Uncached beats lightning", "{\"url\":\"https://x.invalid/a\",\"name\":\"⚡ UHD\",\"description\":\"not cached\"}", "AIOStreams", 0, 1);
  check("Download beats plus", "{\"url\":\"https://x.invalid/a\",\"name\":\"[TB+] [TB download]\"}", "AIOStreams", 0, 1);
  check("Negative text beats true", "{\"url\":\"https://x.invalid/a\",\"name\":\"⏳ UHD\",\"clientResolve\":{\"isCached\":true}}", "AIOStreams", 0, 1);
  check("String boolean is unknown", "{\"url\":\"https://x.invalid/a\",\"clientResolve\":{\"isCached\":\"true\"}}", "AIOStreams", 0, 0);
  check("Numeric boolean is unknown", "{\"url\":\"https://x.invalid/a\",\"clientResolve\":{\"isCached\":1}}", "AIOStreams", 0, 0);
  check("Nested metadata not evidence", "{\"url\":\"https://x.invalid/a\",\"proxyHeaders\":{\"clientResolve\":{\"isCached\":true}}}", "AIOStreams", 0, 0);
  check("Nested name not evidence", "{\"url\":\"https://x.invalid/a\",\"proxyHeaders\":{\"name\":\"⚡ [TB+]\"}}", "AIOStreams", 0, 0);
  check("Header property not evidence", "{\"url\":\"https://x.invalid/a\",\"clientResolve\":{\"headers\":{\"isCached\":true}}}", "AIOStreams", 0, 0);
  check("Sibling key not evidence", "{\"url\":\"https://x.invalid/a\",\"clientResolve\":{},\"isCached\":true}", "AIOStreams", 0, 0);
  check("Structured whitespace", "{\"url\":\"https://x.invalid/a\",\"clientResolve\":{\"isCached\":true  }}", "AIOStreams", 1, 0);
  Stream *s = NULL;
  const char *json = "{\"streams\":[{\"url\":\"https://x.invalid/a\"},{\"url\":\"https://x.invalid/b\",\"clientResolve\":{\"isCached\":true}}]}";
  assert(stream_extrair(json, "AIOStreams", &s) == 2);
#ifdef NV_CACHE_FIX
  assert(!s[0].emCache && s[1].emCache);
#endif
  assert(!s[0].foraCache && !s[1].foraCache);
  free(s); cases++;
  printf("cache_metadata: %d cases passed (%s)\n", cases,
#ifdef NV_CACHE_FIX
    "positive, negative and unknown"
#else
    "upstream negative behavior; no positive cache field"
#endif
  );
}
