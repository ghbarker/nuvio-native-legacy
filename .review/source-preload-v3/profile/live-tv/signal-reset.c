#include "support.h"
#include "ilhasinais.c"
const char *i18n(const char *s) { return s; }
static int accepts, busy = 1, notices;
static char notice_key[80];
int recomenda_aceitar(const char *pub) { assert(!strcmp(pub, "friend0001")); accepts++; return !busy; }
int recomenda_recusar(const char *pub) { (void)pub; assert(0); return 0; }
const char *ilha_forte(char *out, size_t n, const char *s) { snprintf(out, n, "%s", s); return out; }
void ilha_avisar(const char *k, int t, const char *i, const char *text, unsigned ms, int key) {
  (void)t; (void)i; (void)text; (void)ms; (void)key; notices++; snprintf(notice_key, sizeof notice_key, "%s", k);
}
void ilha_avisar_ex(const IlhaAvisoEx *a) { notices++; snprintf(notice_key, sizeof notice_key, "%s", a->chave); }
int recomenda_contatos(RecContato *out, int max) {
  assert(max > 0); memset(out, 0, sizeof *out); snprintf(out->id, sizeof out->id, "profile-%d-contact", perfis_ativo());
  strcpy(out->nome, "Existing B friend"); return 1;
}
int main(void) {
  fixture_init(); pedOpPend = 1; strcpy(pedPubPend, "friend0001"); pedSessao = perfis_sessao_geracao();
  responderPedido(); assert(accepts == 1 && pedOpPend == 1);
  // Guard the interval after epoch advance but before the activation hooks.
  perfis_definir_ativo(2); busy = 0; responderPedido(); assert(accepts == 1);
  pediuTrakt = pediuAmigos = 1; nPend = 1; strcpy(pendPub[0], "old_A"); strcpy(pendChave[0], "pedido:old_A");
  nAmizadesBase = 1; strcpy(amizadesBase[0].id, "A-only-friend"); amizadesDesde = 1;
  syncAntes = SYNC_FALHOU; syncRodandoDesde = 100; syncFalhaDita = traktAvisado = debridDitos = 1; ultSonda = 100;
  ilhasinais_perfil_trocado();
  assert(!pedOpPend && !pedPubPend[0] && !pediuTrakt && !pediuAmigos && !nPend);
  assert(nAmizadesBase == -1 && !amizadesDesde && !ultSonda && !syncRodandoDesde);
  assert(syncAntes == SYNC_PARADO && !syncFalhaDita && !traktAvisado && !debridDitos);
  puts("PASS: an unqueued A retry cannot retarget B; private acknowledgements/UI requests/alert baselines reset");
  int before = notices; amizadesNovas(1000); amizadesNovas(22000);
  assert(nAmizadesBase == 1 && notices == before);
  puts("PASS: B’s existing contacts seed a fresh baseline without false new-friend notifications");
  // Once the immutable API accepts a gesture, the UI consumes it once.
  pedOpPend = 1; strcpy(pedPubPend, "friend0001"); pedSessao = perfis_sessao_geracao(); responderPedido();
  assert(accepts == 2 && !pedOpPend); responderPedido(); assert(accepts == 2);
  puts("PASS: a confirmed gesture successfully queued by the immutable API is consumed exactly once");
  fixture_cleanup();
}
