#include "../src/streams.c"
#include <assert.h>

static int permitidas(void) {
  int total = 0;
  for (int i = 0; i < n; i++) total += passaChips(i) != 0;
  return total;
}

int main(void) {
  Stream fontes[7] = {0};

  fontes[0].emCache = 1;                         // confirmada pelo addon
  fontes[1].cacheNegado = 1;                    // isCached:false com URL
  fontes[2].foraCache = 1;                      // marcador/download do addon
  fontes[3].emCache = 1; fontes[3].foraCache = 1;
  fontes[3].cacheNativo = 1;                    // resolucao nativa mais recente
  fontes[4].emCache = 1; fontes[4].urlP2P = 1;  // P2P nao e cache de debrid
  fontes[5].emCache = 1; fontes[5].naoVideo = 1;// aviso nao e fonte pronta
  /* fontes[6]: estado desconhecido */

  lista = fontes; n = 7; soMp4 = soDub = 0;

  soCache = FILTRO_CACHE_TODAS;
  assert(permitidas() == 7);
  assert(!strcmp(rotuloBotao(BT_CACHE), "Em cache"));

  avancarFiltroCache();
  assert(soCache == FILTRO_CACHE_SIM);
  assert(permitidas() == 2);
  assert(passaChips(0) && passaChips(3));
  assert(!passaChips(1) && !passaChips(2) && !passaChips(4));
  assert(!passaChips(5) && !passaChips(6));
  assert(!strcmp(rotuloBotao(BT_CACHE), "Em cache"));

  avancarFiltroCache();
  assert(soCache == FILTRO_CACHE_NAO);
  assert(permitidas() == 2);
  assert(passaChips(1) && passaChips(2));
  assert(!passaChips(0) && !passaChips(3));
  assert(!passaChips(4) && !passaChips(5) && !passaChips(6));
  assert(!strcmp(rotuloBotao(BT_CACHE), "Fora do cache"));

  avancarFiltroCache();
  assert(soCache == FILTRO_CACHE_TODAS && permitidas() == 7);

  puts("source cache filter: all/cached/uncached and unknown separation ok");
  return 0;
}
