#include "player.h"
#include "video.h"
#include <math.h>
#include "proximopreparo.h"
#include "perfis.h"
#include "perfisessao.h"
#include "addons.h"
#include "debrid.h"
#include "diagnostico.h"
#include "marco.h"
#include "idioma.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdatomic.h>
static int aguardandoFonte, fonteEscolhida;
static FonteFalhaSessao *fonteFalha;
static unsigned fonteFalhaLista,fontePedidoGeracao;
static int fonteFalhaEsperando,fonteFalhaN,fonteFalhaManual,antAberta,fonteVODAutomatica,voltaAtiva,fonteVODTentativas;
static double fonteFalhaPos;static Stream voltaFonte;
static Uint32 fonteVODDesde,fonteManualDesde;
static _Atomic unsigned stalkerGeracao=10;
static int fontePendenteTipo,fontePendenteRenovando;static unsigned fontePendenteGeracao;static char fontePendenteId[64];
enum{FJOB_NONE,FJOB_ADDON,FJOB_CANAL,FJOB_STALKER,FJOB_PREPARO,FJOB_PROXIMA_VOD,FJOB_FALHA,FJOB_PREPARO_FALHA};
static void alvoPlayer(char *,size_t);
static void buscarParaPlayer(void);
static void provaZerar(void);
static int pedirFonteJob(int,unsigned,const char *,int);
#include "app-route-extracted.inc"
/* Typed boundaries record ownership and order; no real decoder/network claims. */
static int opened=1,indexNow=0,seasonNow=1,episodeNow=2,manualQueued,reloadCalls,reloadAccepted=1,profileChangesOnReload,liveMode;
static double currentPosition=123.25;
static char currentId[64]="ttfixture:1:2",workerId[64]="ttfixture:1:3",installedId[64];
static int ready,inProgress,pumpCalls,cancelCalls,consumeCalls,closeCalls,openCalls,pointCalls,searchCalls,subtitleCalls,releaseCalls,adoptCalls,cancelTokenCalls,errorCalls,policyFreed,resetAttemptCalls;
static int switchOnClose,switchOnConsume,allocationFails;
static int orderCounter,orderAdopt,orderClose,orderOpen,orderDefine;
static unsigned workerGeneration=17,streamGeneration=22;
static int installedCount;static ProximoPreparoResultado offered;
static CatItem ci;
static Stream links[2];static FonteFalhaSessao *newPolicy=(void*)0x2222;
int player_indice(void){return indexNow;}
int player_aberto(void){return opened;}
int player_quer_sair(void){return 0;}
const char *player_id_canal(void){return liveMode?"live-fixture":"";}
int player_transporte_pedido_pendente(void){return manualQueued;}
void player_episodio_pedido_liberar(void){releaseCalls++;}
static void alvoPlayer(char *s,size_t n){snprintf(s,n,"%s",currentId);}
int cat_id_stream(int i,int t,int e,char *s,unsigned n){if(i<0||t<1||e<1)return 0;snprintf(s,n,"ttfixture:%d:%d",t,e);return 1;}
int proximopreparo_consumir(const char *id,ProximoPreparoResultado *out){consumeCalls++;if(switchOnConsume)perfis_invalidar_sessao();if(ready&&!strcmp(id,workerId)){*out=offered;ready=0;return 1;}return 0;}
int proximopreparo_em_curso(const char *id){return inProgress&&!strcmp(id,workerId);}
void proximopreparo_liberar_resultado(ProximoPreparoResultado *r){(void)r;}
void proximopreparo_cancelar(void){cancelCalls++;ready=inProgress=0;}
unsigned proximopreparo_geracao(void){return workerGeneration;}
void proximopreparo_passo(const char *id){(void)id;pumpCalls++;}
void player_toast(const char *s,Uint32 d){(void)s;(void)d;}
const char *i18n(const char *s){return s;}
void player_precarregar_adotar(unsigned t){assert(t==71);adoptCalls++;orderAdopt=++orderCounter;}
void player_precarregar_cancelar(unsigned t){assert(t==71);cancelTokenCalls++;}
void diagnostico_cancelar_vazao(void){}
void player_encerrar(void){closeCalls++;orderClose=++orderCounter;opened=0;if(switchOnClose)perfis_invalidar_sessao();}
void player_abrir(int i,const char *url){assert(i==0&&!url);openCalls++;orderOpen=++orderCounter;opened=1;}
void player_definir_episodio(int t,int e){seasonNow=t;episodeNow=e;orderDefine=++orderCounter;snprintf(currentId,sizeof currentId,"ttfixture:%d:%d",t,e);}
void debrid_definir_episodio(int t,int e){assert(t==seasonNow&&e==episodeNow);}
void cat_apontar_episodio(int i,int t,int e){assert(i==0&&t==seasonNow&&e==episodeNow);pointCalls++;}
void fontefalha_sessao_liberar(FonteFalhaSessao *s){if(s)policyFreed++;}
void player_definir_tentativa(int t,int teto){assert(!t&&!teto);resetAttemptCalls++;}
static void provaZerar(void){}
unsigned stream_lista_geracao(void){return streamGeneration;}
void stream_definir_alvo(const char *id){snprintf(installedId,sizeof installedId,"%s",id);}
void stream_definir_lista_idade(const Stream *s,int n,Uint32 age){assert(s==links&&n==2&&age==123);if(!allocationFails){streamGeneration++;installedCount=n;}}
int stream_n(void){return installedCount;}
int stream_lista_do_alvo(const char *id){return !strcmp(id,installedId);}
void stream_definir_lista(const Stream *s,int n){assert(!s&&!n);installedCount=0;streamGeneration++;}
void player_erro_fonte(void){errorCalls++;}
static int pedirFonteJob(int type,unsigned g,const char *id,int renew){assert(type==FJOB_FALHA&&g==fontePedidoGeracao&&!id&&!renew);return 0;}
const CatItem *cat_item(int i){return i==0?&ci:NULL;}
void addons_buscar_legendas(const char *id,const char *type){assert(!strcmp(id,currentId)&&!strcmp(type,"series"));subtitleCalls++;}
void marco(const char *s){(void)s;}
static void buscarParaPlayer(void){searchCalls++;}
double video_pos(void){return currentPosition;}
int video_recarregar(void){reloadCalls++;if(profileChangesOnReload)perfis_invalidar_sessao();return reloadAccepted;}
Uint32 SDL_GetTicks(void){return 8764;}
static void seed(void){
 memset(&proximoIntento,0,sizeof proximoIntento);opened=1;indexNow=0;seasonNow=1;episodeNow=2;manualQueued=reloadCalls=profileChangesOnReload=liveMode=0;reloadAccepted=1;currentPosition=123.25;
 snprintf(currentId,sizeof currentId,"ttfixture:1:2");snprintf(workerId,sizeof workerId,"ttfixture:1:3");snprintf(ci.tipo,sizeof ci.tipo,"series");
 ready=inProgress=pumpCalls=cancelCalls=consumeCalls=closeCalls=openCalls=pointCalls=searchCalls=subtitleCalls=releaseCalls=adoptCalls=cancelTokenCalls=errorCalls=policyFreed=resetAttemptCalls=0;
 switchOnClose=switchOnConsume=allocationFails=orderCounter=orderAdopt=orderClose=orderOpen=orderDefine=0;
 fonteFalha=(void*)0x1111;fonteFalhaManual=8;fonteFalhaPos=100;fonteFalhaN=9;fonteFalhaEsperando=1;fonteVODAutomatica=voltaAtiva=1;fonteVODTentativas=7;fonteVODDesde=fonteManualDesde=999;antAberta=4;
 installedCount=8;streamGeneration=22;workerGeneration=17;aguardandoFonte=2;fonteEscolhida=8;
 offered=(ProximoPreparoResultado){.fontes=links,.n=2,.indiceFonte=1,.politica=newPolicy,.token=71,.sessaoPerfil=perfis_sessao_geracao(),.idadeMs=123};
}
static void intent(int e,int pointing){proximoIntento.tem=1;proximoIntento.titulo=0;proximoIntento.t=1;proximoIntento.e=e;proximoIntento.apontar=pointing;proximoIntento.sessaoPerfil=perfis_sessao_geracao();proximoIntento.geracao=workerGeneration;snprintf(proximoIntento.atual,sizeof proximoIntento.atual,"%s",currentId);snprintf(proximoIntento.alvo,sizeof proximoIntento.alvo,"ttfixture:1:%d",e);}
int main(void){
 seed();abrirProximoEpisodio(1,1,0);assert(closeCalls==1&&openCalls==1&&episodeNow==1&&searchCalls==1&&!pointCalls);assert(!fonteFalha&&fonteFalhaManual==-1&&fonteFalhaN==0&&fonteFalhaPos==-1&&fonteVODTentativas==0&&policyFreed==1);
 seed();ready=1;abrirProximoEpisodio(1,3,0);assert(closeCalls==1&&openCalls==1&&!searchCalls&&!pointCalls&&episodeNow==3);assert(fonteFalha==newPolicy&&fonteFalhaManual==-1&&fonteFalhaN==0&&fonteFalhaPos==-1&&fonteEscolhida==1&&aguardandoFonte==2);assert(orderClose<orderOpen&&orderOpen<orderDefine);assert(subtitleCalls==1);
#ifdef NV_ANDROID
 assert(adoptCalls==1&&orderAdopt<orderClose&&!cancelTokenCalls);
#endif
 seed();ready=1;abrirProximoEpisodio(1,3,1);assert(pointCalls==1);
 seed();inProgress=1;abrirProximoEpisodio(1,3,0);assert(proximoIntento.tem&&!closeCalls&&!openCalls&&!searchCalls&&episodeNow==2&&!pointCalls);inProgress=0;ready=1;proximoPreparoPasso();assert(episodeNow==3&&!pointCalls&&!proximoIntento.tem&&!searchCalls);
 /* App ordering: old auto intent is pumped before manual mailbox; queued manual must win without consuming/canceling the warm worker. */
 seed();intent(3,1);ready=1;manualQueued=1;proximoPreparoPasso();assert(!proximoIntento.tem&&!pumpCalls&&!consumeCalls&&!closeCalls&&!cancelCalls&&ready);manualQueued=0;abrirProximoEpisodio(1,1,0);assert(episodeNow==1&&!pointCalls&&!adoptCalls&&searchCalls==1);
 seed();intent(3,1);ready=1;manualQueued=1;proximoPreparoPasso();manualQueued=0;abrirProximoEpisodio(1,3,0);assert(episodeNow==3&&!pointCalls&&!searchCalls);
 seed();intent(3,0);perfis_invalidar_sessao();proximoPreparoPasso();assert(!proximoIntento.tem&&cancelCalls==1&&!closeCalls&&!openCalls);
 seed();ready=1;switchOnConsume=1;abrirProximoEpisodio(1,3,0);assert(!openCalls&&!closeCalls&&!adoptCalls&&cancelCalls==1&&releaseCalls==1);
 seed();ready=1;switchOnClose=1;abrirProximoEpisodio(1,3,0);assert(closeCalls==1&&!openCalls&&cancelCalls==1);
#ifdef NV_ANDROID
 assert(adoptCalls==1&&cancelTokenCalls==1);
#endif
 seed();ready=1;allocationFails=1;abrirProximoEpisodio(1,3,0);assert(errorCalls==1&&fonteEscolhida==-1&&!installedCount&&!searchCalls&&fonteFalha==NULL);
#ifdef NV_ANDROID
 assert(cancelTokenCalls==1);
#endif
 seed();intent(3,0);workerGeneration++;proximoPreparoPasso();assert(cancelCalls==1&&!openCalls&&releaseCalls==1);
 seed();indexNow=-1;abrirProximoEpisodio(1,3,0);assert(!consumeCalls&&!closeCalls&&releaseCalls==1);
 seed();aguardandoFonte=0;recarregarFonteVOD();assert(reloadCalls==1&&fonteVODDesde==8765&&fonteManualDesde==999&&fonteFalhaPos==100);assert(fonteFalha==(void*)0x1111&&fonteFalhaManual==8&&fonteFalhaN==9&&fonteVODTentativas==7&&fonteEscolhida==8&&streamGeneration==22&&!searchCalls&&!closeCalls&&!pointCalls);
 seed();aguardandoFonte=0;fonteVODAutomatica=0;currentPosition=0;recarregarFonteVOD();assert(reloadCalls==1&&fonteManualDesde==8765&&fonteVODDesde==999&&fonteFalhaPos==100);
 seed();aguardandoFonte=0;reloadAccepted=0;recarregarFonteVOD();assert(reloadCalls==1&&fonteVODDesde==999&&fonteFalhaPos==100);
 seed();aguardandoFonte=0;profileChangesOnReload=1;recarregarFonteVOD();assert(reloadCalls==1&&fonteVODDesde==999&&fonteFalhaPos==100);
 seed();aguardandoFonte=0;currentPosition=NAN;recarregarFonteVOD();assert(reloadCalls==1&&fonteFalhaPos==100&&fonteVODDesde==8765);
 seed();aguardandoFonte=0;opened=0;recarregarFonteVOD();assert(!reloadCalls);
 seed();aguardandoFonte=0;liveMode=1;recarregarFonteVOD();assert(!reloadCalls);
 seed();aguardandoFonte=2;recarregarFonteVOD();assert(!reloadCalls);
 puts("PASS 12 episode + 8 reload actual app router cases: manual/automatic distinction, warm adoption, stale-auto priority, profile ownership and source-budget reset/failure");return 0;
}
