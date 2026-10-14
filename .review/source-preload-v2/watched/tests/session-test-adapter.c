// Review-only: stable no-account epoch used by the legacy watched fixtures.
// Profile transition tests have separate actual producers and are not linked
// to this adapter. The new Trakt wrapper delegates to the old checked double.
#include "perfis.h"
#include "trakt.h"
#include "recomenda.h"
#include <stdlib.h>
uint64_t perfis_sessao_geracao(void) { return 0; }
int perfis_sessao_atual(uint64_t epoch) { return epoch == 0; }
int perfis_sessao_travar(uint64_t epoch) { return perfis_sessao_atual(epoch); }
void perfis_sessao_destravar(void) {}
int trakt_enfeitar_lote_sessao(CatItem *items, int count, uint64_t epoch) {
  return perfis_sessao_atual(epoch) ? trakt_enfeitar_lote(items, count) : 0;
}

// Non-discovery fixtures have no Trakt traffic: fail if an unexpected new
// path enters it. Discovery retains its strong original checked double.
__attribute__((weak)) int trakt_enfeitar_lote(CatItem *items, int count) {
  (void)items; (void)count; abort();
}

int recomenda_social_mesclar_sessao(CatItem *items, int count, int cap, uint64_t epoch) {
  return perfis_sessao_atual(epoch) ? recomenda_social_mesclar(items, count, cap) : 0;
}
__attribute__((weak)) int recomenda_social_mesclar(CatItem *items, int count, int cap) {
  (void)items; (void)count; (void)cap; abort();
}
