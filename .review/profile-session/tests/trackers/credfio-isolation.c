#include "credfio.h"
#include "sync.h"
#include "profile-stubs.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static int oldEntered, releaseOld, aSent, bSent;
int sync_credencial_pedido(SyncCredencialPedido *p) {
  p->sessao = perfis_sessao_geracao(); p->perfil = perfis_ativo();
  snprintf(p->usuario, sizeof p->usuario, "%s", sessao_usuario()); return 1;
}
int sync_empurrar_credencial_pedido(const SyncCredencialPedido *p, const char *prov, const char *json) {
  (void)prov;
  pthread_mutex_lock(&lock);
  if (p->perfil == 1) {
    assert(!strcmp(json, "old-json")); aSent++; oldEntered = 1; pthread_cond_broadcast(&cond);
    while (!releaseOld) pthread_cond_wait(&cond, &lock);
  } else { assert(p->perfil == 2 && !strcmp(json, "new-json")); bSent++; }
  pthread_mutex_unlock(&lock); return p->perfil == 1 ? -1 : 1;
}
int sync_empurrar_credencial(const char *prov, const char *json) { (void)prov; (void)json; assert(!"unsafe legacy sender called"); return 0; }
int main(void) {
  perfis_definir_ativo(1);
  assert(credfio_iniciar("trakt", "old-json"));
  struct timespec until; clock_gettime(CLOCK_REALTIME, &until); until.tv_sec += 3;
  pthread_mutex_lock(&lock); while (!oldEntered) assert(pthread_cond_timedwait(&cond, &lock, &until) == 0); pthread_mutex_unlock(&lock);
  perfis_definir_ativo(2); assert(credfio_iniciar("trakt", "new-json"));
  int res = 0, ready = 0;
  for (int i = 0; i < 300 && !(ready = credfio_resultado("trakt", &res)); i++) usleep(10000);
  assert(ready && res == 1);
  pthread_mutex_lock(&lock); releaseOld = 1; pthread_cond_broadcast(&cond); pthread_mutex_unlock(&lock);
  usleep(100000); assert(!credfio_resultado("trakt", &res));
  pthread_mutex_lock(&lock); assert(aSent == 1 && bSent == 1); pthread_mutex_unlock(&lock);
  puts("PASS credential push captures original account/profile, old worker does not block new profile or overwrite result");
  return 0;
}
