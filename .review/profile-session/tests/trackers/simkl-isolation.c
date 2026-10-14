#include "simkl.h"
#include "trakt.h"
#include "profile-stubs.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
static const char *activeToken = "token-A";
const char *simklauth_token(void) { return activeToken; }
const char *nuvem_simkl_cliente(void) { return "fixture-client"; }
const char *nuvem_simkl_app(void) { return "nuvio"; }
void nuvem_url_escapar(const char *v, char *d, unsigned n) { snprintf(d, n, "%s", v); }
const char *i18n(const char *s) { return s; }
const char *idioma_mes_data(int m, const char *s) { (void)m; return s; }
int ajustes_cw_proximo(void) { return 1; }
int ajustes_cw_minimo(void) { return 1; }
int ajustes_cw_concluido(void) { return 90; }
int trakt_enfeitar_lote(CatItem *s, int n) { (void)s; return n; }
int trakt_enfeitar_lote_sessao(CatItem *s, int n, uint64_t epoch) {
  // Preserve the fixture's metadata mock, but reject stale real profile epochs.
  return perfis_sessao_atual(epoch) ? trakt_enfeitar_lote(s, n) : 0;
}

static pthread_mutex_t netLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t netCond = PTHREAD_COND_INITIALIZER;
static const char *delayPath;
static int entered, releaseOld, postsA, postsB;
static int isA(const char *const *cab) { return cab && strstr(cab[0], "Bearer token-A") != NULL; }
static void pauseNetwork(const char *url, const char *const *cab) {
  pthread_mutex_lock(&netLock);
  if (delayPath && strstr(url, delayPath) && isA(cab)) {
    entered = 1; pthread_cond_broadcast(&netCond);
    while (!releaseOld) pthread_cond_wait(&netCond, &netLock);
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
char *rede_baixar_st(const char *url, int s, const char *const *cab, int *st) {
  (void)s; pauseNetwork(url, cab); *st = 200;
  if (strstr(url, "/sync/activities")) return strdup("{\"all\":\"same-activity\"}");
  char out[900]; int a = isA(cab);
  if (strstr(url, "/sync/playback")) {
    snprintf(out, sizeof out, "[{\"id\":%d,\"progress\":40,\"paused_at\":\"2026-10-01T00:00:00Z\",\"type\":\"movie\",\"movie\":{\"title\":\"Owned\",\"ids\":{\"imdb\":\"%s\"}}}]", a ? 101 : 202, a ? "tt1000001" : "tt2000001");
    return strdup(out);
  }
  if (strstr(url, "/shows/watching")) return strdup(a ?
    "{\"shows\":[{\"next_to_watch\":\"S01E02\",\"show\":{\"title\":\"Owned\",\"ids\":{\"imdb\":\"tt1000002\"}}}]}" : "{\"shows\":[]}");
  if (strstr(url, "/movies/plantowatch")) {
    snprintf(out, sizeof out, "{\"movies\":[{\"movie\":{\"title\":\"Owned list\",\"ids\":{\"imdb\":\"%s\"}}}]}", a ? "tt1000003" : "tt2000003"); return strdup(out);
  }
  return strdup("{\"shows\":[]}");
}
char *rede_postar_st(const char *url, int s, const char *const *cab, const char *corpo, int *st) {
  (void)s; (void)corpo;
  pthread_mutex_lock(&netLock); if (isA(cab)) postsA++; else postsB++; pthread_cond_broadcast(&netCond); pthread_mutex_unlock(&netLock);
  pauseNetwork(url, cab); *st = isA(cab) ? 400 : 201; return strdup("{}");
}
char *rede_apagar(const char *url, int s, const char *const *cab, int *st) { (void)s; pauseNetwork(url, cab); *st = 204; return strdup(""); }
#include "simkl.c"
static int oldCount, listRead;
static void *readOld(void *u) { (void)u; CatItem *out = calloc(8, sizeof *out); oldCount = listRead ? simkl_plantowatch(out, 8) : simkl_continuar(out, 8); free(out); return NULL; }
static void openProfile(int n, const char *tk) { perfis_definir_ativo(n); simkl_abrir_sessao(); activeToken = tk; }
static void incoming(const char *path, int list, int roundtrip) {
  pthread_t t; listRead = list; openProfile(1, "token-A");
  pthread_mutex_lock(&netLock); delayPath = path; entered = releaseOld = 0; pthread_mutex_unlock(&netLock);
  pthread_create(&t, NULL, readOld, NULL); waitEntered();
  openProfile(2, "token-B");
  if (roundtrip) openProfile(1, "token-A");
  else {
    CatItem *out = calloc(8, sizeof *out); assert((list ? simkl_plantowatch(out, 8) : simkl_continuar(out, 8)) == 1); free(out);
  }
  releaseNetwork(); pthread_join(t, NULL);
  pthread_mutex_lock(&netLock); delayPath = NULL; pthread_mutex_unlock(&netLock);
  assert(oldCount == 0 && !simkl_na_plantowatch("tt1000003") && !simkl_e_a_seguir("tt1000002:1:2"));
  pthread_mutex_lock(&trava);
  if (roundtrip) assert(!nPlay && !nPtw && !corpoPlay && !corpoPtwFilmes);
  else if (list) assert(nPtw == 1 && !strcmp(ptw[0], "tt2000003"));
  else assert(nPlay == 1 && play[0].id == 202 && !nProx);
  pthread_mutex_unlock(&trava);
  printf("PASS Simkl delayed %s, %s; old bodies and tables rejected\n", path, roundtrip ? "A-B-A" : "A-B");
}
static void outgoing(void) {
  openProfile(1, "token-A");
  pthread_mutex_lock(&netLock); delayPath = "/sync/add-to-list"; entered = releaseOld = 0; pthread_mutex_unlock(&netLock);
  assert(simkl_lista_tipo("tt1000004", "movie", 1)); waitEntered();
  openProfile(2, "token-B"); assert(simkl_lista_tipo("tt2000004", "movie", 1));
  for (int i = 0; i < 300 && simkl_lista_estado() == SMK_OP_PENDENTE; i++) usleep(10000);
  assert(simkl_lista_estado() == SMK_OP_CONFIRMADA);
  releaseNetwork(); for (int i = 0; i < 50; i++) usleep(1000);
  assert(postsA == 1 && postsB == 1 && simkl_lista_estado() == SMK_OP_CONFIRMADA);
  assert(simkl_na_plantowatch("tt2000004") && !simkl_na_plantowatch("tt1000004"));
  puts("PASS Simkl old-account completion cannot modify new-account list or pending action");
}
int main(void) {
  incoming("/sync/activities", 0, 0);
  incoming("/sync/playback", 0, 0);
  incoming("/shows/watching", 0, 0);
  incoming("/shows/plantowatch", 1, 0);
  incoming("/sync/playback", 0, 1);
  outgoing(); return 0;
}
