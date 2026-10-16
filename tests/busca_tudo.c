// Actual Search preview, See All worker and metadata-provider paging.
// Queue grid pthread dispatch; reuse the metadata fixture's HTTP boundaries
// and control its profile epoch. No network, SDL window or video.
#include <pthread.h>
#include <assert.h>
#include <stdlib.h>
static void *(*queued)(void *);
static void *queuedArg;
static int queueCreate(pthread_t *t, const pthread_attr_t *a,
                       void *(*fn)(void *), void *arg) {
  (void)a;
  assert(!queued);
  *t = pthread_self(); queued = fn; queuedArg = arg;
  return 0;
}
static int queueDetach(pthread_t t) {
  assert(pthread_equal(t, pthread_self())); return 0;
}
#define pthread_create queueCreate
#define pthread_detach queueDetach
#define NV_TEST_CUSTOM_PROFILE_SESSION
#define NV_PERFIL_SESSAO_FORTE
#define main metadata_fixture_main
#include "detalheanime.c"
#undef main
#undef pthread_create
#undef pthread_detach

static uint64_t epoch = 1;
uint64_t perfis_sessao_geracao(void) { return epoch; }
int perfis_sessao_atual(uint64_t s) { return s == epoch; }
int perfis_sessao_travar(uint64_t s) { return s == epoch; }
void perfis_sessao_destravar(void) {}

static const char *empty = "{\"metas\":[]}";
static const char *query = "star & moon";
static const char *base = "https://search.test/config?apikey=fixture";
static char *page(int first, int count, int malformed) {
  size_t cap = (size_t)(count + malformed + 1) * 180 + 32;
  char *s = malloc(cap); assert(s);
  size_t k = (size_t)snprintf(s, cap, "{\"metas\":[");
  for (int i = 0; i < malformed + count; i++) {
    if (i) k += (size_t)snprintf(s + k, cap - k, ",");
    if (i < malformed) k += (size_t)snprintf(s + k, cap - k, "{\"name\":\"Missing ID\"}");
    else {
      int id = first + i - malformed;
      k += (size_t)snprintf(s + k, cap - k,
        "{\"id\":\"%s%d\",\"name\":\"Result %d\",\"type\":\"series\",\"poster\":\"https://p/%d.jpg\"}",
        id % 2 ? "tmdb:" : "tt", id, id, id);
    }
    assert(k < cap);
  }
  assert(k + 3 <= cap); snprintf(s + k, cap - k, "]}");
  return s;
}
static void network(void) {
  nPedidos = nRotas = 0; pedidoMetaHook = NULL;
  metaprov_zerar_pausa();
}
static void targets(int builtin) {
  assert(!queued);
  network(); fakeIdioma = "en-US";
  desc_alvos_busca_zerar();
  if (!builtin) {
    nAlvos = 0;
    desc_alvo_busca_paginavel(base, "series", "results", "Popular", "Fixture", 0);
  } else { alvos[0] = alvos[1]; nAlvos = 1; }
  buscaSessao = epoch;
  snprintf(buscaPedido, sizeof buscaPedido, "%s", query);
  buscaTermo[0] = 0; respGeracao = -1;
}
static void preview(void) {
  proximoAlvo = 0; fiosVivos = 1;
  fioBusca(NULL);
  assert(!fiosVivos && desc_busca_chegou(query));
}
static void dispatch(void) {
  assert(queued);
  void *(*fn)(void *) = queued; void *arg = queuedArg;
  queued = NULL; queuedArg = NULL;
  assert(!fn(arg));
  assert(!queued && !desc_vertudo_carregando());
}
static void openTarget(int i) {
  assert(desc_vertudo_busca(i, desc_busca_geracao(), query));
  dispatch();
}
static void more(void) { desc_vertudo_mais(); dispatch(); }
static void identities(int first, int n) {
  CatItem *it = malloc(sizeof *it); assert(it);
  assert(desc_vertudo_n() == n && desc_vertudo_n_bruto() == n);
  for (int i = 0; i < n; i++) {
    char id[40]; int num = first + i;
    snprintf(id, sizeof id, "%s%d", num % 2 ? "tmdb:" : "tt", num);
    assert(desc_vertudo_item(i, it));
    assert(!strcmp(it->imdb, id) && !strcmp(it->tipo, "series"));
  }
  assert(!desc_vertudo_item(n, it)); free(it);
}
static void noSkip(void) {
  for (int n = 25; n <= 40; n += 15) {
    char *p = page(1000, n, 0);
    targets(0); rota("search.test", p); preview();
    assert(desc_busca_alvo_n(0, query) == 24 && desc_busca_alvo_mais(0, query));
    nPedidos = 0; openTarget(0); identities(1000, n);
    assert(nPedidos == 1 && desc_vertudo_fim() && !desc_vertudo_erro());
    assert(strstr(pedidos[0], "/catalog/series/results/search=star%20%26%20moon.json?apikey=fixture"));
    int before = nPedidos; desc_vertudo_mais();
    assert(!queued && nPedidos == before);
    free(p);
  }
  puts("ok busca_tudo: no-skip response retains 25th/40th mixed IDs and provider order");
}
static void paging(int malformed) {
  char *a = page(2000, 20, malformed), *b = page(2020, 20, 0);
  char skip[32], end[32];
  snprintf(skip, sizeof skip, "skip=%d.json", 20 + malformed);
  snprintf(end, sizeof end, "skip=%d.json", 40 + malformed);
  targets(0); alvos[0].paginavel = 1;
  rota(end, empty); rota(skip, b); rota("search.test", a); preview();
  assert(desc_busca_alvo_n(0, query) == 20 && desc_busca_alvo_mais(0, query));
  nPedidos = 0; openTarget(0); identities(2000, 20);
  assert(!desc_vertudo_fim() && desc_vertudo_pagina() == 20 + malformed);
  more(); identities(2000, 40);
  assert(strstr(pedidos[1], skip) && desc_vertudo_pagina() == 40 + malformed);
  more(); assert(desc_vertudo_fim() && !desc_vertudo_erro() && nPedidos == 3);
  identities(2000, 40); desc_vertudo_mais(); assert(!queued && nPedidos == 3);
  free(a); free(b);
}
static void failures(void) {
  char *p = page(3000, 20, 0);
  targets(0); alvos[0].paginavel = 1;
  rota("search.test", p); preview(); nPedidos = 0;
  openTarget(0); more(); identities(3000, 20);
  assert(desc_vertudo_fim() && nPedidos == 2); // addon ignores skip
  desc_vertudo_mais(); assert(!queued && nPedidos == 2);

  targets(0); alvos[0].paginavel = 1; rota("search.test", p); preview();
  network(); rota("search.test", "{\"error\":\"not a catalog\"}");
  openTarget(0); assert(desc_vertudo_erro() && !desc_vertudo_fim() && !desc_vertudo_n());
  network(); rota("search.test", p); more(); identities(3000, 20);
  network(); rota("search.test", NULL); more();
  assert(desc_vertudo_erro() && !desc_vertudo_fim()); identities(3000, 20);
  network(); rota("search.test", empty); more();
  assert(desc_vertudo_fim() && !desc_vertudo_erro()); identities(3000, 20);
  free(p);
  puts("ok busca_tudo: ignored skip, empty end and retryable error are bounded");
}
static void sourceIdentity(void) {
  char *a = page(4000, 25, 0), *b = page(5000, 25, 0);
  targets(0);
  desc_alvo_busca_paginavel("https://other.test", "series", "other", "Popular", "Fixture", 0);
  rota("other.test", b); rota("search.test", a); preview(); nPedidos = 0;
  openTarget(1); identities(5000, 25);
  assert(strstr(pedidos[0], "other.test/catalog/series/other/search="));
  openTarget(0); identities(4000, 25); assert(nPedidos == 2);
  assert(desc_vertudo_busca(0, geracao, query) && !queued && nPedidos == 2);

  network(); rota("/results.json", b); rota("search.test", a);
  desc_vertudo_abrir(base, "series", "results"); dispatch(); identities(5000, 25);
  assert(!vtBuscaAtiva && !strstr(pedidos[0], "search="));
  openTarget(0); identities(4000, 25);
  assert(vtBuscaAtiva && strstr(pedidos[1], "search="));
  free(a); free(b);
  puts("ok busca_tudo: duplicate labels preserve source; ordinary/search caches stay separate");
}
static int mutate;
static void changeDuringHttp(const char *url) {
  assert(strstr(url, "search=")); pedidoMetaHook = NULL;
  if (mutate == 0) geracao++;
  if (mutate == 1) snprintf(buscaPedido, sizeof buscaPedido, "changed query");
  if (mutate == 2) snprintf(alvos[0].id, sizeof alvos[0].id, "changed-source");
  if (mutate == 3) buscaSessao = 2;
}
static void stale(void) {
  char *p = page(6000, 25, 0);
  targets(0); rota("search.test", p); preview(); nPedidos = 0;
  assert(!desc_vertudo_busca(0, geracao - 1, query));
  assert(!desc_vertudo_busca(0, geracao, "other"));
  assert(!desc_vertudo_busca(-1, geracao, query));
  assert(!desc_vertudo_busca(0, geracao, NULL));
  buscaSessao = 2;
  assert(!desc_vertudo_busca(0, geracao, query) && !queued && !nPedidos);
  buscaSessao = 1;
  assert(desc_vertudo_busca(0, geracao, query));
  vtBusca.addonsVersao++; // fixed-current-version=1 boundary rejects captured 2
  dispatch(); assert(!nPedidos && desc_vertudo_erro() && desc_vertudo_fim());
  for (mutate = 0; mutate < 4; mutate++) {
    targets(0); rota("search.test", p); preview(); nPedidos = 0;
    pedidoMetaHook = changeDuringHttp;
    openTarget(0);
    assert(nPedidos == 1 && !desc_vertudo_n() && desc_vertudo_erro() && desc_vertudo_fim());
  }
  buscaSessao = 1; free(p);
  puts("ok busca_tudo: stale query, generation, source, captured profile/version cannot publish");
}
static void builtin(void) {
  char *a = page(7000, 20, 0), *b = page(7020, 20, 0);
  targets(1); fakeIdioma = "fr-FR";
  rota("skip=20", b); rota("catalog.nuvio.tv", a); preview();
  assert(desc_busca_alvo_mais(0, query)); nPedidos = 0;
  openTarget(0); assert(vtBuscaProvedor == METAPROV_NUVIO);
  fakeIdioma = "de-DE"; more(); identities(7000, 40);
  assert(strstr(pedidos[0], "%22fr-FR%22") && strstr(pedidos[1], "%22fr-FR%22"));
  assert(strstr(pedidos[1], "popular-series/search=star%20%26%20moon&skip=20.json"));
  network(); rota("catalog.nuvio.tv", b);
  openTarget(0); identities(7020, 20); // same query, new locale is a new cache identity
  assert(strstr(pedidos[0], "%22de-DE%22"));

  targets(1); rota("catalog.nuvio.tv", a); preview();
  network(); rota("catalog.nuvio.tv", NULL); rota("v3-cinemeta", a);
  openTarget(0); assert(vtBuscaProvedor == METAPROV_CINEMETA && nPedidos == 2);
  network(); rota("catalog.nuvio.tv", b); rota("v3-cinemeta", b); more();
  identities(7000, 40); assert(nPedidos == 1 && strstr(pedidos[0], "v3-cinemeta"));

  targets(1); rota("catalog.nuvio.tv", a); preview(); nPedidos = 0; openTarget(0);
  network(); rota("catalog.nuvio.tv", NULL); rota("v3-cinemeta", b); more();
  assert(nPedidos == 1 && desc_vertudo_erro() && vtBuscaProvedor == METAPROV_NUVIO);
  identities(7000, 20); // pinned provider cannot change after an accepted first page
  network(); rota("catalog.nuvio.tv", b); more(); identities(7000, 40);

  targets(1); rota("catalog.nuvio.tv", a); preview();
  network(); rota("catalog.nuvio.tv", empty); rota("v3-cinemeta", b); openTarget(0);
  assert(nPedidos == 1 && desc_vertudo_fim() && !desc_vertudo_n() && !desc_vertudo_erro());
  int provider = -1; nPedidos = 0;
  char *bad = metaprov_busca_pagina_com("series", query, "fr-FR", 20, 5, 6,
                                       metaprov_get_rede, NULL, &provider);
  assert(!bad && !nPedidos && provider == -1);
  free(a); free(b);
  puts("ok busca_tudo: localized URLs, responder pinning, fallback and valid empty semantics");
}
static const char *newQuery = "next profile";
static void *incomingPreview(void *arg) {
  (void)arg; fioBusca(NULL);
  return NULL;
}
static void switchWhileInFlight(const char *url) {
  assert(strstr(url, "search=star%20%26%20moon"));
  pedidoMetaHook = NULL;
  assert(vtFioVivo && vtSessao == 1);
  epoch = 2;
  desc_alvos_busca_zerar(); nAlvos = 0;
  desc_alvo_busca_paginavel(base, "series", "results", "Popular", "Fixture", 0);
  snprintf(buscaPedido, sizeof buscaPedido, "%s", newQuery);
  proximoAlvo = 0; fiosVivos = 1;
  // A real separate preview thread preserves the outgoing grid thread's TLS.
  pthread_t incoming;
  assert(!pthread_create(&incoming, NULL, incomingPreview, NULL));
  assert(!pthread_join(incoming, NULL));
  assert(desc_busca_chegou(newQuery));
  assert(desc_vertudo_busca(0, geracao, newQuery));
  assert(!queued && vtFioVivo); // the outgoing worker still owns dispatch
}
static void overlappingProfile(void) {
  char *old = page(8000, 25, 0), *next = page(9000, 25, 0);
  targets(0);
  rota("search=next%20profile", next);
  rota("search=star%20%26%20moon", old);
  preview(); nPedidos = 0;
  assert(desc_vertudo_busca(0, geracao, query));
  pedidoMetaHook = switchWhileInFlight;
  dispatch(); identities(9000, 25);
  assert(nPedidos == 3 && !desc_vertudo_erro() && desc_vertudo_fim());
  assert(vtSessao == 2 && vtBusca.sessao == 2);
  assert(strstr(pedidos[0], "search=star%20%26%20moon"));
  assert(strstr(pedidos[1], "search=next%20profile") && strstr(pedidos[2], "search=next%20profile"));
  epoch = 1; free(old); free(next);
  puts("ok busca_tudo: in-flight old profile yields worker to new profile without stale rows or stranding");
}
int main(void) {
  noSkip(); paging(0); paging(2);
  puts("ok busca_tudo: 20+20 pages use raw offsets, including rejected metadata");
  failures(); sourceIdentity(); stale(); builtin(); overlappingProfile();
  assert(!queued);
  NvPerfilSessao *s = pthread_getspecific(descSessaoChave);
  assert(!pthread_setspecific(descSessaoChave, NULL)); free(s);
  puts("busca_tudo: PASS");
  return 0;
}
