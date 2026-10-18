#include "stub_perfisessao.h"
#include "../src/guia.c"
#include <assert.h>

static int chamadas[5], parcial, cancelar;
const char *i18n(const char *s){return s;}
int addons_base_desligada(const char *s){(void)s;return 0;}
int xtream_configurado(void){return 1;}
int stalker_configurado(void){return 1;}
int m3u_configurado(void){return 1;}
int xtream_ultima_falha(void){return XT_OK;}
int xtream_ultimo_http(void){return 200;}
int xtream_ultimo_total(void){return 1;}
int m3u_ultimo_total(void){return 1;}
int xtream_canais_sessao(XtreamCanal *c,int max,uint64_t sessao){(void)max;(void)sessao;chamadas[1]++;memset(c,0,sizeof *c);strcpy(c->id,"xtream:1");strcpy(c->nome,"Xtream One");strcpy(c->categoria,"Xtream News");return 1;}
int m3u_canais_sessao(M3uCanal *c,int max,uint64_t sessao){(void)max;(void)sessao;chamadas[2]++;memset(c,0,sizeof *c);strcpy(c->id,"nvm3u:0123456789abcdef");strcpy(c->nome,"M3U One");strcpy(c->categoria,"M3U News");return 1;}
int stalker_canais_sessao(StalkerCanal *c,int max,uint64_t sessao){(void)max;(void)sessao;chamadas[3]++;memset(c,0,sizeof *c);strcpy(c->id,"stalker:1");strcpy(c->nome,"Stalker One");strcpy(c->categoria,"Stalker News");return 1;}
int xtream_fonte_configurado(unsigned id){(void)id;return 1;}
int m3u_fonte_configurado(unsigned id){(void)id;return 1;}
int stalker_fonte_configurado(unsigned id){(void)id;return 1;}
int xtream_fonte_canais(unsigned id,XtreamCanal *c,int max,uint64_t sessao) {
  int n=xtream_canais_sessao(c,max,sessao);
  if(id)snprintf(c->id,sizeof c->id,"xtream:@%08x:1",id);
  return n;
}
int m3u_fonte_canais(unsigned id,M3uCanal *c,int max,uint64_t sessao){(void)id;return m3u_canais_sessao(c,max,sessao);}
int stalker_fonte_canais(unsigned id,StalkerCanal *c,int max,uint64_t sessao){(void)id;return stalker_canais_sessao(c,max,sessao);}
int xtream_fonte_estado(unsigned id,int *http){(void)id;*http=200;return XT_OK;}
int xtream_fonte_total(unsigned id){(void)id;return 1;}
int m3u_fonte_total(unsigned id){(void)id;return 1;}
char *rede_baixar_medido_controle(const char *url,int segundos,const char *const *cab,const RedeControle *ctl,RedeMedida *med){
  (void)url;(void)segundos;(void)cab;(void)ctl;(void)med;
  // In All mode the native list must already have been handed to the UI.
  if(cargaFonte==0)assert(parcial>0);
  chamadas[4]++;
  return strdup("{\"metas\":[{\"id\":\"addon:one\",\"name\":\"Addon One\",\"type\":\"tv\",\"genres\":[\"News\"]}]}");
}
static void executar(int fonte,int cancel){
  memset(chamadas,0,sizeof chamadas);parcial=0;cancelar=cancel;
  cargaFonte=fonte;cargaSessao=0;cargaCancelada=0;sNFontes=0;nAddonsCarga=0;
  nContasCarga=0;
  for(int tipo=1;tipo<=3;tipo++)if(fonte==0||fonte==tipo)
    contasCarga[nContasCarga++]=(IptvConta){.tipo=tipo,.ativa=1};
  atomic_store(&pendPronto,0);
  if(fonte==0||fonte==4){
    nAddonsCarga=1;addonsCarga=calloc(1,sizeof *addonsCarga);assert(addonsCarga);
    strcpy(addonsCarga[0].base,"https://addon.invalid");strcpy(addonsCarga[0].nome,"Addon");addonsCarga[0].ativo=1;
    addonsCarga[0].nCatalogos=1;
    strcpy(addonsCarga[0].catalogos[0].tipo,"tv");strcpy(addonsCarga[0].catalogos[0].id,"live");strcpy(addonsCarga[0].catalogos[0].nome,"Live");
  }
  pthread_t t;assert(!pthread_create(&t,NULL,fioGuia,NULL));int final=0;
  for(int i=0;i<2000;i++){
    pthread_mutex_lock(&publicacaoTrava);int pend=atomic_load_explicit(&pendPronto,memory_order_acquire);
    if(pend==2){
      assert(sNCanais>0);if(!parcial)assert(!strcmp(sCanais[0].id,"xtream:1"));parcial++;
      if(cancelar)cargaCancelada=1;
      atomic_store(&pendPronto,0);pthread_cond_signal(&publicacaoSinal);
    }else if(pend==1)final=1;
    pthread_mutex_unlock(&publicacaoTrava);
    if(final)break;
    struct timespec pausa={0,1000000};nanosleep(&pausa,NULL);
  }
  assert(final);pthread_join(t,NULL);
  if(cancel){assert(parcial==1&&!chamadas[4]&&!chamadas[2]&&!chamadas[3]);return;}
  for(int i=1;i<=4;i++)assert(chamadas[i]==(fonte==0||fonte==i));
  assert(sNCanais==(fonte==0?4:1));assert(sOk);
}
static void duasContas(void) {
  cargaFonte=1;cargaSessao=0;cargaCancelada=0;sNFontes=nAddonsCarga=0;
  nContasCarga=2;contasCarga[0]=(IptvConta){.tipo=IPTV_XTREAM,.ativa=1};
  contasCarga[1]=(IptvConta){.tipo=IPTV_XTREAM,.id=19,.ativa=1};
  atomic_store(&pendPronto,0);fioGuia(NULL);
  assert(sNCanais==2 && !strcmp(sCanais[0].id,"xtream:1") && !strcmp(sCanais[1].id,"xtream:@00000013:1"));
  assert(sCanais[0].epg==-1 && sCanais[1].epg==-1);
  puts("PASS: two accounts of the same native type both reach the guide with separate identities");
}
int main(void){duasContas();
  for(int i=0;i<=4;i++)executar(i,0);
  executar(0,1);
  nCanais=2;nCats=1;nCatsVisiveis=1;catsVisiveis[0]=0;nFavOrd=0;catIni[0]=0;catN[0]=2;
  memset(canais,0,sizeof canais);strcpy(canais[0].id,"xtream:1");strcpy(canais[1].id,"xtream:2");
  guia_focar_id("xtream:2");assert(focoCol==1&&!focoPend[0]);
  guia_focar_id("xtream:missing");assert(!strcmp(focoPend,"xtream:missing"));
  guia_focar_id("xtream:1");assert(focoCol==0&&!focoPend[0]);
  puts("guide loading: native lists publish before add-ons; source selection and cancellation PASS");
}
