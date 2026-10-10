/* Actual inline drawing targets cross the real pointer DOWN/redraw/UP route.
 * SDL host, font and account boundaries remain inert. The delivered key pair
 * is observed here; model writes are covered by separate context fixtures. */
#define main reviewStyleFixtureMain
#define ponteiro_ativo reviewCaptureActive
#define ponteiro_alvo reviewCaptureTarget
#define ponteiro_alvo_faixa reviewCaptureRange
#define ponteiro_rolagem reviewCaptureScroll
#include "ctxmenu_retrato.c"
#undef main
#undef ponteiro_ativo
#undef ponteiro_alvo
#undef ponteiro_alvo_faixa
#undef ponteiro_rolagem
#include "ponteiro_sdl.h"
float gfx_opacidade_grupo = 1;
#undef NV_TELA_W
#undef NV_TELA_H
#define NV_TELA_W nv_layout_w
#define NV_TELA_H nv_layout_h
#define visivel reviewPointerVisible
#define lista reviewPointerList
#include "../src/ponteiro.c"
#undef lista
#undef visivel

int cat_n(void) { return 0; }
int cat_indice_por_imdb(const char *id) { (void)id; return -1; }
const CatItem *cat_item(int i) { (void)i; return NULL; }
int cat_historico_estado_id(const char *id,const char *tipo) { (void)id;(void)tipo; return 0; }
int salvos_tem(const char *id) { (void)id; return 0; }
int cat_imdb_na_lista(const char *id) { (void)id; return 0; }
int simkl_ativo(void) { return 0; }
int simkl_na_plantowatch(const char *id) { (void)id; return 0; }
float ctxinfo_compacto(const CatItem *ci,const CtxInfoEstado *state,float x,float y,float w,float a,int draw) {
  (void)ci;(void)state;(void)a;
  if (draw) desenho((GfxRect){x,y,w,200});
  return 200;
}
int logotitulo_desenhar(const CatItem *ci,const char *s,TxtEstilo e,float x,float y,float maxw,float maxh,float fallback,float a) {
  (void)ci;(void)s;(void)e;(void)maxw;(void)maxh;(void)a;
  desenho((GfxRect){x,y,fallback,52}); return 1;
}

static Uint32 reviewNow = 5000;
static int reviewDown, reviewUp, reviewExpected;
static Uint32 reviewClock(void) { return reviewNow; }
static void reviewDeliver(const SDL_Event *e) {
  assert(e->key.keysym.sym == SDLK_RETURN && foco == reviewExpected);
  if (e->type == SDL_KEYDOWN) reviewDown++;
  else { assert(e->type == SDL_KEYUP); reviewUp++; }
}
static void reviewFrame(int clipped) {
  escala = 1; ponteiro_quadro(reviewNow);
  escala = ui; nAlvos = 0;
  float w = fminf(724, nv_layout_w / ui - 100), x = (nv_layout_w / ui - w) * .5f;
  GfxRect clip = {x, 220 + 18 + 150 + 14 + 200 + 14, w, clipped ? 17 : 50};
  ctx_inline_recorte(clip);
  ctx_inline_desenhar(x,220,w,150,1);
  for (int i = 0; i < nAlvos; i++) {
    PonteiroAlvo t = alvos[i];
    ponteiro_alvo(t.x,t.y,t.w,t.h,t.focar,t.ativar,t.a,t.b);
  }
  escala = 1; ponteiro_desenhar(); reviewNow += 16;
}
static PonteiroAlvo reviewTarget(int action) {
  const PonteiroAlvo *v; int n = ponteiro_teste_lista(&v);
  for (int i = 0; i < n; i++) if (v[i].focar == ponteiroCtxOpcao && v[i].a == action) return v[i];
  assert(!"missing production inline target"); return (PonteiroAlvo){0};
}
static void reviewEvent(Uint32 type,PonteiroAlvo t,int mouse,float dy) {
  SDL_Event e; SDL_zero(e);
  float x=t.x+t.w*.5f,y=t.y+t.h*.5f+dy;
  e.type=type;
  if (mouse) { e.button.button=SDL_BUTTON_LEFT; e.button.x=(int)x; e.button.y=(int)y; }
  else { e.tfinger.touchId=3;e.tfinger.fingerId=1;e.tfinger.x=x/nv_layout_w;e.tfinger.y=y/nv_layout_h; }
  assert(ponteiro_evento(&e,reviewDeliver));
}
static void reviewStart(int clipped) {
  ponteiro_iniciar(); ponteiro_teste_toque(1); ponteiro_teste_relogio(reviewClock);
  ponteiro_teste_janela((int)nv_layout_w,(int)nv_layout_h);
  aberto=inlineOn=doPainel=1; pagina=doSocial=doLista=soFileira=0;anim=1;
  snprintf(copiaPainel.imdb,sizeof copiaPainel.imdb,"tt-fixture");
  snprintf(copiaPainel.tipo,sizeof copiaPainel.tipo,"movie");
  nOps=3;ops[0]=(typeof(ops[0])){"Remover",OP_LISTA};
  ops[1]=(typeof(ops[0])){"Assistido",OP_ASSISTIDO};ops[2]=(typeof(ops[0])){"Categoria",OP_CATEGORIA};
  foco=-1;validar=0;reviewDown=reviewUp=0;reviewFrame(clipped);
}
int main(void) {
  const float sizes[][2]={{1080,1920},{1080,2340},{2340,1080},{2520,1080}};
  const float scales[]={1,1.2f,1.3f,1.5f};int cases=0;
  for(int s=0;s<4;s++)for(int z=0;z<4;z++) {
    nv_layout_w=sizes[s][0];nv_layout_h=sizes[s][1];ui=scales[z];
    for(int clipped=0;clipped<2;clipped++)for(int mouse=0;mouse<2;mouse++)for(int action=0;action<3;action++) {
      reviewStart(clipped);reviewExpected=action;PonteiroAlvo t=reviewTarget(action);
      reviewEvent(mouse?SDL_MOUSEBUTTONDOWN:SDL_FINGERDOWN,t,mouse,0);
      assert(foco==-1||mouse);reviewFrame(clipped);reviewFrame(clipped);
      reviewEvent(mouse?SDL_MOUSEBUTTONUP:SDL_FINGERUP,t,mouse,0);
      assert(foco==action&&reviewDown==1&&reviewUp==1);cases++;
    }
    for(int change=0;change<3;change++) {
      reviewStart(0);reviewExpected=1;PonteiroAlvo t=reviewTarget(1);
      reviewEvent(SDL_FINGERDOWN,t,0,0);
      if(change==0)aberto=0;
      else if(change==1)pagina=1;
      else {SDL_Event e;SDL_zero(e);e.type=SDL_WINDOWEVENT;e.window.event=SDL_WINDOWEVENT_SIZE_CHANGED;ponteiro_evento(&e,reviewDeliver);}
      reviewFrame(0);reviewEvent(SDL_FINGERUP,t,0,0);assert(!reviewDown&&!reviewUp);
    }
    reviewStart(0);reviewExpected=1;PonteiroAlvo t=reviewTarget(1);
    reviewEvent(SDL_FINGERDOWN,t,0,0);reviewEvent(SDL_FINGERMOTION,t,0,-50);
    reviewFrame(0);reviewEvent(SDL_FINGERUP,t,0,-50);assert(foco==-1&&!reviewDown&&!reviewUp);
  }
  printf("Saved inline real pointer: %d redraw-spanning taps, closure/page/rotation/drag cancellation PASS\n",cases);
  return 0;
}
