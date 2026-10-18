#include "stub_perfisessao.h"
#include "../src/guia.c"
#include <assert.h>
static int ordem;
static const char *pasta;
static IptvConta contasTeste[2]={{.tipo=IPTV_XTREAM,.id=11,.ativa=1,.nome="Account A"},{.tipo=IPTV_XTREAM,.id=22,.ativa=1,.nome="Account B"}};
const char *i18n(const char *s){return s;}
int ajustes_guia_ordem(void){return ordem;}
int iptv_contas(IptvConta *out,int max){if(out && max>=2)memcpy(out,contasTeste,sizeof contasTeste);return 2;}
int addons_n(void){return 1;}
const char *addons_base(int i){(void)i;return "https://addon.invalid";}
const char *addons_nome(int i){(void)i;return "Addon C";}
int addons_ativo(int i){(void)i;return 1;}
int perfis_ativo(void){return 1;}
char *dados_ler(const char *nome){(void)nome;return NULL;}
char *dados_caminho(char *out,unsigned n,const char *nome){snprintf(out,n,"%s/%s",pasta,nome);return out;}
int ajustes_guia_fonte(void){return 0;}
void marco(const char *nome){(void)nome;}
static void semear(void) {
  memset(canais,0,sizeof canais);nCanais=4;nCats=2;nFav=nFavOrd=0;
  strcpy(cats[0],"Sports");strcpy(cats[1],"News");
  for(int i=0;i<4;i++) {
    snprintf(canais[i].id,sizeof canais[i].id,"xtream:@%08x:%d",i<2?11:22,i%2+1);
    canais[i].cat=i%2;canais[i].epg=i+100;
    snprintf(canais[i].epgId,sizeof canais[i].epgId,"epg-%d",i);
  }
}
static void rotulo(int i,const char *s){assert(!strcmp(linhaNome(i),s));}
int main(int argc,char **argv) {
  assert(argc==2);pasta=argv[1];
  semear();ordem=0;strcpy(fav[nFav++],canais[3].id);empacotar();
  assert(nCanais==4 && nCats==4 && nFavOrd==1);
  rotulo(0,"Favoritos");rotulo(1,"News · Account A");rotulo(2,"News · Account B");
  rotulo(3,"Sports · Account A");rotulo(4,"Sports · Account B");
  char id[80];snprintf(id,sizeof id,"%s",linhaItem(4,0)->id);guia_focar_id(id);
  ordem=1;guia_reordenar();assert(!strcmp(guia_id_focado(),id));
  rotulo(1,"News · Account A");rotulo(2,"Sports · Account A");rotulo(3,"News · Account B");rotulo(4,"Sports · Account B");
  assert(linhaItem(0,0)->fav && linhaItem(0,0)->epg==103 && !strcmp(linhaItem(0,0)->epgId,"epg-3"));
  IptvConta tmp=contasTeste[0];contasTeste[0]=contasTeste[1];contasTeste[1]=tmp;guia_reordenar();
  rotulo(1,"News · Account B");rotulo(3,"News · Account A");assert(!strcmp(guia_id_focado(),id));
  ordem=0;guia_reordenar();rotulo(1,"News · Account B");rotulo(2,"News · Account A");
  // A cached guide already has split categories: regrouping must be idempotent.
  unsigned long assinatura=assinaturaDe(canais,nCanais,cats);empacotar();
  assert(nCats==4 && nCanais==4 && assinaturaDe(canais,nCanais,cats)==assinatura);
  strcpy(contasTeste[0].nome,"Renamed B");guia_reordenar();rotulo(1,"News · Renamed B");
  semear();nCanais=5;canais[4]=canais[0];strcpy(canais[4].id,"addon:1");strcpy(canais[4].base,addons_base(0));
  empacotar();assert(nCanais==5 && nCats==5);rotulo(4,"Sports · Addon C");
  semear();cacheGravar();nCanais=nCats=0;assert(cacheLer());
  assert(nCanais==4 && nCats==4);rotulo(0,"News · Renamed B");
  // Display splitting can exceed the old raw category cap without losing channels.
  nCanais=GUIACORTE_MAX_CAT*2;nCats=GUIACORTE_MAX_CAT;nFav=nFavOrd=0;
  memset(canais,0,sizeof canais);
  for(int i=0;i<nCats;i++)snprintf(cats[i],sizeof cats[i],"Category %03d",i);
  for(int i=0;i<nCanais;i++) { canais[i].cat=i/2;snprintf(canais[i].id,sizeof canais[i].id,"xtream:@%08x:%d",i%2?11:22,i); }
  empacotar();assert(nCats==GUIACORTE_MAX_CAT*2 && nCanais==GUIACORTE_MAX_CAT*2);
  puts("PASS: shared guide, labeled duplicate categories, both order modes, account reorder/rename, favorites/focus/EPG, add-ons and legacy-cache regrouping");
}
