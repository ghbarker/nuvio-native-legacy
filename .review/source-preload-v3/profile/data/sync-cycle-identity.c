/* Isolated integration of the actual sync cycle HTTP helper and real epochs.
 * Transport/session storage neighbours are controlled, not the helper.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sync.c"

static char account[80] = "account-A", storedJwt[80] = "jwt-A";
static char received[2][80];
static int requests, renewals, scenario;
int sessao_logada(void) { return 1; }
const char *sessao_usuario(void) { return account; }
unsigned contacache_geracao(void) { return 1; }
int sessao_identidade_local_copiar(char *u, size_t nu, char *jwt, size_t nj) {
  snprintf(u, nu, "%s", account);
  snprintf(jwt, nj, "%s", storedJwt);
  return 1;
}
void sessao_token_recusado(const char *jwt) {
  assert(!strcmp(jwt, "jwt-A"));
  renewals++;
  snprintf(storedJwt, sizeof storedJwt, "jwt-A-renewed");
}
static char *transport(const char *jwt, int *status) {
  assert(requests < 2);
  snprintf(received[requests], sizeof received[requests], "%s", jwt);
  requests++;
  if (scenario == 1 || scenario == 3) {
    snprintf(account, sizeof account, "account-B");
    snprintf(storedJwt, sizeof storedJwt, "jwt-B");
    perfis_definir_ativo(2);
  }
  *status = (scenario == 2 && requests == 1) || scenario == 3 ? 401 : 200;
  return strdup("[]");
}
char *nuvem_rpc_com(const char *name, const char *body, const char *jwt, int *status) {
  assert(!strcmp(name, "sync_push_profile_settings_blob"));
  assert(!strcmp(body, "{\"p_profile_id\":1}"));
  return transport(jwt, status);
}
char *nuvem_tabela(const char *name, const char *query, const char *jwt, int *status) {
  assert(!strcmp(name, "addons"));
  assert(!strcmp(query, "profile_id=eq.1"));
  return transport(jwt, status);
}
const char *dados_dir(void) { return ""; }
int dados_gravar(const char *name, const char *data) { (void)name; (void)data; return 1; }
void fil_definir_perfil(int p) { (void)p; }
void fontepref_definir_perfil(int p) { (void)p; }
void legmem_definir_perfil(int p) { (void)p; }
void arteesc_definir_perfil(int p) { (void)p; }
void plugins_perfil_mudou(void) {}
void servidores_perfil_trocou(void) {}

static void captureCycle(int which) {
  snprintf(account, sizeof account, "account-A");
  snprintf(storedJwt, sizeof storedJwt, "jwt-A");
  perfis_definir_ativo(1);
  perfilDoCiclo = 1;
  copiaGeracaoDoCiclo = 1;
  sessaoDoCiclo = perfis_sessao_geracao();
  assert(sessao_identidade_local_copiar(usuarioDoCiclo, sizeof usuarioDoCiclo,
                                      tokenDoCiclo, sizeof tokenDoCiclo));
  requests = renewals = 0; scenario = which;
  memset(received, 0, sizeof received);
}
int main(void) {
  char *r;
  int status;
  captureCycle(0);
  perfis_definir_ativo(2);
  r = rpcDoCiclo("sync_push_profile_settings_blob", "{\"p_profile_id\":1}", &status);
  assert(!r && requests == 0);
  puts("PASS: a superseded cycle sends no request");
  captureCycle(1);
  r = rpcDoCiclo("sync_push_profile_settings_blob", "{\"p_profile_id\":1}", &status);
  assert(!r && requests == 1 && !strcmp(received[0], "jwt-A"));
  puts("PASS: in-flight account switch keeps original JWT and discards the response");
  captureCycle(2);
  r = rpcDoCiclo("sync_push_profile_settings_blob", "{\"p_profile_id\":1}", &status);
  assert(r && status == 200 && requests == 2 && renewals == 1);
  assert(!strcmp(received[0], "jwt-A") && !strcmp(received[1], "jwt-A-renewed"));
  free(r);
  puts("PASS: one 401 retry renews only the unchanged original account");
  captureCycle(3);
  r = rpcDoCiclo("sync_push_profile_settings_blob", "{\"p_profile_id\":1}", &status);
  assert(!r && requests == 1 && renewals == 0);
  puts("PASS: a changed session cannot renew/retry with destination credentials");
  captureCycle(1);
  r = tabelaDoCiclo("addons", "profile_id=eq.1", &status);
  assert(!r && requests == 1 && !strcmp(received[0], "jwt-A"));
  puts("PASS: addon table reads use the same frozen original identity");
  return 0;
}
