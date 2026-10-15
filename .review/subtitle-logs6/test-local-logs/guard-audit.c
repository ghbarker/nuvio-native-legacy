// Exercise original report scheduling without executing the worker or making network calls.
#include <assert.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
static int scheduled, automatic, offline;
static void *last_context;
static inline __attribute__((always_inline)) int audit_pthread_create(pthread_t *t,
    const pthread_attr_t *a, void *(*worker)(void *), void *context) {
  (void)t; (void)a; (void)worker;
  scheduled++; last_context = context;
  return 1; // Scheduling failure path: never starts a worker or accesses network.
}
#define pthread_create audit_pthread_create
#include "avisos.c"
#undef pthread_create
int ajustes_envio_auto(void) { return automatic; }
int rede_saude_offline(void) { return offline; }
const char *registro_arquivo(void) { return NULL; }
int main(void) {
  assert(NV_REC_URL[0]);
  automatic = 0; offline = 0; envioEstado = 0;
  avisos_envio_auto_passo(1000);
  assert(scheduled == 0); // Original saved-Off preference is respected.
  avisos_enviar_registro_atual();
  assert(scheduled == 1 && last_context == &ATUAL && envioEstado == 3);
  envioEstado = 0; enviarAgora();
  assert(scheduled == 2 && last_context == NULL && envioEstado == 3);
  envioEstado = 0; offline = 1;
  avisos_enviar_registro_atual();
  assert(scheduled == 2 && envioEstado == 3 && envPendenteRede);
  offline = 0;
  avisos_envio_auto_passo(2000);
  assert(scheduled == 3 && last_context == &ATUAL && !envPendenteRede);
  char previous[] = "/tmp/nuvio-original-log-test.XXXXXX";
  int fd = mkstemp(previous); assert(fd >= 0); close(fd);
  assert(setenv("NUVIO_LOG_ANTERIOR", previous, 1) == 0);
  automatic = 1; envioEstado = 0;
  avisos_envio_auto_passo(3000);
  assert(scheduled == 4 && last_context == &AUTO_ANTERIOR);
  avisos_envio_auto_passo(4000);
  assert(scheduled == 4); // Original once-per-session behavior.
  unlink(previous);
  puts("PASS original Send/crash/offline retry and saved auto Off/On; no worker/network executed");
  return 0;
}
