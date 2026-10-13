// Deterministic delayed network integration; actual tracker and profile opening code.
#include "trakt.h"
#include "traktscrobble.h"
#include "tirarremoto.h"
#include "profile-stubs.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
unsigned int SDL_GetTicks(void) { return 0; }
void marco(const char *m) { (void)m; }
static pthread_mutex_t netLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t netCond = PTHREAD_COND_INITIALIZER;
static const char *delayPath;
static int entered, releaseOld, old401, postsA, postsB;
static int deletesA, deletesB, hiddenA, hiddenB, accountProfile;
static uint64_t accountEpoch;
static void (*callback401)(const char *);
static int isA(const char *const *cab) { return cab && cab[0] && strstr(cab[0], "Bearer token-A") != NULL; }
static void pauseNetwork(const char *url, const char *const *cab) {
  pthread_mutex_lock(&netLock);
  if (delayPath && strstr(url, delayPath) && isA(cab)) {
    entered = 1; pthread_cond_broadcast(&netCond);
    while (!releaseOld) pthread_cond_wait(&netCond, &netLock);
    if (old401 && callback401) callback401(url);
  }
  pthread_mutex_unlock(&netLock);
}
static void waitEntered(void) {
  struct timespec until; clock_gettime(CLOCK_REALTIME, &until); until.tv_sec += 3;
  pthread_mutex_lock(&netLock);
  while (!entered) assert(pthread_cond_timedwait(&netCond, &netLock, &until) == 0);
  pthread_mutex_unlock(&netLock);
}
static void releaseNetwork(void) {
  pthread_mutex_lock(&netLock); releaseOld = 1; pthread_cond_broadcast(&netCond); pthread_mutex_unlock(&netLock);
}
static void delay(const char *path, int unauthorized) {
  pthread_mutex_lock(&netLock); delayPath = path; entered = releaseOld = 0; old401 = unauthorized; pthread_mutex_unlock(&netLock);
}
char *rede_baixar_com(const char *url, int s, const char *const *cab) {
  (void)s; pauseNetwork(url, cab);
  if (strstr(url, "/sync/playback")) {
    char out[1400];
    snprintf(out, sizeof out, "[{\"id\":%d,\"progress\":40,\"paused_at\":\"2026-10-01T00:00:00Z\","
      "\"movie\":{\"title\":\"Owned movie\",\"year\":2026,\"runtime\":100,\"overview\":\"Owned.\","
      "\"ids\":{\"imdb\":\"%s\"},\"images\":{\"poster\":[\"x.test/p\"],\"fanart\":[\"x.test/f\"]}}}]",
      isA(cab) ? 101 : 202, isA(cab) ? "tt1000001" : "tt2000001");
    return strdup(out);
  }
  if (strstr(url, "/sync/history")) return strdup(isA(cab) ?
    "[{\"watched_at\":\"2026-10-01T00:00:00Z\",\"episode\":{\"season\":1,\"number\":1},\"show\":{\"ids\":{\"imdb\":\"tt1000002\"}}}]" : "[]");
  if (strstr(url, "/progress/watched")) return strdup("{\"next_episode\":{\"season\":1,\"number\":2}}");
  return NULL;
}
char *rede_baixar(const char *u, int s) { (void)u; (void)s; return strdup("{\"imdbRating\":\"8.0\",\"videos\":[{\"id\":\"tt1000002:1:2\"}]}"); }
char *rede_baixar_st(const char *u, int s, const char *const *cab, int *st) { (void)u; (void)s; (void)cab; *st = 0; return NULL; }
char *rede_apagar(const char *u, int s, const char *const *cab, int *st) {
  (void)s; pauseNetwork(u, cab);
  pthread_mutex_lock(&netLock); if (isA(cab)) { deletesA++; assert(strstr(u, "/101")); } else deletesB++; pthread_mutex_unlock(&netLock);
  *st = 204; return strdup("");
}
int syncprog_remover_sessao(const char *key, int profile, uint64_t epoch, const char *owner) {
  assert(!strcmp(key, "tt1000001") && !strcmp(owner, "fixture-household")); accountProfile = profile; accountEpoch = epoch; return 1;
}
int syncprog_remover(const char *key) { (void)key; assert(!"unscoped account removal"); return 0; }
char *rede_postar_st(const char *u, int s, const char *const *cab, const char *body, int *st) {
  (void)s; (void)body;
  pthread_mutex_lock(&netLock);
  if (strstr(u, "/users/hidden/")) { if (isA(cab)) hiddenA++; else hiddenB++; }
  pthread_mutex_unlock(&netLock);
  pthread_mutex_lock(&netLock); if (isA(cab)) postsA++; else postsB++; pthread_cond_broadcast(&netCond); pthread_mutex_unlock(&netLock);
  pauseNetwork(u, cab); *st = isA(cab) ? 400 : 201; return strdup("{}");
}
void rede_avisar_401(void (*cb)(const char *)) { callback401 = cb; }
int cwo_conta_a_seguir(const char *id) { (void)id; return 0; }
void cwo_conta_trocar(const char *a, const char *b) { (void)a; (void)b; }
void cwo_marcar_estreia(const char *id, long long ms) { (void)id; (void)ms; }
int cwo_virada_aceita(long long a, long long b) { (void)a; (void)b; return 1; }
int ajustes_tmdb_cw(void) { return 0; }
const char *ajustes_tmdb_idioma(void) { return "en-US"; }
const char *desc_chave_tmdb(void) { return ""; }
const char *desc_tmdb_idioma(void) { return "en"; }
const char *i18n(const char *s) { return s; }
unsigned long long cat_historico_geracao(void) { return 1; }
int cat_historico_definir_se_geracao(const char *a, const char *b, int v, unsigned long long g) { (void)a; (void)b; (void)v; return g == 1; }
const char *cat_tipo_por_imdb(const char *id) { (void)id; return "movie"; }
#include "trakt.c"
static int oldCount;
static void *readOld(void *u) { (void)u; CatItem *out = calloc(8, sizeof *out); oldCount = trakt_continuar(out, 8); free(out); return NULL; }
static void openProfile(int profile, const char *tk) {
  perfis_definir_ativo(profile); trakt_abrir_sessao(); trakt_definir(tk, "fixture-client");
}
static void clearDelay(void) { pthread_mutex_lock(&netLock); delayPath = NULL; pthread_mutex_unlock(&netLock); }
static void incoming(int roundtrip, const char *route, int unauthorized) {
  pthread_t t;
  openProfile(1, "token-A"); delay(route, unauthorized);
  pthread_create(&t, NULL, readOld, NULL); waitEntered();
  uint64_t before = perfis_sessao_geracao();
  openProfile(2, "token-B");
  if (roundtrip) openProfile(1, "token-A");
  assert(perfis_sessao_geracao() != before);
  if (!roundtrip) {
    CatItem *out = calloc(8, sizeof *out); assert(trakt_continuar(out, 8) == 1); free(out);
  }
  releaseNetwork(); pthread_join(t, NULL); clearDelay();
  assert(oldCount == 0 && !trakt_recusada());
  pthread_mutex_lock(&playTrava);
  assert(nPlay == (roundtrip ? 0 : 1)); if (nPlay) assert(play[0].id == 202);
  pthread_mutex_unlock(&playTrava);
  pthread_mutex_lock(&travaCred); assert(nUlt == 0 && nProxIds == 0); pthread_mutex_unlock(&travaCred);
  pthread_mutex_lock(&proxMemTrava); assert(nProxMem == 0); pthread_mutex_unlock(&proxMemTrava);
  printf("PASS delayed %s, %s; stale playback/history/next cache and 401 rejected\n", route, roundtrip ? "A-B-A" : "A-B");
}
static void outgoing(void) {
  openProfile(1, "token-A"); delay("/sync/watchlist", 0);
  assert(trakt_watchlist_tipo("tt1000001", "movie", 1)); waitEntered();
  openProfile(2, "token-B"); assert(trakt_watchlist_tipo("tt2000001", "movie", 1));
  for (int i = 0; i < 300 && trakt_operacao_estado(1) == 1; i++) usleep(10000);
  assert(trakt_operacao_estado(1) == 2);
  releaseNetwork(); for (int i = 0; i < 50; i++) usleep(1000); clearDelay();
  assert(trakt_operacao_estado(1) == 2 && postsA == 1 && postsB == 1);
  puts("PASS outgoing old-account failure cannot overwrite new-account success; destination action starts immediately");
}
static void changedCredential(void) {
  openProfile(1, "token-A");
  delay("/sync/watchlist", 0);
  assert(trakt_watchlist_tipo("tt1000001", "movie", 1)); waitEntered();
  // Relinking/refreshing within a profile must also release the stale busy
  // slot and protect the new operation's completion.
  assert(trakt_definir("token-B", "fixture-client"));
  assert(trakt_watchlist_tipo("tt2000001", "movie", 1));
  for (int i = 0; i < 300 && trakt_operacao_estado(1) == 1; i++) usleep(10000);
  assert(trakt_operacao_estado(1) == 2); releaseNetwork(); clearDelay(); usleep(50000);
  assert(trakt_operacao_estado(1) == 2);
  puts("PASS relinking tracker in same profile invalidates old operation without leaving a permanent busy slot");
}
static void outgoingScrobble(void) {
  openProfile(1, "token-A");
  pthread_mutex_lock(&netLock); postsA = postsB = 0; pthread_mutex_unlock(&netLock);
  delay("/scrobble/start", 0);
  assert(trakt_scrobble(SCR_EV_TOCANDO, "tt1000001", 100, 1000, 100, 0) == SCR_START); waitEntered();
  assert(trakt_scrobble(SCR_EV_SAIU, "tt1000001", 400, 1000, 400, 0) == SCR_PAUSE);
  openProfile(2, "token-B");
  assert(trakt_scrobble(SCR_EV_TOCANDO, "tt2000001", 100, 1000, 100, 0) == SCR_START);
  releaseNetwork();
  for (int i = 0; i < 300; i++) {
    pthread_mutex_lock(&netLock); int complete = postsA == 2 && postsB == 1; pthread_mutex_unlock(&netLock);
    if (complete) break; usleep(10000);
  }
  pthread_mutex_lock(&netLock); assert(postsA == 2 && postsB == 1); pthread_mutex_unlock(&netLock);
  clearDelay(); usleep(20000);
  puts("PASS B's scrobble start preserves A's queued final pause under A's original credentials");
}
static TirarRemotoPedido removal;
static void *removeOld(void *u) { (void)u; tirarremoto_executar_pedido(&removal, "tt1000001", 1); return NULL; }
static void outgoingRemoval(int beforeWorker) {
  openProfile(1, "token-A");
  CatItem *out = calloc(8, sizeof *out); assert(trakt_continuar(out, 8) >= 1); free(out);
  assert(tirarremoto_capturar(&removal, "tt1000001"));
  uint64_t epoch = perfis_sessao_geracao(); pthread_t worker;
  if (!beforeWorker) { delay("/sync/playback/", 0); pthread_create(&worker, NULL, removeOld, NULL); waitEntered(); }
  openProfile(2, "token-B"); out = calloc(8, sizeof *out); assert(trakt_continuar(out, 8) == 1); free(out);
  if (beforeWorker) tirarremoto_executar_pedido(&removal, "tt1000001", 1);
  else { releaseNetwork(); pthread_join(worker, NULL); clearDelay(); }
  assert(deletesB == 0 && hiddenB == 0 && deletesA > 0 && hiddenA > 0);
  assert(accountProfile == 1 && accountEpoch == epoch);
  pthread_mutex_lock(&playTrava); assert(nPlay == 1 && play[0].id == 202 && play[0].chave[0]); pthread_mutex_unlock(&playTrava);
  printf("PASS frozen removal %s: original playback IDs, hide token, account/profile intent, and B cache intact\n", beforeWorker ? "before worker starts" : "between delete and hide");
}
int main(void) {
  incoming(0, "/sync/playback", 0);
  incoming(0, "/progress/watched", 0);
  incoming(1, "/sync/playback", 1);
  outgoing();
  changedCredential();
  outgoingScrobble();
  outgoingRemoval(1);
  outgoingRemoval(0);
  const char *cab[4]; char aut[200], key[140]; assert(trakt_cabecalhos(cab, aut, sizeof aut, key, sizeof key));
  callback401("https://api.trakt.tv/sync/history"); assert(trakt_recusada());
  puts("PASS genuine current-request 401 is still reported");
  return 0;
}
