#ifndef PRELOAD_FIXTURE_SESSION_H
#define PRELOAD_FIXTURE_SESSION_H
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>
static _Atomic uint64_t fixtureSession = 1;
static pthread_mutex_t fixtureSessionGate = PTHREAD_MUTEX_INITIALIZER;
uint64_t perfis_sessao_geracao(void) { return atomic_load(&fixtureSession); }
int perfis_sessao_atual(uint64_t s) { return s == atomic_load(&fixtureSession); }
int perfis_sessao_travar(uint64_t s) {
  pthread_mutex_lock(&fixtureSessionGate);
  if (perfis_sessao_atual(s)) return 1;
  pthread_mutex_unlock(&fixtureSessionGate); return 0;
}
void perfis_sessao_destravar(void) { pthread_mutex_unlock(&fixtureSessionGate); }
#endif
