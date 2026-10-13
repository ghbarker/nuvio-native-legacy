// Actual session identity snapshot, actual profile epoch and actual removal RPC body.
#include "syncprog.h"
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
static int blockA, entered, releaseOld, finished, calls;
static char body[512], bearer[3000];
char *nuvem_rpc_com(const char *fn, const char *json, const char *jwt, int *st) {
  assert(!strcmp(fn, "sync_delete_watch_progress"));
  pthread_mutex_lock(&rpcLock);
  snprintf(body, sizeof body, "%s", json); snprintf(bearer, sizeof bearer, "%s", jwt); calls++;
  if (blockA && !strcmp(jwt, "old-account-jwt")) {
    entered = 1; pthread_cond_broadcast(&rpcCond);
    while (!releaseOld) pthread_cond_wait(&rpcCond, &rpcLock);
  }
  *st = rejectOnce ? 401 : 200; rejectOnce = 0; finished++; pthread_cond_broadcast(&rpcCond); pthread_mutex_unlock(&rpcLock);
  return strdup("null");
}
static void account(const char *owner, const char *jwt) {
  pthread_mutex_lock(&tokTrava); snprintf(sub, sizeof sub, "%s", owner); snprintf(acesso, sizeof acesso, "%s", jwt);
  snprintf(renovar, sizeof renovar, "%s", "fixture-refresh-A");
  anonima = 0; expiraEm = 1; pthread_mutex_unlock(&tokTrava); // expired deliberately: capture cannot refresh
}
static SyncProgRemocao old;
static void *sendOld(void *u) { (void)u; assert(syncprog_remover_pedido(&old, "tt1000001_s1e2")); return NULL; }
static void waitEntered(void) {
  struct timespec until; clock_gettime(CLOCK_REALTIME, &until); until.tv_sec += 3;
  pthread_mutex_lock(&rpcLock); while (!entered) assert(pthread_cond_timedwait(&rpcCond, &rpcLock, &until) == 0); pthread_mutex_unlock(&rpcLock);
}
int main(void) {
  perfis_definir_ativo(2); account("account-A", "old-account-jwt");
  assert(syncprog_remocao_capturar(&old)); assert(old.perfil == 2 && old.autenticada);
  assert(!strcmp(old.usuario, "account-A") && !strcmp(old.jwt, "old-account-jwt") && calls == 0);
  perfis_definir_ativo(3); account("account-B", "new-account-jwt");
  assert(syncprog_remover_pedido(&old, "tt1000001_s1e2"));
  assert(strstr(body, "\"p_profile_id\":2") && strstr(body, "\"p_keys\":[\"tt1000001_s1e2\"]"));
  assert(!strcmp(bearer, "old-account-jwt"));
  puts("PASS queued removal keeps account A JWT and original profile 2 after account/profile switch before dispatch; capture did no HTTP");
  pthread_mutex_lock(&tokTrava); expiraEm = (long)time(NULL) + 3600; pthread_mutex_unlock(&tokTrava);
  assert(syncprog_remover("tt2000001"));
  assert(strstr(body, "\"p_profile_id\":3") && !strcmp(bearer, "new-account-jwt"));
  puts("PASS legacy immediate removal now sends explicit profile ID, avoiding SQL's profile 1 default");
  perfis_definir_ativo(2); account("account-A", "old-account-jwt"); assert(syncprog_remocao_capturar(&old));
  pthread_mutex_lock(&tokTrava); expiraEm = (long)time(NULL) + 3600; pthread_mutex_unlock(&tokTrava);
  blockA = 1; pthread_t worker; pthread_create(&worker, NULL, sendOld, NULL); waitEntered();
  perfis_definir_ativo(3); account("account-B", "new-account-jwt");
  pthread_mutex_lock(&rpcLock); assert(!strcmp(bearer, "old-account-jwt") && strstr(body, "\"p_profile_id\":2"));
  releaseOld = 1; pthread_cond_broadcast(&rpcCond); pthread_mutex_unlock(&rpcLock); pthread_join(worker, NULL);
  assert(!syncprog_remover_sessao("tt1000001_s1e2", old.perfil, old.sessao, old.usuario));
  puts("PASS dispatched old-account delete cannot adopt new login; late unscoped legacy command is rejected");
  perfis_definir_ativo(2); account("account-A", "old-account-jwt");
  SyncProgRemocao needsRefresh; assert(syncprog_remocao_capturar(&needsRefresh) && refreshCalls == 0);
  allowRefresh = 1;
  assert(syncprog_remover_pedido(&needsRefresh, "tt4000001"));
  assert(refreshCalls == 1 && !strcmp(bearer, REFRESHED_JWT) && strstr(body, "\"p_profile_id\":2"));
  puts("PASS expired original account token refreshes in worker, retaining original profile destination");
  account("account-A", "old-account-jwt");
  pthread_mutex_lock(&tokTrava); expiraEm = (long)time(NULL) + 3600; pthread_mutex_unlock(&tokTrava);
  assert(syncprog_remocao_capturar(&needsRefresh)); rejectOnce = 1;
  assert(syncprog_remover_pedido(&needsRefresh, "tt5000001"));
  assert(refreshCalls == 2 && !strcmp(bearer, REFRESHED_JWT) && strstr(body, "\"p_profile_id\":2"));
  puts("PASS current original-account 401 gets one renewal and retry without changing profile");
  allowRefresh = 0;
  pthread_mutex_lock(&tokTrava); anonima = 1; pthread_mutex_unlock(&tokTrava);
  SyncProgRemocao anonymous; assert(syncprog_remocao_capturar(&anonymous) && !anonymous.autenticada && !anonymous.jwt[0]);
  assert(!syncprog_remover_pedido(&anonymous, "tt3000001"));
  puts("PASS anonymous identity cannot send account deletion");
  return 0;
}
