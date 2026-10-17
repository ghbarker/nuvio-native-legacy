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
int main(void){
  for(int i=0;i<=4;i++)executar(i,0);
  executar(0,1);
  puts("guide loading: native lists publish before add-ons; source selection and cancellation PASS");
}
