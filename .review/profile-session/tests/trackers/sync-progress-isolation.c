// Actual session identity snapshot, actual profile epoch and actual removal RPC body.
#include "syncprog.h"
#include "progresso.h"
#include "perfis.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
int dados_gravar(const char *n, const char *v) { (void)n; (void)v; return 1; }
void fil_definir_perfil(int p) { (void)p; }
void fontepref_definir_perfil(int p) { (void)p; }
void legmem_definir_perfil(int p) { (void)p; }
void arteesc_definir_perfil(int p) { (void)p; }
void plugins_perfil_mudou(void) {}
void servidores_perfil_trocou(void) {}
int dados_apagar(const char *name) { (void)name; return 1; }
static int allowRefresh, refreshCalls, rejectOnce;
#define REFRESHED_JWT "eyJhbGciOiJub25lIn0.eyJzdWIiOiJhY2NvdW50LUEiLCJleHAiOjIwMDAwMDAwMDB9.signature"
int nuvem_pronta(void) { return 1; }
void nuvem_falhou(void) {}
void nuvem_ok(void) {}
char *nuvem_post(const char *path, const char *json, const char *jwt, int *st) {
  (void)json; (void)jwt;
  assert(allowRefresh && strstr(path, "grant_type=refresh_token")); refreshCalls++; *st = 200;
  return strdup("{\"access_token\":\"" REFRESHED_JWT "\",\"refresh_token\":\"new-refresh-A\"}");
}
char *nuvem_tabela(const char *table, const char *query, const char *jwt, int *st) {
  (void)table; (void)query; (void)jwt; (void)st; assert(!"unexpected table request"); return NULL;
}
#include "sessao.c"
static pthread_mutex_t rpcLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t rpcCond = PTHREAD_COND_INITIALIZER;
static int blockA, entered, releaseOld, calls, returned;
static int pullRead;
static char aBody[4096], bBody[4096];
char *nuvem_rpc_com(const char *fn, const char *json, const char *jwt, int *st) {
  assert(!strcmp(jwt, "old-account-jwt"));
  assert(!strcmp(fn, pullRead ? "sync_pull_watch_progress" : "sync_push_watch_progress"));
  int a = strstr(json, "\"p_profile_id\":1") != NULL;
  pthread_mutex_lock(&rpcLock); calls++;
  snprintf(a ? aBody : bBody, sizeof aBody, "%s", json);
  if (a && blockA) { entered = 1; pthread_cond_broadcast(&rpcCond); while (!releaseOld) pthread_cond_wait(&rpcCond, &rpcLock); }
  *st = 200; pthread_mutex_unlock(&rpcLock);
  return strdup(pullRead ? "[{\"content_id\":\"tt1000009\",\"position\":30000,\"duration\":120000}]" : "null");
}
static void account(const char *owner, const char *jwt) {
  pthread_mutex_lock(&tokTrava); snprintf(sub, sizeof sub, "%s", owner); snprintf(acesso, sizeof acesso, "%s", jwt);
  snprintf(renovar, sizeof renovar, "%s", "fixture-refresh-A");
  anonima = 0; expiraEm = 1; pthread_mutex_unlock(&tokTrava); // expired deliberately: capture cannot refresh
}
char *dados_ler(const char *name) { (void)name; return NULL; }
const char *dados_cliente_id(void) { return "fixture-client"; }
int cat_indice_por_imdb(const char *id) { (void)id; return -1; }
void cat_aplicar_progresso(int i, double p, double d, int t, int e) { (void)i; (void)p; (void)d; (void)t; (void)e; }
static uint64_t oldEpoch;
static void *syncOld(void *u) {
  (void)u; returned = pullRead ? syncprog_puxar_sessao(1, oldEpoch) : syncprog_empurrar_sessao(1, oldEpoch); return NULL;
}
static void waitEntered(void) {
  struct timespec until; clock_gettime(CLOCK_REALTIME, &until); until.tv_sec += 3;
  pthread_mutex_lock(&rpcLock); while (!entered) assert(pthread_cond_timedwait(&rpcCond, &rpcLock, &until) == 0); pthread_mutex_unlock(&rpcLock);
}
static void begin(int pulling) {
  pullRead = pulling; blockA = 1; entered = releaseOld = 0; calls = 0;
  perfis_definir_ativo(1); account("account-A", "old-account-jwt");
  pthread_mutex_lock(&tokTrava); expiraEm = (long)time(NULL) + 3600; pthread_mutex_unlock(&tokTrava);
  oldEpoch = perfis_sessao_geracao(); syncprog_esquecer();
}
static void releaseNetwork(void) { pthread_mutex_lock(&rpcLock); releaseOld = 1; pthread_cond_broadcast(&rpcCond); pthread_mutex_unlock(&rpcLock); }
static void push(void) {
  begin(0); assert(prog_gravar_local("tt1000001", 0, 0, 30, 120, 1));
  pthread_t worker; pthread_create(&worker, NULL, syncOld, NULL); waitEntered();
  perfis_definir_ativo(2); assert(prog_gravar_local("tt2000001", 0, 0, 60, 120, 1));
  assert(syncprog_empurrar_sessao(2, perfis_sessao_geracao()) == 1);
  releaseNetwork(); pthread_join(worker, NULL); assert(returned == 1 && calls == 2);
  assert(strstr(aBody, "tt1000001") && !strstr(aBody, "tt2000001"));
  assert(strstr(bBody, "tt2000001") && !strstr(bBody, "tt1000001"));
  ProgRegistro r; assert(prog_por_chave("tt2000001", &r) && !r.pendente && r.posSeg == 60);
  perfis_definir_ativo(1); assert(prog_por_chave("tt1000001", &r) && !r.pendente && r.posSeg == 30);
  puts("PASS concurrent A/B progress pushes freeze original profile/token/body and independently acknowledge original records");
}
static void pull(int roundtrip) {
  begin(1); pthread_t worker; pthread_create(&worker, NULL, syncOld, NULL); waitEntered();
  perfis_definir_ativo(2); if (roundtrip) perfis_definir_ativo(1);
  releaseNetwork(); pthread_join(worker, NULL); assert(returned == -1 && syncprog_puxadas() == 0);
  printf("PASS old progress pull rejected through %s before publishing private mailbox\n", roundtrip ? "A-B-A" : "A-B");
}
int main(void) { push(); pull(0); pull(1); return 0; }
