/* Keep every existing fixture assertion unchanged, in its own fresh process. */
#define main original_artereserva_main
#include "original_artereserva.c"
#undef main
#include "arteurl.h"

static unsigned checks;
#define CHECK(cond, msg) do { checks++; OK((cond), (msg)); } while (0)

static void long_url(char *out, size_t n, unsigned key) {
  int prefix = snprintf(out, NV_ARTE_URL_MAX + 2, "https://artwork.test/%04u/", key);
  if (prefix < 0 || (size_t)prefix >= n) abort();
  memset(out + prefix, 'q', n - (size_t)prefix);
  out[n] = 0;
}

static int long_cases(void) {
  char a[NV_ARTE_URL_MAX + 2], b[NV_ARTE_URL_MAX + 2], out[400];
  const size_t sizes[] = {511, 512, 1173, NV_ARTE_URL_MAX - 1};
  respostasColisao = 1;
  for (unsigned i = 0; i < sizeof sizes / sizeof *sizes; i++) {
    long_url(a, sizes[i], i);
    CHECK(strlen(a) == sizes[i], "long fixture exact length");
    CHECK(arte_reserva_registrar(a, "tt0111161", 1) == 1,
          "accepted artwork URL registers fallback");
    CHECK(arte_reserva_url(a, out, sizeof out) == 1 &&
          !strcmp(out, "https://image.tmdb.org/t/p/w342/one.jpg"),
          "long registered URL resolves its poster fallback");
  }
  long_url(a, NV_ARTE_URL_MAX - 1, 90);
  memcpy(b, a, NV_ARTE_URL_MAX);
  a[NV_ARTE_URL_MAX - 2] = 'a';
  b[NV_ARTE_URL_MAX - 2] = 'b';
  CHECK(!memcmp(a, b, NV_ARTE_URL_MAX - 2), "maximum keys share full prefix");
  CHECK(arte_reserva_registrar(a, "tt0111161", 1) == 1, "first full key registered");
  CHECK(arte_reserva_registrar(b, "tt0903747", 0) == 1, "second full key registered");
  CHECK(arte_reserva_url(a, out, sizeof out) == 1 &&
        !strcmp(out, "https://image.tmdb.org/t/p/w342/one.jpg"),
        "full original key resolves its own IMDb and kind");
  CHECK(arte_reserva_url(b, out, sizeof out) == 1 &&
        !strcmp(out, "https://image.tmdb.org/t/p/w1280/two-bg.jpg"),
        "different maximum-key tail resolves independently");
  int repeated = 1;
  for (unsigned i = 0; i < 1100; i++)
    if (!arte_reserva_registrar(a, "tt0903747", 0)) repeated = 0;
  CHECK(repeated, "re-registering a maximum URL does not exhaust arena");
  CHECK(arte_reserva_url(a, out, sizeof out) == 1 &&
        !strcmp(out, "https://image.tmdb.org/t/p/w1280/two-bg.jpg"),
        "re-register changes stored IMDb and kind");
  for (size_t n = NV_ARTE_URL_MAX; n <= NV_ARTE_URL_MAX + 1; n++) {
    long_url(a, n, 99);
    CHECK(!arte_reserva_registrar(a, "tt0111161", 1), "over-limit URL rejected whole");
    pedidos = 0;
    CHECK(!arte_reserva_url(a, out, sizeof out) && pedidos == 0,
          "rejected URL has no fallback or metadata request");
  }
  CHECK(arte_reserva_url(b, out, sizeof out) == 1 &&
        !strcmp(out, "https://image.tmdb.org/t/p/w1280/two-bg.jpg"),
        "over-limit keys do not overwrite an accepted maximum key");
  printf("long-url checks=%u failures=%d\n", checks, falhas);
  return falhas ? 1 : 0;
}

static int arena_cases(void) {
  enum { ARENA_BYTES = 2 * 1024 * 1024 };
  char u[NV_ARTE_URL_MAX + 2], out[400];
  unsigned accepted = 0, rejected = 0;
  respostasColisao = 1;
  for (unsigned i = 0; i < 1200; i++) {
    long_url(u, NV_ARTE_URL_MAX - 1, i);
    if (arte_reserva_registrar(u, "tt0111161", 1)) accepted++;
    else rejected++;
  }
  CHECK(accepted == ARENA_BYTES / (NV_ARTE_URL_MAX - 1),
        "long keys stop exactly at existing two-MiB arena bound");
  CHECK(accepted + rejected == 1200 && rejected > 0,
        "arena exhaustion rejects further complete keys");
  long_url(u, NV_ARTE_URL_MAX - 1, 0);
  CHECK(arte_reserva_registrar(u, "tt0903747", 0) == 1,
        "existing maximum key updates after arena is full");
  CHECK(arte_reserva_url(u, out, sizeof out) == 1 &&
        !strcmp(out, "https://image.tmdb.org/t/p/w1280/two-bg.jpg"),
        "updated full-arena key still resolves its own fallback");
  printf("arena checks=%u failures=%d accepted=%u rejected=%u\n",
         checks, falhas, accepted, rejected);
  return falhas ? 1 : 0;
}

int main(int argc, char **argv) {
  if (argc != 2) return 2;
  if (!strcmp(argv[1], "original")) return original_artereserva_main();
  if (!strcmp(argv[1], "long")) return long_cases();
  if (!strcmp(argv[1], "arena")) return arena_cases();
  return 2;
}
