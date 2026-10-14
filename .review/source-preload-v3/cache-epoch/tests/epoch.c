#include "perfis.h"
#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>

/* Zero initially preserves the original reduced fixture's absent-producer
 * contract. The added cache tests then advance real typed fixture epochs.
 */
static _Atomic uint64_t epoch;
static _Atomic int invalidateOnTick;
static pthread_mutex_t gate;
static pthread_once_t gateOnce = PTHREAD_ONCE_INIT;
static void initializeGate(void) {
  pthread_mutexattr_t attr;
  assert(!pthread_mutexattr_init(&attr));
  assert(!pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE));
  assert(!pthread_mutex_init(&gate, &attr));
  assert(!pthread_mutexattr_destroy(&attr));
}
uint64_t perfis_sessao_geracao(void) { return atomic_load(&epoch); }
int perfis_sessao_atual(uint64_t e) { return e == atomic_load(&epoch); }
int perfis_sessao_travar(uint64_t e) {
  pthread_once(&gateOnce, initializeGate);
  pthread_mutex_lock(&gate);
  if (perfis_sessao_atual(e)) return 1;
  pthread_mutex_unlock(&gate);
  return 0;
}
void perfis_sessao_destravar(void) { pthread_mutex_unlock(&gate); }
void fixture_epoch_advance(void) {
  pthread_once(&gateOnce, initializeGate);
  pthread_mutex_lock(&gate);
  atomic_fetch_add(&epoch, 1);
  pthread_mutex_unlock(&gate);
}
void fixture_epoch_flip_on_tick(void) { atomic_store(&invalidateOnTick, 1); }
void fixture_epoch_tick(void) {
  if (atomic_exchange(&invalidateOnTick, 0)) fixture_epoch_advance();
}
