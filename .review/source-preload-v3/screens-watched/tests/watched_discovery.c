// Issue #366: exercise real paginated discovery and real watched history.
// Synthetic catalog responses only; no production services or account data.
#include <assert.h>
#include <unistd.h>
static int hideWatched, hideFuture;
int ajustes_social(void) { return 1; }
int ajustes_hist_conta(void) { return 1; }
int ajustes_busca_cinemeta(void) { return 1; }
int ajustes_busca_nuvio(void) { return 0; }
int ajustes_ocultar_nao_lancados(void) { return hideFuture; }
int ajustes_ocultar_filmes_vistos(void) { return hideWatched; }
int ajustes_itens_fileira(void) { return 24; }
#include "descoberta.c"
int ajustes_idioma_ingles(void) { return 0; }
int ajustes_idioma(void) { return 0; }
int ajustes_tmdb_ligado(void) { return 1; }
const char *ajustes_tmdb_idioma(void) { return "pt-BR"; }
const char *i18n(const char *s) { return s; }
const char *idioma_mes_data(int mes, const char *nomePt) { (void)mes; return nomePt; }
const char *nuvem_trakt_cliente(void) { return "test-only-client"; }
char *rede_baixar_com(const char *u, int t, const char *const *c) {
  (void)c; return rede_baixar(u, t);
}
int trakt_enfeitar_lote(CatItem *s, int n) { (void)s; (void)n; return 0; }
int addons_n(void) { return 0; }
int addons_base_desligada(const char *b) { (void)b; return 0; }
const char *addons_base(int i) { (void)i; return ""; }
const char *addons_id_manifesto(int i) { (void)i; return ""; }
const char *dados_dir(void) { return ""; }
const char *sessao_usuario(void) { return ""; }
int perfis_ativo(void) { return 1; }
int prog_ler(ProgRegistro *v, int n) { (void)v; (void)n; return 0; }
int prog_tocou(const char *id) { return strcmp(id, "ttSeekOnly") ? 1 : 0; }
int prog_gravar_local(const char *id, int t, int e, double p, double d, int tocou) {
  (void)id; (void)t; (void)e; (void)p; (void)d; (void)tocou; return 0;
}

static pthread_mutex_t fixtureLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t fixtureSignal = PTHREAD_COND_INITIALIZER;
static int requests, skips[32], holdEntered, holdRelease, failedOnce, cachedFailedOnce;

static char *catalog(int start, int count, const char *prefix) {
  size_t cap = (size_t)count * 160 + 32;
  char *json = malloc(cap);
  assert(json);
  size_t used = (size_t)snprintf(json, cap, "{\"metas\":[");
  for (int i = 0; i < count; i++)
    used += (size_t)snprintf(json + used, cap - used,
        "%s{\"id\":\"%s%d\",\"name\":\"Title %d\",\"poster\":\"p.jpg\",\"releaseInfo\":\"2020\"}",
        i ? "," : "", prefix, start + i, start + i);
  snprintf(json + used, cap - used, "]}");
  return json;
}
static char *traktCatalog(int start, int count) {
  size_t cap = (size_t)count * 160 + 32;
  char *json = malloc(cap); assert(json);
  size_t used = (size_t)snprintf(json, cap, "[");
  for (int i = 0; i < count; i++)
    used += (size_t)snprintf(json + used, cap - used,
      "%s{\"type\":\"movie\",\"movie\":{\"title\":\"Trakt %d\",\"year\":2020,\"ids\":{\"imdb\":\"ttTrakt%d\"}}}",
      i ? "," : "", start + i, start + i);
  snprintf(json + used, cap - used, "]"); return json;
}

char *rede_baixar(const char *url, int timeout) {
  (void)timeout;
  int skip = 0;
  const char *s = strstr(url, "skip=");
  if (s) skip = atoi(s + 5);
  if (strstr(url, "/lists/366/")) {
    s = strstr(url, "page="); assert(s); skip = atoi(s + 5);
  }
  pthread_mutex_lock(&fixtureLock);
  assert(requests < (int)(sizeof skips / sizeof skips[0]));
  skips[requests++] = skip;
  if (strstr(url, "hold.invalid") || (strstr(url, "partial.invalid") && skip)) {
    holdEntered = 1; pthread_cond_broadcast(&fixtureSignal);
    while (!holdRelease) pthread_cond_wait(&fixtureSignal, &fixtureLock);
  }
  pthread_mutex_unlock(&fixtureLock);
  if (strstr(url, "/lists/366/")) {
    assert(skip >= 1 && skip <= 4);
    return traktCatalog((skip - 1) * 40, skip == 4 ? 1 : 40);
  }
  if (strstr(url, "mixed.invalid")) return skip ? catalog(0, 0, "tt") : strdup(
    "{\"metas\":["
    "{\"id\":\"ttWatched\",\"name\":\"Watched movie\",\"poster\":\"p.jpg\",\"type\":\"movie\"},"
    "{\"id\":\"ttUnknown\",\"name\":\"Unknown movie\",\"poster\":\"p.jpg\",\"type\":\"movie\"},"
    "{\"id\":\"ttSeries\",\"name\":\"Watched series\",\"poster\":\"p.jpg\",\"type\":\"series\"},"
    "{\"id\":\"ttUnmarked\",\"name\":\"Unmarked movie\",\"poster\":\"p.jpg\",\"type\":\"movie\"},"
    "{\"id\":\"ttKeep\",\"name\":\"Keep movie\",\"poster\":\"p.jpg\",\"type\":\"movie\"},"
    "{\"id\":\"ttAnime\",\"name\":\"Unknown type\",\"poster\":\"p.jpg\",\"type\":\"anime\"}]}"
  );
  if (strstr(url, "gap.invalid")) {
    if (!skip) return catalog(0, 2, "ttGap");
    if (skip == 2) return catalog(2, 1, "ttGap");
    if (skip == 3) return catalog(3, 1, "ttGap");
    assert(skip == 4); return catalog(0, 0, "ttGap");
  }
  if (strstr(url, "long.invalid")) {
    if (skip < 8) { assert(!(skip % 2)); return catalog(skip, 2, "ttLong"); }
    if (skip == 8) return catalog(8, 1, "ttLong");
    assert(skip == 9); return catalog(0, 0, "ttLong");
  }
  if (strstr(url, "empty.invalid")) return catalog(skip, skip ? 0 : 2, "ttEmpty");
  if (strstr(url, "repeat.invalid")) return catalog(0, 2, "ttRepeat");
  if (strstr(url, "overflow.invalid")) return catalog(0, 150, "ttOver");
  if (strstr(url, "refill.invalid")) {
    if (!skip) return catalog(0, 10, "ttRefill");
    assert(skip == 10); return catalog(10, 6, "ttRefill");
  }
  if (strstr(url, "full.invalid")) return catalog(0, 100, "ttFull");
  if (strstr(url, "hidden-page.invalid")) {
    if (!skip) return catalog(0, 100, "ttHidden");
    assert(skip == 100); return catalog(100, 24, "ttHidden");
  }
  if (strstr(url, "partial.invalid")) {
    if (!skip) return catalog(0, 100, "ttPartial");
    assert(skip == 100); return NULL;
  }
  if (strstr(url, "retry.invalid")) {
    assert(skip == 0);
    if (!failedOnce++) return NULL;
    return catalog(0, 2, "ttRetry");
  }
  if (strstr(url, "cached-error.invalid")) {
    if (!skip) return catalog(0, 2, "ttCacheErr");
    assert(skip == 2);
    if (!cachedFailedOnce++) return NULL;
    return catalog(2, 1, "ttCacheErr");
  }
  if (strstr(url, "hold.invalid")) return catalog(0, 2, "ttOld");
  if (strstr(url, "new.invalid")) return catalog(0, 2, "ttNew");
  assert(!"unexpected synthetic URL"); return NULL;
}

static void waitDone(void) {
  for (int i = 0; i < 4000 && desc_vertudo_carregando(); i++) usleep(1000);
  assert(!desc_vertudo_carregando());
}
static void resetRequests(void) { requests = 0; memset(skips, 0, sizeof skips); }
static void expectOrder(const char *const *ids, int n) {
  if (desc_vertudo_n() != n) {
    fprintf(stderr, "expected %d visible, got %d raw=%d hide=%d\n", n, desc_vertudo_n(), vtN, hideWatched);
    for (int i = 0; i < vtN; i++) fprintf(stderr, "  %s/%s watched=%d\n", vtItens[i].imdb, vtItens[i].tipo, cat_visto(&vtItens[i]));
  }
  assert(desc_vertudo_n() == n);
  for (int i = 0; i < n; i++) {
    CatItem item;
    assert(desc_vertudo_item(i, &item));
    assert(!strcmp(item.imdb, ids[i]));
  }
  CatItem item;
  assert(!desc_vertudo_item(n, &item));
  assert(!desc_vertudo_item(-1, &item));
}
static void watched(const char *prefix, int n) {
  char id[64];
  for (int i = 0; i < n; i++) {
    snprintf(id, sizeof id, "%s%d", prefix, i);
    cat_historico_definir_id(id, "movie", 1);
  }
}

int main(void) {
  cat_historico_contexto("synthetic-owner", 1);
  CatItem home[HOME_VISTOS_MAX]; int responded = 0;
  watched("ttRefill", 8);
  hideWatched = 0; resetRequests();
  assert(lerCatalogoHome("https://refill.invalid", "movie", "rank", home, HOME_VISTOS_MAX, 6, 1, &responded) == 6);
  assert(requests == 1 && responded && !strcmp(home[0].imdb, "ttRefill0"));
  hideWatched = 1; resetRequests();
  int rawHome = lerCatalogoHome("https://refill.invalid", "movie", "rank", home, HOME_VISTOS_MAX, 6, 1, &responded);
  assert(rawHome == 14 && requests == 2 && skips[1] == 10 && responded);
  for (int i = 0; i < rawHome; i++) {
    char id[64]; snprintf(id, sizeof id, "ttRefill%d", i);
    assert(!strcmp(home[i].imdb, id));
  }
  resetRequests();
  assert(lerCatalogoHome("https://refill.invalid", "movie", "rank", home, HOME_VISTOS_MAX, 6, 0, &responded) == 10);
  assert(requests == 1);
  resetRequests();
  assert(lerCatalogoHome("https://refill.invalid", "series", "rank", home, HOME_VISTOS_MAX, 6, 1, &responded) == 6);
  assert(requests == 1);
  watched("ttFull", 60); resetRequests();
  assert(lerCatalogoHome("https://full.invalid", "movie", "rank", home, HOME_VISTOS_MAX, 24, 1, &responded) == HOME_VISTOS_MAX);
  assert(requests == 1);
  for (int i = 0; i < HOME_VISTOS_MAX; i++) {
    char id[64]; snprintf(id, sizeof id, "ttFull%d", i < 24 ? i : 60 + i - 24);
    assert(!strcmp(home[i].imdb, id));
  }
  hideWatched = 0; resetRequests();
  assert(lerCatalogoHome("https://full.invalid", "movie", "rank", home, HOME_VISTOS_MAX, 24, 1, &responded) == 24);
  assert(requests == 1 && !strcmp(home[0].imdb, "ttFull0") && !strcmp(home[23].imdb, "ttFull23"));
  hideWatched = 1; watched("ttHidden", 100); resetRequests();
  assert(lerCatalogoHome("https://hidden-page.invalid", "movie", "rank", home, HOME_VISTOS_MAX, 24, 1, &responded) == HOME_VISTOS_MAX);
  assert(requests == 2 && skips[1] == 100 && !strcmp(home[24].imdb, "ttHidden100"));
  watched("ttRepeat", 2); resetRequests();
  assert(lerCatalogoHome("https://repeat.invalid", "movie", "rank", home, HOME_VISTOS_MAX, 24, 1, &responded) == 2);
  assert(requests == 2 && skips[1] == 2 && !strcmp(home[0].imdb, "ttRepeat0") && !strcmp(home[1].imdb, "ttRepeat1"));
  const char *modernSkip = "{\"extra\":[{\"name\":\"skip\"}]}";
  const char *legacySkip = "{\"extraSupported\":[\"skip\"]}";
  const char *noSkip = "{\"extra\":[{\"name\":\"search\"}]}";
  assert(aceitaSkip(modernSkip, modernSkip + strlen(modernSkip)));
  assert(aceitaSkip(legacySkip, legacySkip + strlen(legacySkip)));
  assert(!aceitaSkip(noSkip, noSkip + strlen(noSkip)));
  // Extra raw candidates cannot consume slots reserved for later catalogs.
  assert(homeVistosLimitar(48, CAT_MAX - 24 - 24, 1, 0) == 24);
  assert(homeVistosLimitar(48, CAT_MAX - 48 - 24, 1, 0) == 48);
  puts("PASS Home reader: disabled/non-movie unchanged, raw reservoir, two requests only with declared skip, capacity preserves later rows");
  puts("PASS dense Home page: first 60/100 hidden still fills 24 visible in one GET; off restores original 24; all-hidden page uses raw skip=100");

  // Actual async catalog worker: once its extra GET stalls, the accepted first
  // page is already exposed as a stable snapshot for the grace-period path.
  watched("ttPartial", 90); resetRequests(); holdEntered = holdRelease = 0;
  RodadaCat *round = calloc(1, sizeof *round); assert(round);
  round->t = calloc(1, sizeof *round->t); assert(round->t);
  round->nT = 1; round->refs = 2;
  TarefaCat *task = round->t;
  strcpy(task->base, "https://partial.invalid"); strcpy(task->tipo, "movie"); strcpy(task->id, "rank"); task->paginavel = 1;
  pthread_t worker; assert(!pthread_create(&worker, NULL, fioCatalogo, round));
  pthread_mutex_lock(&fixtureLock);
  while (!holdEntered) pthread_cond_wait(&fixtureSignal, &fixtureLock);
  pthread_mutex_unlock(&fixtureLock);
  pthread_mutex_lock(&catTrava);
  assert(!task->pronto && task->primeira && task->nPrimeira == 34);
  assert(!strcmp(task->primeira[0].imdb, "ttPartial0"));
  assert(!strcmp(task->primeira[24].imdb, "ttPartial90"));
  CatItem *accepted = task->primeira;
  task->largada = 1;
  pthread_mutex_unlock(&catTrava);
  // The publisher uses the immutable first-page pointer after dropping lock.
  assert(!strcmp(accepted[33].imdb, "ttPartial99"));
  pthread_mutex_lock(&fixtureLock); holdRelease = 1;
  pthread_cond_broadcast(&fixtureSignal); pthread_mutex_unlock(&fixtureLock);
  assert(!pthread_join(worker, NULL));
  pthread_mutex_lock(&catTrava);
  assert(task->pronto && task->respondeu && task->n == 34);
  assert(task->primeira == accepted && task->nPrimeira == 34 && round->refs == 1);
  pthread_mutex_unlock(&catTrava);
  assert(!strcmp(accepted[0].imdb, "ttPartial0") && !strcmp(accepted[33].imdb, "ttPartial99"));
  rodadaSoltar(round);
  puts("PASS slow extra page: actual worker exposes first page before completion and retains it safely through abandonment/failure");
  cat_historico_definir_id("ttWatched", "movie", 1);
  cat_historico_definir_id("ttSeries", "series", 1);
  cat_historico_definir_id("ttAnime", "movie", 1);
  cat_historico_definir_id("ttUnmarked", "movie", 0);
  static const char *all[] = {"ttWatched", "ttUnknown", "ttSeries", "ttUnmarked", "ttKeep", "ttAnime"};
  static const char *visible[] = {"ttUnknown", "ttSeries", "ttUnmarked", "ttKeep", "ttAnime"};
  static const char *marked[] = {"ttSeries", "ttUnmarked", "ttKeep", "ttAnime"};
  hideWatched = 0; resetRequests();
  desc_vertudo_abrir("https://mixed.invalid", "movie", "rank"); waitDone();
  expectOrder(all, 6); assert(requests == 1);
  // Existing deMeta guesses unknown catalog media as movie. Seed a truly
  // unresolved raw type to verify the filter itself never guesses.
  strcpy(vtItens[5].tipo, "anime");
  hideWatched = 1; expectOrder(visible, 5); assert(requests == 1 && vtN == 6);
  assert(cat_historico_estado_id("ttUnknown", "movie") == -1);
  // A user marking a loaded movie hides it without fetching or losing raw data.
  cat_historico_definir_id("ttUnknown", "movie", 1);
  expectOrder(marked, 4); assert(requests == 1 && vtN == 6);
  cat_historico_definir_id("ttUnknown", "movie", 0);
  expectOrder(visible, 5);
  unsigned projectionRevision = desc_vertudo_revisao();
  cat_historico_definir_id("ttUnknown", "movie", 1);
  cat_historico_definir_id("ttWatched", "movie", 0);
  static const char *swapped[] = {"ttWatched", "ttSeries", "ttUnmarked", "ttKeep", "ttAnime"};
  expectOrder(swapped, 5);
  assert(desc_vertudo_revisao() != projectionRevision && requests == 1);
  cat_historico_definir_id("ttUnknown", "movie", 0);
  cat_historico_definir_id("ttWatched", "movie", 1);
  expectOrder(visible, 5);
  // Known unmark wins over old completion progress; seek-only also stays visible.
  vtItens[3].progresso = 100;
  assert(!cat_visto(&vtItens[3])); expectOrder(visible, 5);
  CatItem seek = {0}; strcpy(seek.imdb, "ttSeekOnly"); strcpy(seek.tipo, "movie"); seek.progresso = 100;
  assert(!cat_visto(&seek));
  hideWatched = 0; expectOrder(all, 6); assert(requests == 1);
  hideWatched = 1; expectOrder(visible, 5);
  // A profile switch clears the watched map; a stale profile response cannot hide anything.
  unsigned long long oldGeneration = cat_historico_geracao();
  cat_historico_contexto("synthetic-owner", 2);
  assert(!cat_historico_definir_se_geracao("ttWatched", "movie", 1, oldGeneration));
  vtItens[3].progresso = 0;
  expectOrder(all, 6);
  CatItem canonical = {0};
  strcpy(canonical.imdb, "ttKeep"); strcpy(canonical.tipo, "movie"); canonical.progresso = 100;
  cat_definir(&canonical, 1);
  static const char *canonicalVisible[] = {"ttWatched", "ttUnknown", "ttSeries", "ttUnmarked", "ttAnime"};
  expectOrder(canonicalVisible, 5); assert(requests == 1 && vtItens[4].progresso == 0);
  cat_historico_definir_id("ttKeep", "movie", 0); expectOrder(all, 6);
  cat_definir(NULL, 0);
  puts("PASS projection: disabled, exact movies only, stable order, mark/unmark, toggle restore, profile refresh, no network");
  puts("PASS metadata-only catalog: canonical local completion is reused; explicit unmark overrides stale local progress");

  watched("ttGap", 3); resetRequests();
  desc_vertudo_abrir("https://gap.invalid", "movie", "rank"); waitDone();
  static const char *gap[] = {"ttGap3"}; expectOrder(gap, 1);
  assert(requests == 3 && skips[0] == 0 && skips[1] == 2 && skips[2] == 3);
  assert(!desc_vertudo_fim());
  desc_vertudo_mais(); waitDone(); assert(desc_vertudo_fim() && requests == 4 && skips[3] == 4);
  puts("PASS pagination: all-hidden pages advance by raw lengths, later visible item and genuine exhaustion");

  watched("ttLong", 8); resetRequests();
  desc_vertudo_abrir("https://long.invalid", "movie", "rank"); waitDone();
  assert(requests <= 3 && desc_vertudo_n() == 0 && !desc_vertudo_fim());
  int before = requests;
  for (int frame = 0; frame < 20; frame++) {
    assert(desc_vertudo_n() == 0 && !desc_vertudo_carregando());
    (void)desc_vertudo_revisao();
  }
  usleep(10000); assert(requests == before);
  hideWatched = 0;
  desc_vertudo_abrir("https://long.invalid", "movie", "rank"); waitDone();
  assert(requests == before && desc_vertudo_n() == 6);
  hideWatched = 1;
  // Returning to the same cached catalog is another explicit, bounded request.
  desc_vertudo_abrir("https://long.invalid", "movie", "rank"); waitDone();
  static const char *longGap[] = {"ttLong8"}; expectOrder(longGap, 1);
  assert(requests - before <= 3 && requests == 5 && skips[4] == 8 && vtN == 9);
  desc_vertudo_abrir("https://long.invalid", "movie", "rank"); waitDone();
  assert(requests == 5);
  puts("PASS pagination: 3-page request budget, idle does not fetch, explicit cached empty reopen advances, visible cache reopens locally");

  watched("ttEmpty", 2); resetRequests();
  desc_vertudo_abrir("https://empty.invalid", "movie", "rank"); waitDone();
  assert(desc_vertudo_n() == 0 && desc_vertudo_fim() && requests == 2);
  desc_vertudo_abrir("https://empty.invalid", "movie", "rank"); waitDone();
  assert(requests == 2 && desc_vertudo_fim());
  watched("ttRepeat", 2); resetRequests();
  desc_vertudo_abrir("https://repeat.invalid", "movie", "rank"); waitDone();
  assert(desc_vertudo_n() == 0 && desc_vertudo_fim() && requests == 2 && skips[1] == 2);
  puts("PASS pagination: genuinely empty catalogs and non-paginating repeated catalogs finish");

  watched("ttCacheErr", 2); resetRequests();
  desc_vertudo_abrir("https://cached-error.invalid", "movie", "rank"); waitDone();
  assert(desc_vertudo_erro() && !desc_vertudo_fim() && vtN == 2 && requests == 2);
  desc_vertudo_abrir("https://cached-error.invalid", "movie", "rank"); waitDone();
  assert(desc_vertudo_erro() && requests == 2);
  desc_vertudo_mais(); waitDone();
  static const char *cachedRetry[] = {"ttCacheErr2"}; expectOrder(cachedRetry, 1);
  assert(requests == 3 && !desc_vertudo_erro());
  puts("PASS cached error: reopening does not hide or silently retry failures; explicit More can recover");

  watched("ttTrakt", 120); resetRequests();
  ColSource source = {0}; strcpy(source.prov, "trakt"); strcpy(source.midia, "Movies"); source.traktLista = 366;
  desc_vertudo_fonte(&source); waitDone();
  assert(requests == 3 && desc_vertudo_n() == 0 && vtN == 120 && !desc_vertudo_fim());
  int sourceBefore = requests;
  for (int frame = 0; frame < 20; frame++) {
    assert(desc_vertudo_n() == 0 && !desc_vertudo_carregando());
    (void)desc_vertudo_revisao();
  }
  usleep(10000); assert(requests == sourceBefore);
  desc_vertudo_fonte(&source); waitDone();
  static const char *traktVisible[] = {"ttTrakt120"}; expectOrder(traktVisible, 1);
  assert(requests == 4 && skips[3] == 4 && vtN == 121 && desc_vertudo_fim());
  desc_vertudo_fonte(&source); waitDone(); assert(requests == 4);
  hideWatched = 0; assert(desc_vertudo_n() == 121);
  hideWatched = 1; expectOrder(traktVisible, 1);
  puts("PASS collection source: cached all-hidden source resumes only on explicit reopen, bounded requests and reversible raw cache");

  watched("ttOver", 50); resetRequests();
  desc_vertudo_abrir("https://overflow.invalid", "movie", "top100"); waitDone();
  assert(vtN == 100 && desc_vertudo_n() == 50 && desc_vertudo_fim() && requests == 1);
  CatItem first, last;
  assert(desc_vertudo_item(0, &first) && !strcmp(first.imdb, "ttOver50"));
  assert(desc_vertudo_item(49, &last) && !strcmp(last.imdb, "ttOver99"));
  hideWatched = 0; assert(desc_vertudo_n() == 100);
  assert(desc_vertudo_item(0, &first) && !strcmp(first.imdb, "ttOver0"));
  puts("PASS overflow: raw ranking cap stays bounded, projection preserves hidden raw entries");

  hideWatched = 1; resetRequests();
  desc_vertudo_abrir("https://retry.invalid", "movie", "rank"); waitDone();
  assert(desc_vertudo_erro() && !desc_vertudo_fim() && vtPagina == 0);
  desc_vertudo_mais(); waitDone();
  assert(!desc_vertudo_erro() && desc_vertudo_n() == 2 && requests == 2 && skips[1] == 0);
  puts("PASS request failure: retry retains raw cursor and does not report exhaustion");

  resetRequests(); holdEntered = holdRelease = 0;
  desc_vertudo_abrir("https://hold.invalid", "movie", "rank");
  pthread_mutex_lock(&fixtureLock);
  while (!holdEntered) pthread_cond_wait(&fixtureSignal, &fixtureLock);
  pthread_mutex_unlock(&fixtureLock);
  desc_vertudo_abrir("https://new.invalid", "movie", "rank");
  pthread_mutex_lock(&fixtureLock); holdRelease = 1;
  pthread_cond_broadcast(&fixtureSignal); pthread_mutex_unlock(&fixtureLock);
  waitDone();
  static const char *newCatalog[] = {"ttNew0", "ttNew1"}; expectOrder(newCatalog, 2);
  assert(requests == 2);
  puts("PASS generation: a late page from an old catalog cannot enter the new catalog");
  puts("watched_discovery: all scenarios passed");
  return 0;
}
