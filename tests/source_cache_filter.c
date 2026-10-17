#include "../src/fontefiltro.h"
#include <assert.h>
#include <stdio.h>

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

  puts("source cache filter: classifier and All/Cached/Uncached routing PASS");
  return 0;
}
