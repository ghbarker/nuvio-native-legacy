/* Explicit session infrastructure for three unchanged, reduced regressions.
 * Their existing fake profile stays at 1 for the entire test. Real ownership
 * and transition fixtures NEVER link this file: they link actual perfis.c.
 */
#include "perfis.h"
#include <assert.h>
#include <pthread.h>

static const uint64_t fixtureSession = 1;
static pthread_mutex_t fixtureGate;
static pthread_once_t fixtureGateOnce = PTHREAD_ONCE_INIT;
static _Thread_local unsigned fixtureLocks;

static void initializeGate(void) {
  pthread_mutexattr_t attributes;
  assert(pthread_mutexattr_init(&attributes) == 0);
  assert(pthread_mutexattr_settype(&attributes, PTHREAD_MUTEX_RECURSIVE) == 0);
  assert(pthread_mutex_init(&fixtureGate, &attributes) == 0);
  assert(pthread_mutexattr_destroy(&attributes) == 0);
}

uint64_t perfis_sessao_geracao(void) {
  assert(perfis_ativo() == 1);
  return fixtureSession;
}

int perfis_sessao_atual(uint64_t sessao) {
  return sessao == fixtureSession && perfis_ativo() == 1;
}

int perfis_sessao_travar(uint64_t sessao) {
  assert(pthread_once(&fixtureGateOnce, initializeGate) == 0);
  assert(pthread_mutex_lock(&fixtureGate) == 0);
  if (!perfis_sessao_atual(sessao)) {
    assert(pthread_mutex_unlock(&fixtureGate) == 0);
    return 0;
  }
  fixtureLocks++;
  return 1;
}

void perfis_sessao_destravar(void) {
  assert(fixtureLocks > 0);
  fixtureLocks--;
  assert(pthread_mutex_unlock(&fixtureGate) == 0);
}
