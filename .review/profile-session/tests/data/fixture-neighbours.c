/* Existing isolated discovery fixtures omit this current Settings API. */
__attribute__((weak)) int ajustes_itens_fileira(void) { return 12; }
// Retained preview only: these CW isolation fixtures keep Hide watched off.
// Its real Settings/parser/Home behavior is covered by the retained suite.
__attribute__((weak)) int ajustes_ocultar_filmes_vistos(void) { return 0; }

#include "catalogo.h"
#include "perfisessao.h"
int trakt_enfeitar_lote(CatItem *itens, int n);
// The reduced fixture's enrichment is synchronous and makes no HTTP calls.
// Ownership/race fixtures link real perfis.c; old unscoped regressions retain
// their existing optional-module contract. Never bypass a supplied epoch gate.
int trakt_enfeitar_lote_sessao(CatItem *itens, int n, uint64_t sessao) {
  int enriquecidos;
  if (!nv_perfil_sessao_travar(sessao)) return 0;
  enriquecidos = trakt_enfeitar_lote(itens, n);
  nv_perfil_sessao_destravar();
  return enriquecidos;
}
