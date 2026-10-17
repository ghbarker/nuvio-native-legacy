#include "../src/fontefiltro.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
  int emCache, cacheNegado, foraCache, cacheNativo, urlP2P, naoVideo;
  unsigned esperado;
} Caso;

int main(void) {
  static const Caso casos[] = {
    {1,0,0,0,0,0,FONTEFILTRO_CACHE_SIM},   /* confirmado pelo addon */
    {0,1,0,0,0,0,FONTEFILTRO_CACHE_NAO},   /* isCached:false */
    {0,0,1,0,0,0,FONTEFILTRO_CACHE_NAO},   /* marcador fora de cache */
    {1,0,1,1,0,0,FONTEFILTRO_CACHE_SIM},   /* confirmacao nativa vence */
    {1,0,0,0,1,0,FONTEFILTRO_CACHE_TODAS}, /* P2P nao e cache de debrid */
    {1,0,0,0,0,1,FONTEFILTRO_CACHE_TODAS}, /* aviso nao e fonte pronta */
    {0,0,0,0,0,0,FONTEFILTRO_CACHE_TODAS}  /* desconhecido */
  };
  FonteFiltroConfig cfg = {0};
  FonteFiltroDados dados = {0};

  for (unsigned i = 0; i < sizeof casos / sizeof casos[0]; i++) {
    const Caso *c = &casos[i];
    dados.cache = fontefiltro_cache_estado(c->emCache,c->cacheNegado,c->foraCache,
                                           c->cacheNativo,c->urlP2P,c->naoVideo);
    assert(dados.cache == c->esperado);

    cfg.cache = FONTEFILTRO_CACHE_TODAS;
    assert(fontefiltro_permite(&cfg,&dados));
    cfg.cache = FONTEFILTRO_CACHE_SIM;
    assert(fontefiltro_permite(&cfg,&dados) ==
           (c->esperado == FONTEFILTRO_CACHE_SIM));
    cfg.cache = FONTEFILTRO_CACHE_NAO;
    assert(fontefiltro_permite(&cfg,&dados) ==
           (c->esperado == FONTEFILTRO_CACHE_NAO));
  }

  // Live sources bypass persistent cache filters for all declared/unknown states.
  dados.naoVod = 1;
  for (unsigned cache = FONTEFILTRO_CACHE_TODAS; cache <= FONTEFILTRO_CACHE_NAO; cache++) {
    cfg.cache = cache;
    for (unsigned state = FONTEFILTRO_CACHE_TODAS; state <= FONTEFILTRO_CACHE_NAO; state++) {
      dados.cache = state;
      assert(fontefiltro_permite(&cfg, &dados));
    }
  }
  dados.naoVod = 0;

  // A candidate selected by Uncached may become confirmed Cached after the
  // resolver hands out its first URL. Renewing that same winner must retain
  // its selection-time cache eligibility instead of exhausting the queue.
  { FonteFiltroDados selecionado = {0}, resolvido = {0};
    cfg.cache = FONTEFILTRO_CACHE_NAO;
    selecionado.cache = FONTEFILTRO_CACHE_NAO;
    resolvido.cache = FONTEFILTRO_CACHE_SIM;
    assert(!fontefiltro_permite(&cfg, &resolvido));
    assert(fontefiltro_renovacao_permite(&cfg, &selecionado, &resolvido));
    cfg.excluir[FONTEFILTRO_RESOLUCAO] = fontefiltro_desconhecido(FONTEFILTRO_RESOLUCAO);
    assert(!fontefiltro_renovacao_permite(&cfg, &selecionado, &resolvido));
    memset(&cfg, 0, sizeof cfg);
  }

  puts("source cache filter: classifier, routing and selected-winner renewal PASS");
  return 0;
}
