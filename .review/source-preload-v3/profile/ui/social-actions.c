#define NV_REC_URL "https://fixture.invalid"
#include "common.h"
#include "recomenda.c"
struct SDL_Thread { pthread_t thread; SDL_ThreadFunction fn; void *arg; };
static SDL_Thread *lastThread;
static int failThread, viaTrakt;
static char loginToken[80] = "account-a-token";
static pthread_mutex_t netLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t netCond = PTHREAD_COND_INITIALIZER;
static int blockRequest = 1, waiting, releaseOld, requests;
static int blockPedidos, blockRefresh, getCalls;
static struct { char aut[3200], via[32], perfil[40], path[600]; } seen[8];
int ajustes_social(void) { return 1; }
const char *i18n(const char *x) { return x; }
int trakt_ativo(void) { return viaTrakt; }
int trakt_cabecalhos(const char **cab, char *aut, size_t na, char *key, size_t nk) {
  snprintf(aut, na, "Authorization: Bearer %s", loginToken);
  snprintf(key, nk, "unused-fixture-key"); cab[0] = aut; cab[1] = key; cab[2] = NULL;
  return 1;
}
int sessao_identidade_local_copiar(char *user, size_t nu, char *token, size_t nt) {
  snprintf(user, nu, "fixture-account-a"); snprintf(token, nt, "%s", loginToken); return 1;
}
int sessao_token_copiar(char *token, size_t n) {
  pthread_mutex_lock(&netLock);
  if (blockRefresh) {
    waiting = 1; pthread_cond_broadcast(&netCond);
    while (!releaseOld) pthread_cond_wait(&netCond, &netLock);
  }
  pthread_mutex_unlock(&netLock); snprintf(token, n, "%s", loginToken); return 1;
}
const ContaPerfil *perfis_item_ativo(void) {
  static ContaPerfil current;
  current.indice = perfis_ativo(); current.primario = current.indice == 1;
  return &current;
}
static void *threadRun(void *p) { SDL_Thread *t = p; t->fn(t->arg); return NULL; }
SDL_Thread *SDL_CreateThread(SDL_ThreadFunction fn, const char *name, void *arg) {
  (void)name;
  if (failThread) return NULL;
  SDL_Thread *t = calloc(1, sizeof *t); t->fn = fn; t->arg = arg;
  assert(!pthread_create(&t->thread, NULL, threadRun, t)); lastThread = t; return t;
}
void SDL_DetachThread(SDL_Thread *t) { (void)t; }
static void joinWorker(void) { assert(lastThread); pthread_join(lastThread->thread, NULL); free(lastThread); lastThread = NULL; }
char *rede_postar_st(const char *u, int timeout, const char *const *cab, const char *body, int *status) {
  (void)timeout; (void)body;
  pthread_mutex_lock(&netLock);
  int n = requests++; assert(n < 8);
  snprintf(seen[n].aut, sizeof seen[n].aut, "%s", cab[0]);
  snprintf(seen[n].via, sizeof seen[n].via, "%s", cab[1]);
  snprintf(seen[n].perfil, sizeof seen[n].perfil, "%s", cab[2] ? cab[2] : "");
  snprintf(seen[n].path, sizeof seen[n].path, "%s", u);
  if (blockRequest) {
    waiting = 1; pthread_cond_broadcast(&netCond);
    while (!releaseOld) pthread_cond_wait(&netCond, &netLock);
  }
  pthread_mutex_unlock(&netLock); *status = 200; return strdup("{}");
}
char *rede_baixar_st(const char *u, int timeout, const char *const *cab, int *status) {
  (void)u; (void)timeout; (void)cab;
  pthread_mutex_lock(&netLock);
  getCalls++;
  if (blockPedidos) {
    waiting = 1; pthread_cond_broadcast(&netCond);
    while (!releaseOld) pthread_cond_wait(&netCond, &netLock);
  }
  pthread_mutex_unlock(&netLock); *status = 200;
  return strdup("{\"recebidos\":[{\"pub\":\"bbbbbbbbbb\",\"apelido\":\"other\"}]}");
}
static void *readPedidos(void *arg) {
  recSessaoDefinir(*(uint64_t *)arg);
  const char *cab[] = { "original-reader", NULL }; lerPedidos(cab); return NULL;
}
static void *readSocialFeed(void *arg) {
  recSessaoDefinir(*(uint64_t *)arg);
  CatItem item; lerFeedAmigos(&item, 1); return NULL;
}
static void waitRequest(void) { pthread_mutex_lock(&netLock); while(!waiting)pthread_cond_wait(&netCond,&netLock); pthread_mutex_unlock(&netLock); }
static void releaseRequest(void) { pthread_mutex_lock(&netLock); releaseOld = 1; blockRequest = 0; pthread_cond_broadcast(&netCond); pthread_mutex_unlock(&netLock); }
static void blockNext(void) { pthread_mutex_lock(&netLock); waiting = releaseOld = 0; blockRequest = 1; pthread_mutex_unlock(&netLock); }
int main(void) {
  setup(); alarm(10); recomenda_abrir_sessao();
  // An unqueued action remains retryable; starting a thread failed.
  failThread = 1; assert(!recomenda_aceitar("aaaaaaaaaa")); assert(!pedidoAcaoN && !pedidoAcaoFio);
  failThread = 0;
  // Capture a secondary native profile's identity even while another Social
  // read is busy. Both already-confirmed choices survive a profile change.
  selectProfile(2); recomenda_abrir_sessao(); socEstado = REC_SOC_INDO;
  assert(recomenda_aceitar("aaaaaaaaaa")); waitRequest();
  assert(recomenda_recusar("bbbbbbbbbb"));
  selectProfile(3); snprintf(loginToken, sizeof loginToken, "account-b-token"); recomenda_abrir_sessao();
  nPedidos = 1; snprintf(pedidosRec[0].pub, sizeof pedidosRec[0].pub, "aaaaaaaaaa");
  releaseRequest(); joinWorker();
  assert(requests == 2 && !pedidoAcaoN && !pedidoAcaoFio);
  for (int i = 0; i < 2; i++) {
    assert(!strcmp(seen[i].aut, "Authorization: Bearer account-a-token"));
    assert(!strcmp(seen[i].via, "X-Nuvio-Auth: nuvio"));
    assert(!strcmp(seen[i].perfil, "X-Nuvio-Perfil: 2"));
  }
  assert(strstr(seen[0].path, "/aceitar") && strstr(seen[1].path, "/recusar"));
  assert(nPedidos == 1 && socEstado == REC_SOC_NADA);
  // A -> B -> A cannot publish an old A response even when its pub is the
  // same. Trakt uses its own captured identity, without a Nuvio profile.
  selectProfile(1); recomenda_abrir_sessao(); viaTrakt = 1;
  snprintf(loginToken, sizeof loginToken, "trakt-a-token"); blockNext();
  assert(recomenda_aceitar("aaaaaaaaaa")); waitRequest();
  selectProfile(2); recomenda_abrir_sessao(); selectProfile(1); recomenda_abrir_sessao();
  nPedidos = 1; snprintf(pedidosRec[0].pub, sizeof pedidosRec[0].pub, "aaaaaaaaaa");
  releaseRequest(); joinWorker();
  assert(!strcmp(seen[2].aut, "Authorization: Bearer trakt-a-token"));
  assert(!strcmp(seen[2].via, "X-Nuvio-Auth: trakt") && !seen[2].perfil[0]);
  assert(nPedidos == 1 && socEstado == REC_SOC_NADA);
  // Current opening's positive control publishes the relationship and status.
  nAchados = temCartao = 1; snprintf(achados[0].pub, sizeof achados[0].pub, "aaaaaaaaaa");
  snprintf(cartaoP.pub, sizeof cartaoP.pub, "aaaaaaaaaa");
  assert(recomenda_aceitar("aaaaaaaaaa")); joinWorker();
  assert(!nPedidos && socEstado == REC_SOC_OK && !strcmp(achados[0].relacao, "amigo"));
  assert(!strcmp(cartaoP.relacao, "amigo") && pedidoAgora);
  // A requests-box GET that started before a confirmed Reject may not
  // resurrect that request when its older response arrives afterwards.
  pthread_mutex_lock(&netLock); blockPedidos = 1; waiting = releaseOld = 0; pthread_mutex_unlock(&netLock);
  uint64_t opening = perfis_sessao_geracao(); pthread_t reader;
  assert(!pthread_create(&reader, NULL, readPedidos, &opening)); waitRequest();
  nPedidos = 1; snprintf(pedidosRec[0].pub, sizeof pedidosRec[0].pub, "bbbbbbbbbb");
  assert(recomenda_recusar("bbbbbbbbbb")); joinWorker(); assert(!nPedidos);
  releaseRequest(); pthread_join(reader, NULL); assert(!nPedidos);
  // JWT renewal may perform HTTP. A blocked renewal must not hold the epoch
  // gate; switching completes first and the old feed request is then discarded.
  pthread_mutex_lock(&netLock);
  blockPedidos = 0; blockRefresh = 1; waiting = releaseOld = 0;
  pthread_mutex_unlock(&netLock); viaTrakt = 0;
  opening = perfis_sessao_geracao(); int previousGets = getCalls;
  assert(!pthread_create(&reader, NULL, readSocialFeed, &opening)); waitRequest();
  selectProfile(2); recomenda_abrir_sessao();
  releaseRequest(); pthread_join(reader, NULL); assert(getCalls == previousGets);
  blockRefresh = 0; opening = perfis_sessao_geracao();
  assert(!pthread_create(&reader, NULL, readSocialFeed, &opening)); pthread_join(reader, NULL);
  assert(getCalls == previousGets + 1);
  puts("PASS Social actions: captured native/Trakt identities finish, old epochs stay hidden, current positive control accepted");
}
