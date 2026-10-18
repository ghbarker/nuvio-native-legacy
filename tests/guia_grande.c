// Large source-keyed guide retains packed strings, category ownership and caches.
#include "perfis.h"
#define NV_PERFIL_SESSAO_FORTE
#include "../src/guia.c"
#include <assert.h>
static const char *pasta;
uint64_t perfis_sessao_geracao(void){return 1;}
int perfis_sessao_atual(uint64_t s){return s==1;}
int perfis_sessao_travar(uint64_t s){return s==1;}
void perfis_sessao_destravar(void){}
int perfis_ativo(void){return 1;}
int addons_n(void){return 1;}
const char *addons_base(int i){(void)i;return "https://addon.invalid";}
int addons_ativo(int i){(void)i;return 1;}
const char *addons_nome(int i){(void)i;return "Addon";}
const char *i18n(const char *s){return s;}
int ajustes_guia_ordem(void){return 0;}
int ajustes_guia_fonte(void){return 0;}
int iptv_contas(IptvConta *out,int n){(void)out;(void)n;return 0;}
char *dados_ler(const char *n){(void)n;return NULL;}
int dados_gravar(const char *n,const char *s){(void)n;(void)s;return 1;}
char *dados_caminho(char *out,unsigned n,const char *nome){snprintf(out,n,"%s/%s",pasta,nome);return out;}
void marco(const char *s){(void)s;}
static void seed(int total,int ng) {
 stagingZerar();sTeto=30000;
 int f=sFonte(LTV_ADDON,"https://addon.invalid","Addon","https://addon.invalid");assert(f>=0);
 for(int i=0;i<total;i++) {
  char id[80],nome[80],grupo[80];snprintf(id,sizeof id,"channel:%d",i);snprintf(nome,sizeof nome,"Channel %d",i);snprintf(grupo,sizeof grupo,"Group %05d",i%ng);
  GCru c={.id=id,.nome=nome,.grupo=grupo,.logo="",.desc="",.epgId="",.tvgNome="",.url="",.ua="",.ref="",.tipo="channel"};
  assert(sCanalAdd(f,&c));
 }
 assert(sNCanais==total && sNCats==ng);adotarStaging();
 assert(nCanais==total && nCats==ng && nLinhas()==ng);
}
int main(int argc,char **argv) {
 assert(argc==2);pasta=argv[1];seed(12000,600);
 for(int c=0;c<nCats;c++)for(int k=0;k<catN[c];k++) {
  char id[80];snprintf(id,sizeof id,"channel:%d",c+k*600);assert(!strcmp(canais[catIni[c]+k].id,id));
 }
 assert(sizeof(GCanal)<160);cacheGravar();stagingZerar();sTeto=30000;
 char key[24];ltv_chave_fonte(LTV_ADDON,"https://addon.invalid",key,sizeof key);
 assert(cacheFonteLer(key)==12000);adotarStaging();assert(nCanais==12000 && nCats==600);
 seed(30000,3000);assert(nCats==3000 && catN[2999]==10);
 // Source capacity covers32 native accounts plus add-ons/URL sources (>14).
 stagingZerar();sTeto=30000;
 for(int i=0;i<40;i++) {char ch[24];snprintf(ch,sizeof ch,"a%08x",i);assert(sFonteChave(ch,LTV_ADDON,"Source","")==i);}
 assert(sNLtvF==40);
 // Platform budget remains upstream's cross-platform measured budget.
 assert(ltv_teto_canais(3,4096)>=30000);assert(ltv_teto_canais(0,900)<=5000);
 puts("PASS: packed12000channels/600categories cache roundtrip,30000channels/3000groups,40source capacity and retained platform budgets");
}
