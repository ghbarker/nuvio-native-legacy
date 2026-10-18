#include "perfis.h"
#define NV_PERFIL_SESSAO_FORTE
static int perfilTeste=1, falharGravacao;
static uint64_t sessaoTeste=1;
uint64_t perfis_sessao_geracao(void){return sessaoTeste;}
int perfis_sessao_atual(uint64_t s){return s==sessaoTeste;}
int perfis_sessao_travar(uint64_t s){return s==sessaoTeste;}
void perfis_sessao_destravar(void){}
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
int perfis_ativo(void){return perfilTeste;}
char *dados_ler(const char *nome){
  char cam[1024];snprintf(cam,sizeof cam,"%s/%s",pasta,nome);FILE *f=fopen(cam,"rb");if(!f)return NULL;
  assert(!fseek(f,0,SEEK_END));long n=ftell(f);assert(n>=0);rewind(f);char *b=calloc((size_t)n+1,1);assert(b);
  assert(fread(b,1,(size_t)n,f)==(size_t)n);fclose(f);return b;
}
int dados_gravar(const char *nome,const char *texto){
  if(falharGravacao)return 0;char cam[1024];snprintf(cam,sizeof cam,"%s/%s",pasta,nome);
  FILE *f=fopen(cam,"wb");assert(f);assert(fwrite(texto,1,strlen(texto),f)==strlen(texto));return !fclose(f);
}
char *dados_caminho(char *out,unsigned n,const char *nome){snprintf(out,n,"%s/%s",pasta,nome);return out;}
int ajustes_guia_fonte(void){return 0;}
void marco(const char *nome){(void)nome;}
static void semear(void) {
  assert(canaisReservar(5));memset(canais,0,5*sizeof *canais);nCanais=4;nCats=2;nFav=nFavOrd=0;
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
  const int grandes=300;assert(canaisReservar(grandes*2));
  nCanais=grandes*2;nCats=grandes;nFav=nFavOrd=0;
  memset(canais,0,(size_t)nCanais*sizeof *canais);
  for(int i=0;i<nCats;i++)snprintf(cats[i],sizeof cats[i],"Category %03d",i);
  for(int i=0;i<nCanais;i++) { canais[i].cat=i/2;snprintf(canais[i].id,sizeof canais[i].id,"xtream:@%08x:%d",i%2?11:22,i); }
  empacotar();assert(nCats==grandes*2 && nCanais==grandes*2);
  semear();ordem=0;empacotar();GuiaCategoria categorias[4];assert(guia_categorias(categorias,4)==4);
  int a=-1;for(int i=0;i<4;i++)if(!strcmp(categorias[i].nome,"Sports") && !strcmp(categorias[i].conta,"Account A"))a=i;
  assert(a>=0);GuiaCategoria alvo=categorias[a];
  char outro[80];snprintf(outro,sizeof outro,"%s",linhaItem(0,0)->id);guia_focar_id(outro);
  assert(guia_categoria_definir(&alvo,1));assert(nLinhas()==3 && nCanais==4 && nCats==4);
  assert(!strcmp(guia_id_focado(),outro));
  int esportes=0;for(int i=0;i<nLinhas();i++)esportes+=!strcmp(cats[linhaCatDe(i)],"Sports");assert(esportes==1);
  // The Categories drawer uses these same visible lines. Both duplicate
  // Sports entries remain in Settings, but only Account B remains in the drawer.
  for(int i=0;i<nLinhas();i++)assert(categoriaChave(linhaCatDe(i))!=alvo.chave);
  cacheGravar();nCanais=nCats=0;categoriasPerfil=-1;assert(cacheLer());
  assert(nLinhas()==3);
  for(int i=0;i<nLinhas();i++)assert(categoriaChave(linhaCatDe(i))!=alvo.chave);
  // Simulate a fresh provider publication from the saved channel records.
  empacotar();assert(nLinhas()==3);
  IptvConta troca=contasTeste[0];contasTeste[0]=contasTeste[1];contasTeste[1]=troca;
  for(int i=0;i<2;i++)if(contasTeste[i].id==11)strcpy(contasTeste[i].nome,"Renamed A");
  ordem=1;guia_reordenar();assert(nLinhas()==3 && guia_categorias(categorias,4)==4);
  int oculta=-1;for(int i=0;i<4;i++)if(categorias[i].chave==alvo.chave)oculta=i;
  assert(oculta>=0 && categorias[oculta].oculta && !strcmp(categorias[oculta].conta,"Renamed A"));
  falharGravacao=1;assert(!guia_categoria_definir(&categorias[oculta],0));assert(nLinhas()==3);falharGravacao=0;
  perfilTeste=2;sessaoTeste++;categoriasReunir();assert(nLinhas()==4);
  assert(!guia_categoria_definir(&alvo,1)); // Old editor cannot affect new profile.
  perfilTeste=1;sessaoTeste++;categoriasReunir();assert(nLinhas()==3);
  assert(guia_categorias(categorias,4)==4);
  for(int i=0;i<4;i++)assert(guia_categoria_definir(&categorias[i],1));
  assert(!nLinhas() && nCanais==4 && nCats==4 && !guia_id_focado()[0]);
  assert(guia_categoria_definir(&categorias[oculta],0));assert(nLinhas()==1 && nCanais==4);
  assert(guia_categorias(categorias,4)==4);
  for(int i=0;i<4;i++)categorias[i].oculta=0;
  int antes=nLinhas();falharGravacao=1;assert(!guia_categorias_definir(categorias,4));
  assert(nLinhas()==antes);falharGravacao=0;
  assert(guia_categorias_definir(categorias,4) && nLinhas()==4);
  for(int i=0;i<4;i++)categorias[i].oculta=i%2;
  assert(guia_categorias_definir(categorias,4) && nLinhas()==2);
  categoriasPerfil=-1;empacotar();assert(nLinhas()==2);
  GuiaCategoria invalida[2]={categorias[0],categorias[1]};invalida[1].sessao--;
  assert(!guia_categorias_definir(invalida,2) && nLinhas()==2);
  puts("PASS: atomic multi-category save, failed-save rollback, persisted batch and mixed-session rejection");
  puts("PASS: source-specific hiding, disk reload, rename/order stability, failed-save rollback, profile isolation, all-hidden and restore without deleting channels");
  puts("PASS: shared guide, labeled duplicate categories, both order modes, account reorder/rename, favorites/focus/EPG, add-ons and legacy-cache regrouping");
}
