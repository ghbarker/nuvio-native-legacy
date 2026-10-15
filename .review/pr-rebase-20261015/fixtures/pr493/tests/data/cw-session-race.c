/* Actual discovery/catalog/progress/CW code, controlled provider fixture.
 * The barrier holds an old CW request after its session was captured.
 * Real perfis.c advances epochs; profile/config/network neighbours are stubs.
 */
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
static pthread_mutex_t contTrava;
static int proofLock(pthread_mutex_t *m);
static int proofUnlock(pthread_mutex_t *m);
#define pthread_mutex_lock(m) proofLock(m)
#define pthread_mutex_unlock(m) proofUnlock(m)
#define perfis_ativo fixture_perfis_ativo
#define main fixture_cwremover_main
#include "cwremover.c"
#undef main
#undef perfis_ativo
#undef pthread_mutex_lock
#undef pthread_mutex_unlock
extern int perfis_ativo(void);
void fil_definir_perfil(int p) { (void)p; }
void fontepref_definir_perfil(int p) { (void)p; }
void legmem_definir_perfil(int p) { (void)p; }
void arteesc_definir_perfil(int p) { (void)p; }
void plugins_perfil_mudou(void) {}
void servidores_perfil_trocou(void) {}

static pthread_mutex_t barrierMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t barrierCv = PTHREAD_COND_INITIALIZER;
static int arrived, released;
static _Thread_local int pauseThisWorker;
static int proofLock(pthread_mutex_t *m) {
  return pthread_mutex_lock(m);
}
static int proofUnlock(pthread_mutex_t *m) {
  int r = pthread_mutex_unlock(m);
  if (pauseThisWorker && m == &contTrava) {
    pauseThisWorker = 0;
    pthread_mutex_lock(&barrierMutex);
    arrived = 1;
    pthread_cond_broadcast(&barrierCv);
    while (!released) pthread_cond_wait(&barrierCv, &barrierMutex);
    pthread_mutex_unlock(&barrierMutex);
  }
  return r;
}
static void *pausedCW(void *u) { pauseThisWorker = 1; return fioContinuar(u); }
static void waitForOldRequest(void) {
  pthread_mutex_lock(&barrierMutex);
  while (!arrived) pthread_cond_wait(&barrierCv, &barrierMutex);
  pthread_mutex_unlock(&barrierMutex);
}
static void finishOldRequest(void) {
  pthread_mutex_lock(&barrierMutex);
  released = 1;
  pthread_cond_broadcast(&barrierCv);
  pthread_mutex_unlock(&barrierMutex);
}
static void publishDestination(void) {
  CatItem destination = {0};
  CatFileira row = {0};
  snprintf(destination.imdb, sizeof destination.imdb, "ttDestination");
  snprintf(destination.tipo, sizeof destination.tipo, "movie");
  snprintf(destination.titulo, sizeof destination.titulo, "Destination catalog");
  snprintf(row.chave, sizeof row.chave, "destination_movie_popular");
  row.n = 1;
  cat_definir_tudo(&destination, 1, &row, 1);
}
static void runCase(int mode) {
  pthread_t worker;
  NvPerfilSessao *old = malloc(sizeof *old);
  NvPerfilSessao oldEpoch;
  const char *label = mode == 0 ? "A -> B" : mode == 1 ? "A -> B -> A" : "A -> A fresh opening";
  assert(old);
  perfis_definir_ativo(1);
  *old = perfis_sessao_geracao();
  oldEpoch = *old;
  arrived = released = 0;
  cwDeNovo = 0; cwVivo = 1;
  assert(pthread_create(&worker, NULL, pausedCW, old) == 0);
  waitForOldRequest();
  if (mode != 2) perfis_definir_ativo(2);
  if (mode == 1 || mode == 2) perfis_definir_ativo(1);
  assert(!perfis_sessao_atual(oldEpoch));
  {
    CatItem stale = {0};
    assert(trakt_enfeitar_lote_sessao(&stale, 1, oldEpoch) == 0);
    assert(trakt_enfeitar_lote_sessao(&stale, 1, perfis_sessao_geracao()) == 1);
  }
  // Relevant real clearing API, without enqueueing a second CW fixture request.
  desc_abrir_sessao();
  cwDeNovo = 0;
  publishDestination();
  finishOldRequest();
  pthread_join(worker, NULL);
  assert(cat_n() == 1);
  assert(!strcmp(cat_item(0)->imdb, "ttDestination"));
  assert(nContinuar() == 0);
  assert(cwVivo == 0);
  printf("PASS: %s rejects the old CW publication and retains destination catalog\n", label);
}
int main(void) {
  runCase(0); runCase(1); runCase(2);
  puts("CW session race: all three controlled transitions pass");
  return 0;
}
