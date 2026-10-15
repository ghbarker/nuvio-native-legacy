/* Review-only executable: compile the actual complete detail.c dispatcher.
 * Unrelated UI branches link to fail-fast traps; no renderer or remote APIs.
 * Private static state seeds a settled title page rather than calling network
 * and rendering setup. Every tested OK down/up goes through detail_evento.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef DETAIL_SOURCE
#error DETAIL_SOURCE must pin an actual product source file
#endif
#include DETAIL_SOURCE
#include "dados.h"
#include "manual_gate.h"

#ifdef NV_TOUCH_UI
float nv_layout_w=1080.0f,nv_layout_h=1920.0f;
int layout_modo_mobile(void) {return 1;}
float trocaarte_visivel(void) {return 0;}
#endif
static CatItem testTitle;
static CatEp testEpisodes[4];
static Stream testSource;
int stream_n(void) {return 1;}
const Stream *stream_item(int i) {return i==0 ? &testSource : NULL;}
int stream_automatico(void) {return 0;}
int stream_grupo_sem_perda(int i) {return i==0;}
static int testManual, testLong, testMenu, checks, failures;
static int menuT, menuE;
static Uint32 testClock=1000;
Uint32 SDL_GetTicks(void) { return testClock; }
const CatItem *cat_item(int i) { return i == 0 ? &testTitle : NULL; }
int cat_n_episodios(int i) { return i == 0 ? 4 : 0; }
const CatEp *cat_episodio(int i,int n) { return i==0 && n>=0 && n<4 ? &testEpisodes[n] : NULL; }
int ajustes_fonte_manual(void) { return testManual; }
int ajustes_cw_concluido(void) { return 90; }
int reacao_aberta(void) { return 0; }
int trailer_cheia(void) { return 0; }
int trocaarte_aberto(void) { return 0; }
int amtui_aberta(void) { return 0; }
int detmais_aberto(void) { return 0; }
int episodios_menu_aberto(void) { return 0; }
int ctxhold_evento(CtxHold *h,const SDL_Event *e,int aceita) { (void)h;(void)e;assert(!aceita);return 0; }
int ponteiro_ok_longo(void) { return testLong; }
void episodios_menu_visto(int i,int t0,int e0,const char *nome) { assert(i==0 && nome && nome[0]);testMenu++;menuT=t0;menuE=e0; }

static void verify(int ok,const char *name) {
  checks++;printf("%s %s\n",ok?"PASS":"FAIL",name);if(!ok)failures++;
}
static void seed(int manual,int history,int col,int season) {
  memset(&testTitle,0,sizeof testTitle);
  snprintf(testTitle.imdb,sizeof testTitle.imdb,"tt1234567");
  snprintf(testTitle.tipo,sizeof testTitle.tipo,"series");
  testTitle.nTemporadas=2;testTitle.temporadas[0]=1;testTitle.temporadas[1]=2;
  if(history){testTitle.progresso=42;testTitle.temporada=1;testTitle.episodio=2;}
  for(int i=0;i<4;i++){testEpisodes[i].temporada=1+i/2;testEpisodes[i].episodio=1+i%2;snprintf(testEpisodes[i].nome,sizeof testEpisodes[i].nome,"Episode %d",i);}
  aberto=1;saindo=0;carro=0;nivel=1;idx=0;temporada=season;
  memset(&foco,0,sizeof foco);foco.fileira=SEC_EPISODIOS;foco.coluna=col;
  pedReproduzir=pedFontes=pedDoInicio=0;okDesceEm=0;comPicando=0;
  testManual=manual;testLong=testMenu=0;menuT=menuE=0;
  fontepref_definir_perfil(history?1:2);
}
static void click(void) {
  SDL_Event event;memset(&event,0,sizeof event);event.type=SDL_KEYDOWN;event.key.keysym.sym=SDLK_RETURN;
  detail_evento(&event);testClock+=80;event.type=SDL_KEYUP;detail_evento(&event);
}
int main(void) {
  char label[160];
  const char *dir=getenv("NUVIO_DADOS");assert(dir && strstr(dir,"issue513"));
  dados_iniciar(dir);assert(!strcmp(dados_dir(),dir));
  snprintf(testSource.provedor,sizeof testSource.provedor,"Test addon");
  snprintf(testSource.bingeGroup,sizeof testSource.bingeGroup,"series|English");
  snprintf(testSource.url,sizeof testSource.url,"https://example.invalid/episode.mkv");
  testSource.altura=1080;
  fontepref_definir_perfil(1);
  verify(fontepref_guardar("tt1234567:1:2",&testSource)==1,"save actual manual source preference for viewed episode");
  /* Every explicit card: ongoing episode, earlier/later episode, next season.
   * CW progress and saved-source presence affect later playback routing, not
   * the card's intent. Asking for sources must happen before any fast path.
   */
  for(int history=0;history<2;history++)for(int season=0;season<2;season++)for(int col=0;col<2;col++){
    seed(1,history,col,season);click();
    snprintf(label,sizeof label,"manual card CW=%d S%dE%d opens source selection without Play",history,season+1,col+1);
    int sources=detail_pediu_fontes(), play=detail_pediu_reproduzir();
    verify(sources==1 && play==0,label);
    char target[64];snprintf(target,sizeof target,"tt1234567:%d:%d",season+1,col+1);
    int remembered=fontepref_escolher_manual(target);
    verify(remembered==(history?0:-1),"actual title-wide source memory matches all episodes after viewing");
    int wouldOpenSheet=sources || (play && existing_manual_sheet_gate(remembered));
    verify(wouldOpenSheet,"actual source-selection request or existing app gate shows sheet");
    int t0=0,e0=0;verify(detail_ep_foco(&t0,&e0) && t0==season+1 && e0==col+1,"source selection keeps exact focused episode");
    verify(!detail_pediu_fontes()&&!detail_pediu_reproduzir(),"requests consumed once");
  }
  for(int history=0;history<2;history++){
    seed(0,history,1,0);click();verify(detail_pediu_reproduzir()==1&&!detail_pediu_fontes(),"automatic card still requests playback");
    seed(1,history,1,0);nivel=0;botao=0;click();verify(detail_pediu_reproduzir()==1&&!detail_pediu_fontes(),"primary Play/Resume keeps existing playback request");
    seed(1,history,1,0);detail_pedir_reproduzir();verify(detail_pediu_reproduzir()==1&&!detail_pediu_fontes(),"Continue Watching direct Play keeps existing request");
    seed(1,history,1,1);testLong=1;click();verify(testMenu==1&&menuT==2&&menuE==2&&!detail_pediu_fontes()&&!detail_pediu_reproduzir(),"long press still opens focused episode context menu");
  }
  seed(1,1,0,0);SDL_Event orphan;memset(&orphan,0,sizeof orphan);orphan.type=SDL_KEYUP;orphan.key.keysym.sym=SDLK_RETURN;detail_evento(&orphan);
  verify(!detail_pediu_fontes()&&!detail_pediu_reproduzir(),"orphan OK release cannot start playback or open sources");
  printf("RESULT checks=%d failures=%d\n",checks,failures);return failures?1:0;
}
