/* Exercise the actual vertudo_desenhar empty-page path, without a GL context. */
#define main original_watched_catalog_main
#include "watched_catalog_ui.c"
#undef main

static int moreLabelCreated, moreLabelPainted, moreTargetRegistered;
static GfxRect lastClip, moreTarget;
static PonteiroFn moreClick;
static float currentScale = 1.0f;
float gfx_escala(void) { return currentScale; }
void gfx_escala_sair(float scale) { currentScale = scale; }
void gfx_recorte(float x, float y, float w, float h) { lastClip = (GfxRect){x,y,w,h}; }
void gfx_sem_recorte(void) { lastClip = (GfxRect){0}; }
TxtLinha txt_linha(TxtEstilo style, const char *s, int r, int g, int b, int a) {
  (void)style; (void)r; (void)g; (void)b; (void)a;
  if (!strcmp(s, "Carregar mais")) moreLabelCreated++;
  return (TxtLinha){.tex = !strcmp(s, "Carregar mais") ? 366u : 1u, .w = (int)strlen(s)*10, .h = 24};
}
TxtLinha txt_linha_corta(TxtEstilo style, const char *s, int r, int g, int b, int a, float maxW) {
  TxtLinha t = txt_linha(style,s,r,g,b,a);
  if (t.w > maxW) t.w = (int)maxW;
  return t;
}
void txt_desenhar_alpha(TxtLinha t, float x, float y, float a) {
  (void)x; (void)y;
  if (t.tex == 366u && a > 0.0f) moreLabelPainted++;
}
void ponteiro_alvo(float x,float y,float w,float h,PonteiroFn focus,PonteiroFn click,int a,int b) {
  (void)focus; (void)a; (void)b;
  if (click != carregarMaisVistos) return;
  moreTargetRegistered++;
  moreTarget = (GfxRect){x,y,w,h}; moreClick = click;
  assert(lastClip.h > 0 && y >= lastClip.y && y+h <= lastClip.y+lastClip.h);
  assert(x >= 0 && x+w <= NV_TELA_W);
}

#include "draw-boundary-stubs.inc"

static void drawScenario(void) {
  freshHistory(366); seed(6,"ttDrawHidden","movie",2020);
  for (int i=0;i<6;i++) mark(i,1);
  hideWatched = 1; loading = atEnd = catalogError = semFonte = 0;
  resetGrid(VT_LAY_GRADE); aberta = 1; anim = 1.0f; tabFocus = 0;
  moreLabelCreated = moreLabelPainted = moreTargetRegistered = 0; moreClick = NULL;
  int before = moreRequests;
  vertudo_desenhar(1000);
  assert(moreLabelCreated == 1 && moreLabelPainted == 1 && moreTargetRegistered == 1);
  assert(moreClick && moreTarget.w > 0 && moreTarget.h > 0 && moreRequests == before);
  moreClick(0,0); assert(moreRequests == before+1);
  loading = 1;
  moreLabelCreated = moreLabelPainted = moreTargetRegistered = 0;
  vertudo_desenhar(1001);
  assert(!moreLabelCreated && !moreLabelPainted && !moreTargetRegistered);
  loading = 0; atEnd = 1;
  vertudo_desenhar(1002); assert(!moreTargetRegistered);
  atEnd = 0; hideWatched = 0;
  vertudo_desenhar(1003); assert(!moreTargetRegistered);
  printf("PASS actual draw: Carregar mais paints/clips/registers/clicks once; loading/end/visible pages suppress it (%s)\n",
         focusPhone ? "Mobile portrait" : "TV");
}
int main(void) {
  focusPhone = 0;
#ifdef NV_TOUCH_UI
  nv_layout_w=1920; nv_layout_h=1080;
#endif
  drawScenario();
#ifdef NV_TOUCH_UI
  focusPhone=1; nv_layout_w=1080; nv_layout_h=1920;
  drawScenario();
#endif
  return 0;
}
