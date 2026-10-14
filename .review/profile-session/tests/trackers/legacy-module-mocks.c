// Explicit dependencies for old single-account module fixtures only.
// Never link this into run.sh, OAuth ownership fixtures, or the app.
#ifndef NV_LEGACY_MODULE_FIXTURE
#error "legacy mocks require an explicitly selected legacy fixture"
#endif
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "sync.h"
#include "syncprog.h"
#include "sessao.h"
#include "trakt.h"
uint64_t perfis_sessao_geracao(void) { return 0; }
int perfis_sessao_atual(uint64_t g) { return g == 0; }
int perfis_sessao_travar(uint64_t g) { return g == 0; }
void perfis_sessao_destravar(void) {}
#ifdef NV_LEGACY_SCOPED_CREDENTIAL
int sync_credencial_pedido(SyncCredencialPedido *p) {
  memset(p, 0, sizeof *p); p->perfil = 1; return 1;
}
int sync_empurrar_credencial_pedido(const SyncCredencialPedido *p, const char *provider, const char *json) {
  assert(p->sessao == 0 && p->perfil == 1 && !p->usuario[0]);
  return sync_empurrar_credencial(provider, json);
}
#endif
#ifdef NV_LEGACY_SCOPED_ENRICH
int trakt_enfeitar_lote_sessao(CatItem *items, int n, uint64_t g) {
  assert(g == 0); return trakt_enfeitar_lote(items, n);
}
#endif
#ifdef NV_LEGACY_AUTH_ACCOUNT
const char *sessao_usuario(void) { return ""; }
int sessao_identidade_local_copiar(char *u, size_t nu, char *jwt, size_t nj) {
  if (nu) u[0] = 0;
  if (nj) jwt[0] = 0;
  return 0;
}
#endif
#ifdef NV_LEGACY_ACCOUNT_RPC
// Existing progress/watch fixtures model one fixed account with sessao_rpc.
// Its new snapshot/RPC interface uses that same fake JWT and original body.
__attribute__((weak)) const char *sessao_usuario(void) { return "legacy-fixture-account"; }
int sessao_identidade_local_copiar(char *u, size_t nu, char *jwt, size_t nj) {
  snprintf(u, nu, "%s", sessao_usuario()); snprintf(jwt, nj, "legacy-fixture-jwt"); return 1;
}
int sessao_token_copiar(char *jwt, size_t n) { snprintf(jwt, n, "legacy-fixture-jwt"); return 1; }
void sessao_token_recusado(const char *jwt) { (void)jwt; assert(!"legacy fixture does not simulate JWT rejection"); }
char *nuvem_rpc_com(const char *func, const char *body, const char *jwt, int *status) {
  assert(!strcmp(jwt, "legacy-fixture-jwt")); return sessao_rpc(func, body, status);
}
#endif
#ifdef NV_LEGACY_REMOVAL
// This existing removal fixture has no profile/account module at all.
int perfis_ativo(void) { return 1; }
const char *sessao_usuario(void) { return ""; }
int syncprog_remocao_capturar(SyncProgRemocao *p) {
  memset(p, 0, sizeof *p); p->perfil = 1; return 1;
}
int syncprog_remover_pedido(const SyncProgRemocao *p, const char *key) {
  assert(p->sessao == 0 && p->perfil == 1 && !p->usuario[0]); return syncprog_remover(key);
}
int syncprog_remover_sessao(const char *key, int profile, uint64_t g, const char *owner) {
  assert(profile == 1 && g == 0 && !owner[0]); return syncprog_remover(key);
}
#endif
