// #424: actual guide storage, deduplication, stable category packing and cache.
#include <stdlib.h>
static int falharMalloc;
static void *testeMalloc(size_t n) { return falharMalloc ? NULL : malloc(n); }
#define malloc testeMalloc
#include "../src/guia.c"
#undef malloc
#include <assert.h>
static const char *pasta;
int perfis_ativo(void) { return 1; }
int addons_n(void) { return 0; }
const char *addons_base(int i) { (void)i; return ""; }
int addons_ativo(int i) { (void)i; return 1; }
char *dados_ler(const char *nome) { (void)nome; return NULL; }
char *dados_caminho(char *out,unsigned n,const char *nome) { snprintf(out,n,"%s/%s",pasta,nome); return out; }
void marco(const char *nome) { (void)nome; }
static void semear(int total, int categorias) {
  sNCanais=sNCats=sIndexados=0;
  memset(sCanalIndice,0,sizeof sCanalIndice);memset(sCatIndice,0,sizeof sCatIndice);
  memset(&sCorte,0,sizeof sCorte);
  for(int i=0;i<total;i++) {
    GCanal c={0};char nome[64];
    snprintf(c.id,sizeof c.id,"xtream:%d",i);
    snprintf(c.nome,sizeof c.nome,"Channel %d",i);
    snprintf(nome,sizeof nome,"Category %04d",i%categorias);
    assert(sCanalPorId(c.id)<0); c.cat=sCatDe(nome);c.epg=-1;
    assert(sCanalGuardar(&c));assert(sCanalPorId(c.id)==i);
  }
  assert(!guiacorte_cortou(&sCorte) && sNCanais==total && sNCats==categorias);
}
int main(int argc,char **argv) {
  assert(argc==2);pasta=argv[1];favLido=1;
  int total=G_MAX_CANAL>3000?12000:600;
  int categorias=G_MAX_CANAL>3000?600:300;
  assert(!canais && !sCanais && capCanais==0 && sCapCanais==0);
  semear(total,categorias);
  assert(canaisReservar(total));
  nCanais=total;nCats=sNCats;
  memcpy(canais,sCanais,(size_t)total*sizeof *canais);memcpy(cats,sCats,(size_t)nCats*sizeof cats[0]);
  empacotar();assert(nCanais==total && nCats==categorias);
  for(int c=0;c<nCats;c++) {
    assert(catN[c]==total/categorias);
    for(int k=0;k<catN[c];k++) {
      char esperado[80];snprintf(esperado,sizeof esperado,"xtream:%d",c+k*categorias);
      assert(!strcmp(canais[catIni[c]+k].id,esperado));
    }
  }
  cacheGravar();nCanais=nCats=0;assert(cacheLer());
  assert(nCanais==total && nCats==categorias);
  assert(!strcmp(canais[nCanais-1].id,G_MAX_CANAL>3000?"xtream:11999":"xtream:599"));
  // Reloading another list cannot find old IDs/categories in the hash index.
  semear(2,2);assert(sCanalPorId("xtream:11999")==-1);
  GCanal *antes=canais; int capacidade=capCanais;
  falharMalloc=1;assert(!canaisReservar(total+1));falharMalloc=0;
  assert(canais==antes && capCanais==capacidade && nCanais==total);
  assert(canaisReservar(G_MAX_CANAL+1)==0 && nCanais==total);
  // Invalid persisted category must never index the counting-sort scratch.
  canais[0].cat=-1;empacotar();assert(nCanais==total-1);
  // One category per channel at the Android bound must still fit.
  if (G_MAX_CANAL>3000) {
    semear(G_MAX_CANAL,G_MAX_CANAL);assert(canaisReservar(G_MAX_CANAL));
    nCanais=sNCanais;nCats=sNCats;
    memcpy(canais,sCanais,(size_t)nCanais*sizeof *canais);memcpy(cats,sCats,(size_t)nCats*sizeof cats[0]);
    empacotar();assert(nCanais==G_MAX_CANAL && nCats==G_MAX_CANAL);
    for(int i=0;i<nCats;i++)assert(catN[i]==1);
  }
  free(canais);free(ordemCanais);free(sCanais);
  printf("PASS: %d channels / %d categories, stable packing, cache round trip, reload dedup and safety bounds\n",total,categorias);
}
