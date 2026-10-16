// #512: the real catalog store owns episode identities and copied source lists.
// Link real perfis.c as well: stale-session assertions must exercise its epoch
// and recursive lock, not a test replacement for ownership validation.
#include "../src/catalogo.h"
#include "../src/perfis.h"
#include "../src/progresso.h"
#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// No saved progress, filesystem, localization or network is involved here.
int prog_ler(ProgRegistro *out, int max) { (void)out; (void)max; return 0; }
int ajustes_idioma(void) { return 0; }
const char *i18n(const char *s) { return s; }
const char *dados_dir(void) { return ""; }
const char *sessao_usuario(void) { return "catalog-identity-fixture"; }
const char *desc_genero_pt(const char *s) { return s; }

static const char *TITLE = "tt21975436";
static const char *ORIGIN = "org.metadata.fixture";
static unsigned assertions;
#define CHECK(condition) do { assertions++; assert(condition); } while (0)

static CatItem title(const char *id, const char *type, const char *origin) {
  CatItem item;
  memset(&item, 0, sizeof item);
  snprintf(item.imdb, sizeof item.imdb, "%s", id);
  snprintf(item.tipo, sizeof item.tipo, "%s", type);
  snprintf(item.origem, sizeof item.origem, "%s", origin);
  snprintf(item.titulo, sizeof item.titulo, "Fixture series");
  return item;
}

static CatEp episode(int season, int number, int tvdb, const char *vid) {
  CatEp ep;
  memset(&ep, 0, sizeof ep);
  ep.temporada = season; ep.episodio = number; ep.tvdbEp = tvdb;
  snprintf(ep.vid, sizeof ep.vid, "%s", vid);
  snprintf(ep.nome, sizeof ep.nome, "Source S%dE%d", season, number);
  return ep;
}

static void sourceEpisodes(CatEp eps[3]) {
  // Public Kaiju No. 8 episode identities; the displayed/source numbers stay
  // unchanged even if a tracking service uses a different season layout.
  eps[0] = episode(1, 12, 10364437, "tt21975436:1:12");
  eps[1] = episode(2, 1, 10567809, "tt21975436:2:1");
  eps[2] = episode(2, 2, 11209906, "tt21975436:2:2");
}

static void publish(const CatItem *items, int count, const CatEp *eps, int q) {
  cat_definir_tudo(items, count, NULL, 0);
  for (int i = 0; i < count; i++) if (eps && q) cat_definir_episodios(i, eps, q);
}

static void unavailable(const char *id, const char *origin, uint64_t epoch) {
  CatEp sentinel = {0}, *owned = &sentinel;
  int count = -1;
  CHECK(!cat_copiar_episodios_fonte(id, origin, epoch, &owned, &count));
  CHECK(owned == NULL && count == 0);
}

static void lookups(void) {
  CatItem item = title(TITLE, "series", ORIGIN);
  CatEp eps[3];
  char vid[64];
  int season = 77, number = 88;
  sourceEpisodes(eps); publish(&item, 1, eps, 3);
  CHECK(cat_episodio_tvdb(0, TITLE, 2, 1) == 10567809);
  CHECK(cat_episodio_por_tvdb(0, TITLE, 10567809, &season, &number));
  CHECK(season == 2 && number == 1);
  CHECK(cat_episodio_tvdb(0, "tt21975436:2:1", 2, 2) == 11209906);
  CHECK(cat_episodio_por_tvdb(0, "tt21975436:1:12", 10364437, &season, &number));
  CHECK(season == 1 && number == 12);
  CHECK(cat_id_stream(0, 2, 1, vid, sizeof vid) && !strcmp(vid, eps[1].vid));
  CHECK(!memcmp(cat_episodio(0, 1), &eps[1], sizeof eps[1]));
  CHECK(cat_episodio_tvdb(-1, TITLE, 2, 1) == 0);
  CHECK(cat_episodio_tvdb(1, TITLE, 2, 1) == 0);
  CHECK(cat_episodio_tvdb(0, "tt219754360", 2, 1) == 0);
  CHECK(cat_episodio_tvdb(0, "kitsu:21975436", 2, 1) == 0);
  CHECK(cat_episodio_tvdb(0, NULL, 2, 1) == 0);
  season = 77; number = 88;
  CHECK(!cat_episodio_por_tvdb(0, TITLE, 999, &season, &number));
  CHECK(season == 77 && number == 88);
  CHECK(!cat_episodio_por_tvdb(0, TITLE, 0, &season, &number));
  item = title(TITLE, "movie", ORIGIN); publish(&item, 1, eps, 3);
  CHECK(cat_episodio_tvdb(0, TITLE, 2, 1) == 0);
  CHECK(!cat_episodio_por_tvdb(0, TITLE, 10567809, &season, &number));
  item = title(TITLE, "anime", ORIGIN); publish(&item, 1, eps, 3);
  CHECK(cat_episodio_tvdb(0, TITLE, 2, 1) == 0);
}

static void snapshotIdentity(void) {
  CatItem items[3];
  CatEp eps[3], *owned = NULL;
  uint64_t epoch = perfis_sessao_geracao();
  int count = -1;
  sourceEpisodes(eps);
  items[0] = title("tt21975436:2:1", "series", ORIGIN);
  items[1] = title(TITLE, "series", "org.other.fixture");
  items[2] = title(TITLE, "movie", ORIGIN);
  publish(items, 3, eps, 3);
  CHECK(cat_copiar_episodios_fonte(TITLE, ORIGIN, epoch, &owned, &count));
  CHECK(count == 3 && owned && !memcmp(owned, eps, sizeof eps));
  free(owned);
  CHECK(cat_copiar_episodios_fonte("tt21975436:1:12", ORIGIN, epoch, &owned, &count));
  CHECK(count == 3 && owned); free(owned);
  unavailable(TITLE, "org.missing.fixture", epoch);
  unavailable("tt219754360", ORIGIN, epoch);
  unavailable("kitsu:21975436", ORIGIN, epoch);
  unavailable(NULL, ORIGIN, epoch);
  // A known source with no published list is distinct from a missing source.
  items[0] = title(TITLE, "series", ORIGIN); publish(items, 1, NULL, 0);
  owned = NULL; count = -1;
  CHECK(cat_copiar_episodios_fonte(TITLE, ORIGIN, epoch, &owned, &count));
  CHECK(count == 0 && owned == NULL);
  // Kitsu is a separate source namespace even with the same TVDB evidence.
  items[0] = title("kitsu:7442", "series", ORIGIN);
  eps[0] = episode(1, 13, 10567809, "kitsu:7442:13"); publish(items, 1, eps, 1);
  CHECK(cat_copiar_episodios_fonte("kitsu:7442", ORIGIN, epoch, &owned, &count));
  CHECK(count == 1 && !strcmp(owned[0].vid, "kitsu:7442:13")); free(owned);
  unavailable("kitsu:7443", ORIGIN, epoch);
  unavailable(TITLE, ORIGIN, epoch);
}

static void ambiguity(void) {
  CatItem items[2] = {title(TITLE, "series", ORIGIN), title("tt21975436:2:1", "series", ORIGIN)};
  CatEp eps[4], changed[3], *owned = NULL;
  int count, season = 77, number = 88;
  uint64_t epoch = perfis_sessao_geracao();
  sourceEpisodes(eps); memcpy(changed, eps, sizeof changed);
  publish(items, 2, eps, 3);
  CHECK(cat_copiar_episodios_fonte(TITLE, ORIGIN, epoch, &owned, &count));
  CHECK(count == 3 && owned); free(owned);
  changed[1].tvdbEp = 999; cat_definir_episodios(1, changed, 3);
  unavailable(TITLE, ORIGIN, epoch);
  memcpy(changed, eps, sizeof changed);
  snprintf(changed[1].vid, sizeof changed[1].vid, "another-source:1");
  cat_definir_episodios(1, changed, 3); unavailable(TITLE, ORIGIN, epoch);
  cat_definir_episodios(1, eps, 2); unavailable(TITLE, ORIGIN, epoch);
  // One TVDB episode assigned to two different pairs cannot identify a source.
  eps[3] = episode(1, 13, 10567809, "tt21975436:1:13");
  publish(items, 1, eps, 4);
  CHECK(cat_episodio_tvdb(0, TITLE, 2, 1) == 0);
  CHECK(!cat_episodio_por_tvdb(0, TITLE, 10567809, &season, &number));
  CHECK(season == 77 && number == 88);
  // Two distinct TVDB identities assigned to a single displayed pair fail too.
  eps[3] = episode(2, 1, 999, "another-source:1"); publish(items, 1, eps, 4);
  CHECK(cat_episodio_tvdb(0, TITLE, 2, 1) == 0);
  // A repeated copy of the same identity/pair remains unambiguous.
  eps[3] = eps[1]; publish(items, 1, eps, 4);
  CHECK(cat_episodio_tvdb(0, TITLE, 2, 1) == 10567809);
  CHECK(cat_episodio_por_tvdb(0, TITLE, 10567809, &season, &number));
  CHECK(season == 2 && number == 1);
}

static void ownedSnapshot(void) {
  CatItem item = title(TITLE, "series", ORIGIN);
  CatEp eps[3], changed[3], *owned = NULL, *second = NULL;
  int count;
  uint64_t epoch = perfis_sessao_geracao();
  sourceEpisodes(eps); publish(&item, 1, eps, 3);
  CHECK(cat_copiar_episodios_fonte(TITLE, ORIGIN, epoch, &owned, &count));
  CHECK(count == 3 && owned);
  owned[1].tvdbEp = 999;
  snprintf(owned[1].vid, sizeof owned[1].vid, "owned-only");
  CHECK(cat_episodio_tvdb(0, TITLE, 2, 1) == 10567809);
  CHECK(!strcmp(cat_episodio(0, 1)->vid, eps[1].vid));
  CHECK(cat_copiar_episodios_fonte(TITLE, ORIGIN, epoch, &second, &count));
  CHECK(second && second != owned && !memcmp(second, eps, sizeof eps));
  memcpy(changed, eps, sizeof changed); changed[1].tvdbEp = 888;
  cat_definir_episodios(0, changed, 3);
  CHECK(second[1].tvdbEp == 10567809);
  item = title("tt0111161", "movie", "org.other.fixture");
  publish(&item, 1, NULL, 0); cat_quadro(); cat_quadro();
  CHECK(!memcmp(second, eps, sizeof eps));
  CHECK(owned[1].tvdbEp == 999 && !strcmp(owned[1].vid, "owned-only"));
  free(owned); free(second);
}

static void staleSession(void) {
  CatItem item = title(TITLE, "series", ORIGIN);
  CatEp eps[3], *owned = NULL;
  int count;
  uint64_t previous = perfis_sessao_geracao();
  sourceEpisodes(eps); publish(&item, 1, eps, 3);
  CHECK(cat_copiar_episodios_fonte(TITLE, ORIGIN, previous, &owned, &count));
  free(owned);
  perfis_invalidar_sessao();
  CHECK(!perfis_sessao_atual(previous));
  unavailable(TITLE, ORIGIN, previous);
  // Two visits later an old completion remains stale even for the same title.
  perfis_invalidar_sessao();
  unavailable(TITLE, ORIGIN, previous);
  CHECK(cat_copiar_episodios_fonte(TITLE, ORIGIN, perfis_sessao_geracao(), &owned, &count));
  CHECK(count == 3); free(owned);
}

static atomic_int startConcurrent;
static void *replaceCatalog(void *unused) {
  CatItem items[2];
  CatEp eps[3];
  (void)unused;
  sourceEpisodes(eps);
  while (!atomic_load_explicit(&startConcurrent, memory_order_acquire)) { }
  for (int i = 0; i < 180; i++) {
    int target = i % 2;
    items[target] = title(TITLE, "series", ORIGIN);
    items[1 - target] = title("tt0111161", "movie", "org.other.fixture");
    cat_definir_tudo(items, 2, NULL, 0);
    cat_definir_episodios(target, eps, 3);
    cat_quadro();
  }
  return NULL;
}

static void concurrentSnapshot(void) {
  CatItem item = title(TITLE, "series", ORIGIN);
  CatEp eps[3];
  pthread_t writer;
  uint64_t epoch = perfis_sessao_geracao();
  sourceEpisodes(eps); publish(&item, 1, eps, 3);
  CHECK(pthread_create(&writer, NULL, replaceCatalog, NULL) == 0);
  atomic_store_explicit(&startConcurrent, 1, memory_order_release);
  for (int i = 0; i < 700; i++) {
    CatEp *owned = NULL;
    int count = -1;
    CHECK(cat_copiar_episodios_fonte(TITLE, ORIGIN, epoch, &owned, &count));
    CHECK(count == 0 || count == 3);
    if (count) CHECK(owned && !memcmp(owned, eps, sizeof eps));
    else CHECK(owned == NULL);
    free(owned);
  }
  CHECK(pthread_join(writer, NULL) == 0);
}

int main(int argc, char **argv) {
  const char *scenario = argc > 1 ? argv[1] : "";
  if (!strcmp(scenario, "lookups")) lookups();
  else if (!strcmp(scenario, "snapshot-identity")) snapshotIdentity();
  else if (!strcmp(scenario, "ambiguity")) ambiguity();
  else if (!strcmp(scenario, "owned-snapshot")) ownedSnapshot();
  else if (!strcmp(scenario, "stale-session")) staleSession();
  else if (!strcmp(scenario, "concurrent-snapshot")) concurrentSnapshot();
  else { fprintf(stderr, "Unknown scenario: %s\n", scenario); return 2; }
  cat_definir_tudo(NULL, 0, NULL, 0); cat_quadro(); cat_quadro();
  printf("catalog_episode_identity512: PASS %s (%u assertions)\n", scenario, assertions);
  return 0;
}
