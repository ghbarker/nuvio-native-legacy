// Only the retained reduced fixtures listed by prepare_legacy_tests.py link
// this object. They predate profile epochs and intentionally omit perfis.c.
// This supplies exactly perfisessao.h's original absent-module fallback.
// Profile ownership/race tests continue linking the real perfis.c instead.
#include <stdint.h>
#include "catalogo.h"

uint64_t perfis_sessao_geracao(void) { return 0; }
int perfis_sessao_atual(uint64_t geracao) { return geracao == 0; }
int perfis_sessao_travar(uint64_t geracao) { return geracao == 0; }
void perfis_sessao_destravar(void) { }

// watched_discovery already mocks the original enrichment boundary. Forward
// its scoped entry point to that same mock only in the legacy epoch-zero case.
extern int trakt_enfeitar_lote(CatItem *, int);
int trakt_enfeitar_lote_sessao(CatItem *itens, int n, uint64_t geracao) {
  return geracao == 0 ? trakt_enfeitar_lote(itens, n) : 0;
}
