/* Production miniature drawing/actions; native video, font and external
 * session services are inert. No source loading, seek or account requests. */
#define NV_ANDROID 1
#define NV_TOUCH_PREVIEW 1
#define SDL_MAIN_HANDLED 1
#ifdef _WIN32
#include <time.h>
static struct tm *mini_localtime_r(const time_t *t, struct tm *out) {
  struct tm *p=localtime(t); if(p)*out=*p; return p?out:NULL;
}
#define localtime_r mini_localtime_r
#endif
#include "../src/player.c"
#include <assert.h>
float nv_layout_w=1080,nv_layout_h=2340;
static int permitido=1,orientation=-1,orientationCalls,stops,windows,layers,nTargets,draws;
static int vw=1920,vh=1080;
static PonteiroAlvo targets[64];
static GfxRect window;
static float testScale=1, testUI=1.5f;
static int testClipOn;
static GfxRect testClip;
static int pode(void) { return permitido; }
static void bounds(GfxRect r) {
  if(testClipOn) {
    float right=fminf(r.x+r.w,testClip.x+testClip.w),bottom=fminf(r.y+r.h,testClip.y+testClip.h);
    r.x=fmaxf(r.x,testClip.x);r.y=fmaxf(r.y,testClip.y);r.w=right-r.x;r.h=bottom-r.y;
    if(r.w<=0||r.h<=0)return;
  }
  assert(r.w>=0&&r.h>=0&&r.x>=-.01f&&r.y>=-.01f);
  assert(r.x+r.w<=nv_layout_w/testScale+.01f&&r.y+r.h<=nv_layout_h/testScale+.01f);
}
float gfx_escala(void) { return testScale; }
float gfx_escala_ui(void) { return testUI; }
int video_largura(void) { return vw; }
int video_altura(void) { return vh; }
int video_recorte_fonte(void) { return 0; }
void video_janela(int x,int y,int w,int h) { windows++;window=(GfxRect){x,y,w,h};assert(x>=0&&y>=0&&w>=0&&h>=0&&x+w<=nv_layout_w+1&&y+h<=nv_layout_h+1); }
void video_janela_fonte(int sx,int sy,int sw,int sh,int x,int y,int w,int h) {
  (void)sx;(void)sy;(void)sw;(void)sh;video_janela(x,y,w,h);
}
void video_parar(void) { stops++; }
void fontevolta_esquecer(const char *porque) { (void)porque; }
int video_pronto(void) { return 0; }
int video_falhou(void) { return 0; }
const char *video_url_atual(void) { return ""; }
int dvtela_ativa(void) { return 0; }
const CatItem *cat_item(int n) { (void)n;return NULL; }
int stream_atual(void) { return -1; }
const Stream *stream_item(int n) { (void)n;return NULL; }
int xtream_e_id(const char *s) { (void)s;return 0; }
int epg_agora(int n,time_t t,EpgProg *p) { (void)n;(void)t;(void)p;return 0; }
void video_velocidade(int v) { (void)v; }
void legsync_encerrar(void) {}
void pausao_fechar(void) {}
void reacao_fechar(void) {}
void episodios_fechar(void) {}
void intro_desligar(void) {}
void seekr_desligar(void) {}
void mkvass_parar(void) {}
void mkvass_video_aberto(int v) { (void)v; }
void legenda_desligar(void) {}
void servidores_reproducao_fim(const char *u,double p,double d) { (void)u;(void)p;(void)d; }
double video_creditos(void) { return 0; }
double intro_creditos_seg(void) { return 0; }
int intro_creditos_aniskip(void) { return 0; }
double cred_escolher(const CredEntrada *e,int *f) { (void)e;*f=0;return 0; }
double intro_fim_estimado(double d) { return d; }
double intro_creditos_janela(double d) { (void)d;return 0; }
void android_player_tela_cheia(int v) { orientation=v;orientationCalls++; }
void ponteiro_camada(void) { layers++;nTargets=0; }
void ponteiro_alvo(float x,float y,float w,float h,PonteiroFn f,PonteiroFn a,int u,int v) {
  assert(nTargets<64);bounds((GfxRect){x,y,w,h});
  targets[nTargets++]=(PonteiroAlvo){x,y,w,h,f,a,u,v,0,NULL};
}
const char *i18n(const char *s) { return s; }
Uint32 SDL_GetTicks(void) { return 5000; }
void plrui_material(GfxRect r,float radius,int modal,float a) { (void)radius;(void)modal;(void)a;bounds(r);draws++; }
void plrui_botao_repouso(GfxRect r,float a) { (void)a;bounds(r); }
void plrui_limpar_sep(char *s) { (void)s; }
void gfx_furo_raio(GfxRect r,float raio) { (void)raio;bounds(r);draws++; }
void gfx_cor(GfxRect r,float raio,float cr,float cg,float cb,float a) { (void)raio;(void)cr;(void)cg;(void)cb;(void)a;bounds(r); }
void gfx_recorte(float x,float y,float w,float h) { testClip=(GfxRect){x,y,w,h};testClipOn=1; }
void gfx_sem_recorte(void) { testClipOn=0; }
void gfx_icone(GfxRect r,const char *id,float cr,float cg,float cb,float a) { (void)id;gfx_cor(r,0,cr,cg,cb,a); }
TxtLinha txt_linha(TxtEstilo e,const char *s,int r,int g,int b,int a) {
  (void)e;(void)r;(void)g;(void)b;(void)a;return (TxtLinha){0,(int)strlen(s)*12,28};
}
TxtLinha txt_linha_corta(TxtEstilo e,const char *s,int r,int g,int b,int a,float w) {
  assert(w>0);TxtLinha t=txt_linha(e,s,r,g,b,a);if(t.w>w)t.w=(int)w;return t;
}
void txt_desenhar_alpha(TxtLinha t,float x,float y,float a) { if(a>0)bounds((GfxRect){x,y,t.w,t.h}); }
float txt_tracking(TxtEstilo e,const char *s,int r,int g,int b,float x,float y,float tracking,float a) {
  (void)tracking;TxtLinha t=txt_linha(e,s,r,g,b,255);if(x>=0)txt_desenhar_alpha(t,x,y,a);return t.w;
}
float plrui_dicas(const char *const *k,const char *const *r,int n,float x,float y,int dir,float a) {
  (void)k;(void)r;(void)n;(void)x;(void)y;(void)dir;(void)a;return 0;
}

static void abrir(void) {
  mini=1;miniGuia=0;comVideo=1;aberto=0;retido=0;janAtiva=0;aspPendente=0;canalSessao=1;
  aspecto=PLR_ASP_ORIGINAL;encolhe=1;permitido=1;nTargets=0;layers=0;
  snprintf(itemCanal.titulo,sizeof itemCanal.titulo,"A very long channel and programme name which must be clipped");
  player_mini_toque_guarda(pode);
}
int main(void) {
  const float screens[][2]={{1080,1920},{1080,2340},{2340,1080},{2520,1080}};
  for(int s=0;s<4;s++) for(int aspect=0;aspect<2;aspect++) {
    nv_layout_w=screens[s][0];nv_layout_h=screens[s][1];vw=aspect?1440:1920;vh=1080;
    abrir();player_mini_desenhar(5000);assert(nTargets==2&&!layers);
    PonteiroAlvo restore=targets[0],close=targets[1];
    assert(restore.ativar==miniToqueRestaurar&&close.ativar==miniToqueFechar);
    assert(close.x>=restore.x&&close.y>=restore.y&&close.x+close.w<=restore.x+restore.w&&close.y+close.h<=restore.y+restore.h);
    assert(close.w==56&&close.h==56&&!close.focar&&!restore.focar);
    /* A modal may open after the frame; saved callbacks must recheck it. */
    permitido=0;int before=stops,oc=orientationCalls;
    restore.ativar(0,0);close.ativar(0,0);assert(mini&&!aberto&&stops==before&&orientationCalls==oc);
    nTargets=0;draws=0;player_mini_desenhar(5000);assert(!nTargets&&!layers&&!draws);
    int win=windows;
    nv_layout_w=screens[(s+1)%4][0];nv_layout_h=screens[(s+1)%4][1];
    player_mini_desenhar(5000);assert(windows>win&&!nTargets&&!draws);
    nv_layout_w=screens[s][0];nv_layout_h=screens[s][1];
    permitido=1;restore.ativar(0,0);assert(!mini&&aberto&&orientation==1&&orientationCalls==oc+1&&stops==before);
    assert(window.w>0&&window.h>0);
    abrir();player_mini_desenhar(5000);before=stops;oc=orientationCalls;
    targets[1].ativar(0,0);assert(!mini&&!aberto&&!comVideo&&stops>before&&orientation==0&&orientationCalls==oc+1);
    close.ativar(0,0);restore.ativar(0,0);assert(orientationCalls==oc+1);
    abrir();miniGuia=1;miniGuiaCaixa=(PlrRect){40,100,400,225};nTargets=0;player_mini_desenhar(5000);assert(!nTargets&&!layers);
  }
  vw=1920;vh=1080;nv_layout_w=1920;nv_layout_h=1080;abrir();player_mini_desenhar(5000);assert(!nTargets&&!layers);
  nv_layout_w=1728;nv_layout_h=1080;abrir();player_mini_desenhar(5000);assert(!nTargets&&!layers);
  nv_layout_w=1080;nv_layout_h=2340;abrir();player_mini_toque_guarda(NULL);player_mini_desenhar(5000);assert(!nTargets&&!layers);
  puts("player_mini_toque: real drawing/callbacks, current guard, aspect, restore/close orientation, Guide/tablet/TV PASS");
  return 0;
}
