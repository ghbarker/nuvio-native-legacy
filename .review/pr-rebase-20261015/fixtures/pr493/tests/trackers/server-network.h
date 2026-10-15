#include "rede.h"
#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
struct RedeGrupo { atomic_uint geracao; atomic_int cancelado; };
struct RedeJob { RedeGrupo *grupo; unsigned geracao; int cancelado; };
RedeGrupo *rede_grupo_criar(void) { RedeGrupo *g = calloc(1, sizeof *g); atomic_store(&g->geracao, 1); return g; }
void rede_grupo_soltar(RedeGrupo *g) { free(g); }
uint64_t rede_grupo_avancar(RedeGrupo *g) { return atomic_fetch_add(&g->geracao, 1) + 1; }
void rede_grupo_cancelar(RedeGrupo *g) { atomic_store(&g->cancelado, 1); }
RedeJob *rede_job_criar(RedeGrupo *g) { RedeJob *j = calloc(1, sizeof *j); j->grupo = g; j->geracao = atomic_load(&g->geracao); return j; }
void rede_job_soltar(RedeJob *j) { free(j); }
void rede_job_cancelar(RedeJob *j) { if (j) j->cancelado = 1; }
RedeErro rede_job_estado(RedeJob *j) {
  if (!j) return REDE_OK;
  if (j->cancelado || atomic_load(&j->grupo->cancelado)) return REDE_CANCELADO;
  return j->geracao == atomic_load(&j->grupo->geracao) ? REDE_OK : REDE_GERACAO;
}
unsigned rede_pedido_capacidades(void) { return REDE_CAP_JOB; }
void rede_resposta_limpar(RedeResposta *r) { free(r->corpo); free(r->cabecalhos); memset(r, 0, sizeof *r); }
static pthread_mutex_t netLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t netCond = PTHREAD_COND_INITIALIZER;
static int requests, blockFirst, entered, released, unauthorized;
int rede_pedir(const RedePedido *p, RedeResposta *r) {
  pthread_mutex_lock(&netLock); requests++;
  assert(strstr(p->url, "server-A"));
  int oldToken = 0;
  for (int k = 0; p->cabecalhos && p->cabecalhos[k]; k++) oldToken |= strstr(p->cabecalhos[k], "token-A") != NULL;
  assert(oldToken);
  if (blockFirst && requests == 1) {
    entered = 1; pthread_cond_broadcast(&netCond);
    while (!released) pthread_cond_wait(&netCond, &netLock);
  }
  memset(r, 0, sizeof *r); r->erro = rede_job_estado(p->job); r->status = unauthorized ? 401 : 200;
  r->corpo = strdup("{}"); r->n_corpo = 2;
  pthread_mutex_unlock(&netLock); return 0;
}
static void waitEntered(void) {
  struct timespec until; clock_gettime(CLOCK_REALTIME, &until); until.tv_sec += 3;
  pthread_mutex_lock(&netLock); while (!entered) assert(pthread_cond_timedwait(&netCond, &netLock, &until) == 0); pthread_mutex_unlock(&netLock);
}
static void releaseNetwork(void) { pthread_mutex_lock(&netLock); released = 1; pthread_cond_broadcast(&netCond); pthread_mutex_unlock(&netLock); }
static void networkReset(int blocked, int authFail) { pthread_mutex_lock(&netLock); requests = entered = released = 0; blockFirst = blocked; unauthorized = authFail; pthread_mutex_unlock(&netLock); }
int dados_apagar(const char *name) { (void)name; return 1; }
int perfis_ativo(void) { return 1; }
char *dados_caminho(char *d, unsigned n, const char *name) { snprintf(d, n, "/tmp/%s", name); return d; }
void dados_fs_travar(void) {}
void dados_fs_liberar(void) {}
