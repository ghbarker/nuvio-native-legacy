// Preserve upstream #493's request-session guard across this feature rebase.
#define main original_watched_discovery_main
#include "watched_discovery.c"
#undef main
#include <stdatomic.h>
static _Atomic uint64_t profileEpoch = 1;
uint64_t perfis_sessao_geracao(void) { return atomic_load(&profileEpoch); }
int perfis_sessao_atual(uint64_t epoch) { return epoch == atomic_load(&profileEpoch); }
int perfis_sessao_travar(uint64_t epoch) { return perfis_sessao_atual(epoch); }
void perfis_sessao_destravar(void) {}
int main(void) {
  hideWatched = 1; hideFuture = 0;
  cat_historico_contexto("profile-epoch-review", 1);
  resetRequests(); holdEntered = holdRelease = 0;
  desc_vertudo_abrir("https://hold.invalid", "movie", "profile-late-page");
  struct timespec limit; clock_gettime(CLOCK_REALTIME, &limit); limit.tv_sec += 5;
  pthread_mutex_lock(&fixtureLock);
  while (!holdEntered) assert(!pthread_cond_timedwait(&fixtureSignal, &fixtureLock, &limit));
  // A -> B -> A: same profile ID, different request epoch. Do not change
  // catalog generation, which would independently reject the old response.
  unsigned catalogGeneration = vtGeracao;
  atomic_store(&profileEpoch, 3);
  holdRelease = 1; pthread_cond_broadcast(&fixtureSignal);
  pthread_mutex_unlock(&fixtureLock);
  waitDone();
  assert(vtGeracao == catalogGeneration && requests == 1);
  assert(vtN == 0 && desc_vertudo_n() == 0 && !desc_vertudo_carregando());
  puts("PASS profile-session A->B->A rejects held page without relying on catalog generation");
  return 0;
}
