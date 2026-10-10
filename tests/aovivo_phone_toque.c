/* Real Live TV button drawing and existing player decisions. The decoder,
 * channel list and account services are inert; no source/provider requests. */
#include <stdio.h>
static int preferenceAttempts;
static FILE *avFixtureOpen(const char *path,const char *mode) { (void)path;(void)mode;preferenceAttempts++;return NULL; }
#define fopen avFixtureOpen
#define NV_PLAYER_ERROR_DOUBLES_ONLY 1
#include "player_error_toque.c"
#undef fopen
#include "../src/aovivoui.c"
#include "../src/aovivo.c"
static PonteiroRolagemFn scrolling;
static int favourites,pauseCalls,seekCalls,guideAvailable=1;
static double windowDuration=120;
char *SDL_GetBasePath(void) { return NULL; }
void SDL_free(void *p) { (void)p; }
void dados_marcar_sujo(int i) { (void)i; }
int txt_largura(TxtEstilo e,const char *s) { return txt_linha(e,s,0,0,0,255).w; }
void ponteiro_rolagem(PonteiroRolagemFn fn) { scrolling=fn; }
double video_duracao(void) { return windowDuration; }
void video_pausar(int paused) { (void)paused;pauseCalls++; }
void video_buscar(double position) { assert(position>=0);seekCalls++; }
int guia_info_canal(const char *id,int *n,int *total,char *cat,size_t cap) {
  (void)id;(void)n;(void)total;(void)cat;(void)cap;return guideAvailable;
}
void guia_alternar_favorito(const char *id) { assert(!strcmp(id,"fixture-channel"));favourites++; }
void plrui_disco_osd(GfxRect r,float a) { plrui_botao_repouso(r,a); }
int ajustes_vidro(void) { return 0; }
void guia_logo_desenhar(const char *logo,const char *name,GfxRect r,float mw,float mh,float tone,float a) {
  (void)logo;(void)name;(void)mw;(void)mh;(void)tone;(void)a;bounds(r);
}
float plrui_kicker(const char *s,float x,float y,int r,int g,int b,float a) {
  TxtLinha t=txt_linha(TXT_MINI,s,r,g,b,255);if(x>=0)txt_desenhar_alpha(t,x,y,a);return t.w;
}
static void channel(int error) {
  aberto=canalSessao=comVideo=1;mini=saindo=sheet=tracks=episodes=0;erroFonte=error;
  anim=entrada=1;tocando=visivel=1;pedFontes=pedRecarregar=pedGuiaCheio=pedFaixas=infoAV=0;
  botaoAV=0;zapEst=(AoVivoZap){0};avAtraso=25;avLat0=3;avPausaDesde=0;
  snprintf(itemCanal.imdb,sizeof itemCanal.imdb,"fixture-channel");
  permitido=guideAvailable=1;windowDuration=120;nTargets=layers=0;scrolling=NULL;
  memset(&toqueLista,0,sizeof toqueLista);toqueRol=0;testClipOn=0;
  player_toque_modal_guarda(pode);avToquePreparar(error);
}
static void decisions(void) {
  for(int id=0;id<AV_B_N;id++) {
    channel(0);int fav=favourites,pc=pauseCalls,sc=seekCalls,prefs=preferenceAttempts;
    AoVivoOsd o={0};o.nBotoes=avBotoes(o.botoes);
    for(int i=0;i<o.nBotoes;i++)if(o.botoes[i]==id)o.foco=i;
    fileiraBotoes(&o,AV_BTN_Y,1);int found=-1;
    for(int i=0;i<nTargets;i++)if(targets[i].a==id)found=i;
    assert(found>=0);targets[found].ativar(targets[found].a,targets[found].b);
    switch(id) {
      case AV_B_PAUSA:assert(!tocando&&pauseCalls==pc+1);break;
      case AV_B_AOVIVO:assert(seekCalls==sc+1&&!avAtraso);break;
      case AV_B_GUIA:assert(pedGuiaCheio);break;
      case AV_B_ANT:assert(zapEst.pend==-1);break;
      case AV_B_PROX:assert(zapEst.pend==1);break;
      case AV_B_FAV:assert(favourites==fav+1);break;
      case AV_B_AUDIO:assert(pedFaixas==1);break;
      case AV_B_LEGENDA:assert(pedFaixas==2);break;
      case AV_B_INFO:assert(infoAV);break;
      case AV_B_RECARREGAR:assert(pedRecarregar);break;
      case AV_B_FONTE:assert(pedFontes);break;
      case AV_B_ASPECTO:assert(preferenceAttempts==prefs+1&&toastAte==SDL_GetTicks()+PLR_TOAST_MS);break;
    }
  }
  for(int block=0;block<14;block++) {
    channel(0);int q=avToqueQuadro;
    if(block==0)aberto=0;if(block==1)mini=1;if(block==2)saindo=1;
    if(block==3)canalSessao=0;if(block==4)erroFonte=1;if(block==5)permitido=0;
    if(block==6)sheet=1;if(block==7)tracks=1;if(block==8)episodes=1;
    if(block==9)zapEst.pend=1;if(block==10)anim=0;
    if(block==11)snprintf(itemCanal.imdb,sizeof itemCanal.imdb,"changed-channel");
    if(block==12){permitido=0;avToquePreparar(0);permitido=1;avToquePreparar(0);}if(block==13)q--;
    avToqueAcao(AV_B_FONTE,0,q);assert(!pedFontes);
  }
  channel(0);windowDuration=0;int pc=pauseCalls,sc=seekCalls;
  avToqueAcao(AV_B_PAUSA,0,avToqueQuadro);avToqueAcao(AV_B_AOVIVO,0,avToqueQuadro);
  assert(pauseCalls==pc&&seekCalls==sc);
  guideAvailable=0;int fav=favourites;avToqueAcao(AV_B_FAV,0,avToqueQuadro);assert(fav==favourites);
}
#ifndef NV_AOVIVO_DOUBLES_ONLY
int main(void) {
  const float sizes[][2]={{1080,1920},{1080,2340},{2160,1080},{2520,1080}};
  const float scales[]={1,1.2f,1.3f,1.5f};int cases=0;
  for(int t=0;t<4;t++)for(int s=0;s<4;s++) {
    nv_layout_w=sizes[t][0];nv_layout_h=sizes[t][1];testUI=testScale=scales[s];
    channel(0);AoVivoOsd o={0};o.nBotoes=AV_B_N;for(int i=0;i<AV_B_N;i++)o.botoes[i]=i;
    o.foco=0;o.aspecto="A very long translated zoom mode label";
    fileiraBotoes(&o,AV_BTN_Y,1);assert(nTargets>0&&!layers&&scrolling);
    int frame=avToqueQuadro;nTargets=0;avToquePreparar(0);fileiraBotoes(&o,AV_BTN_Y,1);
    assert(avToqueQuadro==frame&&targets[0].b==frame);
    for(int i=0;i<nTargets;i++)assert(targets[i].a>=0&&targets[i].a<AV_B_N&&targets[i].ativar==toqueBotao);
    PonteiroRolagem e={PONT_ROL_INICIO,0,0,0,AV_X*testScale,(AV_BTN_Y+10)*testScale};
    if(toqueLista.maximo>0) {
      assert(scrolling(&e));e.fase=PONT_ROL_MOVER;e.delta=-10000;assert(scrolling(&e));
      nTargets=0;fileiraBotoes(&o,AV_BTN_Y,1);
      assert(toqueRol==toqueLista.maximo);assert(targets[nTargets-1].a==AV_B_ASPECTO);
      assert(targets[nTargets-1].x+targets[nTargets-1].w<=NV_TELA_W-AV_X+.01f);
      e.fase=PONT_ROL_INERCIA;assert(!scrolling(&e));
    }
    decisions();channel(1);
    aovivo_erro_desenharCorpo_("An intentionally long channel name which must stay inside its card",NULL,
      "The selected source could not be opened by the decoder after a connection failure",
      "Reload the selected source or choose a different source from the list",1);
    assert(layers==1&&nTargets==4&&!targets[0].ativar);
    const int actions[]={AV_B_RECARREGAR,AV_B_FONTE,AV_B_GUIA};
    for(int i=1;i<4;i++) {
      assert(targets[i].a==actions[i-1]&&targets[i].ativar==toqueErro);
      pedRecarregar=pedFontes=pedGuiaCheio=0;targets[i].ativar(targets[i].a,targets[i].b);
      assert((i==1&&pedRecarregar)||(i==2&&pedFontes)||(i==3&&pedGuiaCheio));
      permitido=0;pedRecarregar=pedFontes=pedGuiaCheio=0;targets[i].ativar(targets[i].a,targets[i].b);
      assert(!pedRecarregar&&!pedFontes&&!pedGuiaCheio);permitido=1;
    }
    avToqueAcao(AV_B_AUDIO,1,avToqueQuadro);assert(!pedFaixas);
    cases++;
  }
  nv_layout_w=1920;nv_layout_h=1080;testUI=testScale=1;channel(0);
  AoVivoOsd o={.nBotoes=1,.botoes={AV_B_FONTE},.foco=0};fileiraBotoes(&o,AV_BTN_Y,1);assert(!nTargets);
  aovivo_erro_desenharCorpo_("TV",NULL,"Error","Try another source",1);assert(!nTargets&&!layers);
  printf("aovivo_phone_toque: %d real OSD/error layouts, all action IDs, direct scroll, current channel/overlay/state guards and TV PASS\n",cases);
}
#endif
