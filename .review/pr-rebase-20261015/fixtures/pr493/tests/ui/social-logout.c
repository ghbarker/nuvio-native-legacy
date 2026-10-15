#define NV_REC_URL "https://fixture.invalid"
#include "common.h"
#include "recomenda.c"
static char currentUser[80] = "fixture-account-a";
int ajustes_social(void) { return 1; }
const char *i18n(const char *x) { return x; }
int sessao_identidade_local_copiar(char *user, size_t nu, char *token, size_t nt) {
  snprintf(user, nu, "%s", currentUser); snprintf(token, nt, "expired-local-token"); return 1;
}
static int exists(const char *name) { char *b = dados_ler(name); int yes = b != NULL; free(b); return yes; }
static const char *bases[] = {
 "recomendacoes.txt", "recomendacoes-cursor.txt", "recomendacoes-cartao.txt",
 "recomendacoes-eu.txt", "recomendacoes-aparecer.txt", "recomendacoes-perfil.txt",
 "recomendacoes-alcance.txt", "recomendacoes-feed.json", "recomendacoes-amigo.json",
 "recomendacoes-respostas.txt", "amigos-vistos.txt"
};
static void fillBanks(void) {
  for (size_t i = 0; i < sizeof bases / sizeof bases[0]; i++) {
    dados_gravar(bases[i], "A private fixture\n");
    const char *ext = strrchr(bases[i], '.');
    for (int p = 1; p <= CONTA_PERFIL_MAX; p++) {
      char name[96]; snprintf(name, sizeof name, "%.*s-p%d%s", (int)(ext-bases[i]), bases[i], p, ext);
      dados_gravar(name, "A private fixture\n");
    }
  }
  dados_gravar("recomendacoes-aparecer-p2.txt", "2\n");
  dados_gravar("recomendacoes-perfil-p2.txt", "1\t1\t1\t2\t0\t1\tAlice\tA private bio\n");
}
static void assertPurged(void) {
  for (size_t i = 0; i < sizeof bases / sizeof bases[0]; i++) {
    assert(!exists(bases[i]));
    const char *ext = strrchr(bases[i], '.');
    for (int p = 1; p <= CONTA_PERFIL_MAX; p++) {
      char name[96]; snprintf(name, sizeof name, "%.*s-p%d%s", (int)(ext-bases[i]), bases[i], p, ext);
      assert(!exists(name));
    }
  }
  assert(exists("keep-settings.txt"));
}
int main(void) {
  setup(); alarm(10); dados_gravar("keep-settings.txt", "preserved\n");
  // Adopt the original primary file once without renaming or erasing it.
  dados_gravar("recomendacoes-aparecer.txt", "2\n"); recomenda_abrir_sessao();
  assert(aparecer == REC_APARECER_SIM && exists("recomendacoes-aparecer.txt"));
  fillBanks(); selectProfile(2); recomenda_abrir_sessao();
  assert(aparecer == REC_APARECER_SIM && perfil.publicado && !strcmp(perfil.apelido, "alice"));
  recresp_responder(99, 1, "A private reply"); RecResp rr; assert(recresp_ler(99, &rr));
  // Local UUID comparison rejects A:p2 before any HTTP, even when the cached
  // access token is expired and no explicit logout callback ran.
  snprintf(currentUser, sizeof currentUser, "fixture-account-b"); selectProfile(2); recomenda_abrir_sessao();
  assertPurged(); assert(!perfil.publicado && aparecer == REC_APARECER_NAO_PERGUNTADO);
  assert(!recresp_ler(99, &rr));
  char *owner = dados_ler(REC_ARQ_DONO); assert(owner && !strcmp(owner, currentUser)); free(owner);
  // Explicit logout removes ALL profile banks, not only currently selected p2.
  snprintf(currentUser, sizeof currentUser, "fixture-account-a"); selectProfile(2); recomenda_abrir_sessao();
  fillBanks(); recomenda_abrir_sessao(); recresp_responder(99, 1, "A private reply");
  recomenda_esquecer_conta(); assertPurged(); assert(!exists(REC_ARQ_DONO));
  snprintf(currentUser, sizeof currentUser, "fixture-account-b"); selectProfile(2); recomenda_abrir_sessao();
  assert(!perfil.publicado && !recresp_ler(99, &rr) && !nItens && !nFeedN);
  puts("PASS Social logout: primary compatibility, A:p2 owner mismatch rejected offline, all-profile purge, B:p2 clean");
}
