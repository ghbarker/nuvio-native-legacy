#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
static int probe_printf(const char *format, ...);
#define printf probe_printf
#include DETAIL_TRACE
#undef printf

static char captured[32768];
static size_t written;
static int checks, q = 2, source = 0, alternate, bg;
static Uint32 now = 1000;
static CatItem selected;
static int probe_printf(const char *format, ...) {
  va_list ap; va_start(ap, format);
  int n = vsnprintf(captured + written, sizeof captured - written, format, ap);
  va_end(ap); assert(n >= 0 && (size_t)n < sizeof captured - written);
  written += (size_t)n; return n;
}
const CatItem *cat_item(int i) { return i == 0 ? &selected : NULL; }
int ajustes_qualidade_imagem(void) { return q; }
int ajustes_hero_fonte(void) { return source; }
int ajustes_hero_arte_diferente(void) { return alternate; }
int fundo_modo(void) { return bg; }
Uint32 SDL_GetTicks(void) { return now; }
void probe_cache(const char *, int, int, int, int, int, int, int, int, int, int);
void probe_absent(void);
unsigned long probe_cache_checksum(void);
void probe_snapshot_checks(void);
#define CHECK(x) do { assert(x); checks++; } while (0)

int main(void) {
  const char *url = "https://images.metahub.space/background/medium/tt9876543/img?apikey=SECRET";
  const char *path;
  idx = 0;
  snprintf(selected.imdb, sizeof selected.imdb, "tt9876543");
  snprintf(selected.titulo, sizeof selected.titulo, "Oak Street");
  probe_snapshot_checks();
  CHECK(!strcmp(arteTesteHost(url, &path), "images.metahub.space"));
  CHECK(!strcmp(arteTesteTamanho("images.metahub.space", path), "metahub-medium"));
  const char *host = arteTesteHost("https://SECRET:SECRET@image.tmdb.org:443/t/p/w1280/PRIVATE.jpg?token=SECRET", &path);
  CHECK(!strcmp(host, "image.tmdb.org"));
  CHECK(!strcmp(arteTesteTamanho(host, path), "tmdb-w1280"));
  host = arteTesteHost("https://image.tmdb.org@private-secret.example/private-id?SECRET", &path);
  CHECK(!strcmp(host, "other"));
  CHECK(!strcmp(arteTesteTamanho(host, path), "unknown"));
  CHECK(!strcmp(arteTesteHost(NULL, &path), "none"));
  CHECK(!strcmp(arteTesteHost("/private/SECRET.jpg", &path), "local"));
  host = arteTesteHost("https://images.metahub.space?secret=/background/large/id", &path);
  CHECK(!strcmp(arteTesteTamanho(host, path), "unknown"));

  probe_cache(url, 3, 77, 1920, 1280, 1280, 0, 1280, 720, 0, 0);
  unsigned long unchanged = probe_cache_checksum();
  arteTesteAbrir(); arteTesteRegistrar(url, 0, 0);
  CHECK(arteTesteN == 1 && arteTesteSelecao == 1);
  CHECK(strstr(captured, "host=images.metahub.space size=metahub-medium"));
  CHECK(strstr(captured, "state=3 texture=1 hero_request=1920 max_request=1920"));
  CHECK(strstr(captured, "source_w=0 decoded=1280x720 gpu=1280x720"));
  CHECK(probe_cache_checksum() == unchanged);
  for (int i = 0; i < 100; i++) { now++; arteTesteRegistrar(url, 0, 0); }
  now += 250; arteTesteRegistrar(url, 0, 0);
  CHECK(arteTesteN == 1);

  probe_cache(url, 2, 77, 1920, 1920, 1920, 0, 1280, 720, 1920, 1080);
  now += 250; arteTesteRegistrar(url, 0, 0);
  CHECK(arteTesteN == 2 && strstr(captured, "decoded=1920x1080 gpu=1280x720"));
  probe_cache(url, 3, 88, 1920, 1920, 1920, 0, 1920, 1080, 0, 0);
  now += 250; arteTesteRegistrar(url, 0, 0);
  CHECK(arteTesteN == 3 && strstr(captured, "decoded=1920x1080 gpu=1920x1080"));
  now += 250; arteTesteRegistrar(url, 0, 1);
  CHECK(arteTesteN == 4 && strstr(captured, "video_background=1"));

  const char *custom = "https://SECRET@custom-private.example/PRIVATE-ID?token=SECRET";
  probe_cache(custom, 3, 88, 1920, 600, 0, 600, 600, 300, 0, 0);
  now += 250; arteTesteRegistrar(custom, 1, 0);
  CHECK(strstr(captured, "host=other size=unknown"));
  CHECK(strstr(captured, "poster=1 video_background=0"));
  CHECK(strstr(captured, "source_w=600 decoded=600x300 gpu=600x300"));
  CHECK(!strstr(captured, "SECRET") && !strstr(captured, "PRIVATE"));
  CHECK(!strstr(captured, "tt9876543") && !strstr(captured, "Oak Street"));
  CHECK(!strstr(captured, "https://") && !strstr(captured, "apikey") && !strstr(captured, "?"));
  for (int i = 0; i < 30; i++) { now += 250; bg = i % 3; arteTesteRegistrar(custom, 1, 0); }
  CHECK(arteTesteN == 16);
  size_t capped = written;
  for (int i = 0; i < 30; i++) { now += 250; bg = i % 3; arteTesteRegistrar(custom, 0, 1); }
  CHECK(written == capped);
  unsigned oldSelection = arteTesteSelecao;
  arteTesteAbrir(); arteTesteRegistrar(custom, 1, 0);
  CHECK(arteTesteSelecao == oldSelection + 1 && arteTesteN == 1 && written > capped);
  snprintf(selected.imdb, sizeof selected.imdb, "tt-other");
  now += 250; arteTesteRegistrar(custom, 1, 0);
  CHECK(arteTesteN == 2);
  probe_absent(); now += 250; arteTesteRegistrar(NULL, 0, 0);
  CHECK(arteTesteN == 3 && strstr(captured, "host=none size=unknown"));
  CHECK(!strstr(captured, "SECRET") && !strstr(captured, "PRIVATE"));
  printf("private-art-trace: %d checks passed; cache-hit, upgrades, unknown source, privacy, lifecycle and event bound\n", checks);
  return 0;
}
