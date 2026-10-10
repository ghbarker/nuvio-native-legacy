/* Actual Support panel drawing and registered targets, without GL or providers. */
#define NV_TOUCH_PREVIEW 1
#define SDL_MAIN_HANDLED 1
#include "../src/layout.h"
#include "../src/apoio.h"
#include "../src/ponteiro.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

float nv_layout_w = 1080, nv_layout_h = 2340;
static float escala = 1, fonte = 1;
static int apoioAberto, destino, nProviders, nAlvos, nQrs;
static PonteiroAlvo alvos[8];
static GfxRect painel, qrs[2];
#define NV_VTELA_W (NV_TELA_W / escala)
#define NV_VTELA_H (NV_TELA_H / escala)
enum { AJ_APOIAR = 37 };
typedef struct { const char *k, *l; } AjDica;
#define AJ_CHIP_SOL .141f, .149f, .173f
static void dentro(GfxRect r, GfxRect b) {
  assert(r.w > 0 && r.h > 0 && r.x >= b.x - .03f && r.y >= b.y - .03f);
  assert(r.x + r.w <= b.x + b.w + .03f && r.y + r.h <= b.y + b.h + .03f);
}
static void desenhado(GfxRect r) { dentro(r, (GfxRect){0,0,NV_VTELA_W,NV_VTELA_H}); }
static void focarOpcao(int op) { destino = op; }
static int guiaVoltar(SDL_Keycode k, const SDL_Event *e) { (void)e; return k == SDLK_AC_BACK; }
static int ajTelaCheia(void) { return 1; }
float ajustes_tamanho_ajustes(void) { return escala; }
static const char *ajudaOpcao(int op) {
  assert(op == AJ_APOIAR);
  return "Apoie o desenvolvimento do projeto. Esta descricao comprida testa a altura medida e os limites do painel sem modificar os provedores ou seus enderecos.";
}
const char *i18n(const char *s) { return s; }
static int altura(TxtEstilo e) { return (int)((e == TXT_AJ_SEG ? 28 : e == TXT_AJ_SUB ? 25 : 24) * fonte); }
TxtLinha txt_linha_corta(TxtEstilo e, const char *s, int r, int g, int b, int a, float max) {
  (void)r; (void)g; (void)b; (void)a; assert(max > 0);
  int h = altura(e), w = (int)(strlen(s) * h * .65f);
  return (TxtLinha){.w = w > max ? (int)max : w, .h = h};
}
void txt_desenhar_alpha(TxtLinha t, float x, float y, float a) {
  if (a > 0 && t.w > 0) desenhado((GfxRect){x,y,t.w,t.h});
}
void txt_desenhar(TxtLinha t, float x, float y) { txt_desenhar_alpha(t,x,y,1); }
float txt_bloco_corta(TxtEstilo e, const char *s, int r, int g, int b,
                      float x, float y, float w, float leading, float a, int max) {
  (void)r; (void)g; (void)b;
  int n = (int)ceilf(strlen(s) * altura(e) * .65f / w);
  if (max > 0 && n > max) n = max;
  if (a > 0) desenhado((GfxRect){x,y,w,n * leading});
  return n * leading;
}
static TxtLinha ajTxtC(TxtEstilo e, const char *s, float max) { return txt_linha_corta(e,s,255,255,255,255,max); }
static void ajVeuModal(void) {}
static void ajIlha(GfxRect r, float raio, float a, int modal) {
  (void)raio; (void)a; (void)modal; desenhado(r); painel = r;
}
static void ajIcone(const char *s, float x, float y, float lado, float a) {
  (void)s; (void)a; desenhado((GfxRect){x,y,lado,lado});
}
static void ajNeutro(GfxRect r, float raio, float va, float cr, float cg, float cb, float a) {
  (void)raio; (void)va; (void)cr; (void)cg; (void)cb; (void)a; desenhado(r);
}
static void ajToqueCamada(void) {}
static float ajDicasLargura(const AjDica *d, int n) { (void)d; (void)n; return 0; }
static void ajKicker(const char *s, float x, float y, float a) { (void)s; (void)x; (void)y; (void)a; }
static float ajDicasLinha(const AjDica *d, int n, float x, float y, float a) { (void)d; (void)n; (void)x; (void)y; (void)a; return 0; }
static float ajBloco(TxtEstilo e, const char *s, float x, float y, float w, float leading, float a, int max) {
  return txt_bloco_corta(e,s,255,255,255,x,y,w,leading,a,max);
}
void gfx_cor(GfxRect r, float raio, float cr, float cg, float cb, float a) {
  (void)raio; (void)cr; (void)cg; (void)cb; (void)a; desenhado(r);
}
void ponteiro_camada(void) { nAlvos = 0; }
void ponteiro_alvo(float x, float y, float w, float h, PonteiroFn f, PonteiroFn ativar, int a, int b) {
  assert(nAlvos < 8); desenhado((GfxRect){x,y,w,h});
  alvos[nAlvos++] = (PonteiroAlvo){x,y,w,h,f,ativar,a,b};
}
int apoio_n(void) { return nProviders; }
int apoio_qual(int i) { assert(i >= 0 && i < nProviders); return i; }
const char *apoio_nome(int q) { return q ? "Ko-fi" : "Patreon"; }
const char *apoio_url_curta(int q) { return q ? "ko-fi.com/iqui27" : "patreon.com/cw/CraaazyDevs"; }
int apoio_qr(int q, float x, float y, float lado, float a) {
  (void)a; assert(q == nQrs && q < 2);
  GfxRect r = {x,y,lado,lado}; dentro(r,painel); desenhado(r); qrs[nQrs++] = r; return 1;
}
float apoio_rotulo(int q, float x, float y, float h, int centro, float a) {
  (void)q; (void)x; (void)y; (void)h; (void)centro; (void)a; assert(!"legacy label used by phone panel"); return 0;
}
#include "../src/ajustes_ux_apoio.inc"

/* Use the production topmost hit-test. Unused event machinery is discarded. */
#define ponteiro_alvo account_unused_alvo
#define ponteiro_camada account_unused_camada
#include "../src/ponteiro.c"
#undef ponteiro_alvo
#undef ponteiro_camada

static void tap(float x, float y) {
  int k = ponteiro_achar(alvos,nAlvos,x,y); assert(k >= 0);
  if (alvos[k].ativar) alvos[k].ativar(alvos[k].a,alvos[k].b);
}
int main(void) {
  const float telas[][2] = {{1080,2340},{1080,1920},{2340,1080},{2520,1080}};
  int desenhos = 0;
  for (int t = 0; t < 4; t++) for (int s = 0; s < 3; s++) for (int f = 0; f < 2; f++) for (int n = 0; n <= 2; n++) {
    nv_layout_w = telas[t][0]; nv_layout_h = telas[t][1];
    escala = (.8f + .1f * s) * (t < 2 ? 1.25f : 1);
    fonte = f ? 1.2f : 1; nProviders = n; nQrs = 0;
    apoioAbrir(); destino = -1; apoioDesenharTelefone(); desenhos++;
    assert(apoioAberto && nAlvos == 3 && nQrs == n);
    assert(painel.x * escala >= 48 - .03f);
    /* QR/body taps are absorbed; a tap must not fall through to Settings. */
    for (int q = 0; q < n; q++) { tap(qrs[q].x + qrs[q].w * .5f,qrs[q].y + qrs[q].h * .5f); assert(apoioAberto && destino == -1); }
    tap(alvos[2].x + 28,alvos[2].y + 28); assert(!apoioAberto && destino == AJ_APOIAR);
    apoioAbrir(); nQrs = 0; apoioDesenharTelefone(); tap(1,1); assert(!apoioAberto && destino == AJ_APOIAR);
    apoioAbrir(); SDL_Event e = {0}; e.type = SDL_KEYDOWN; e.key.keysym.sym = SDLK_AC_BACK;
    apoioEvento(&e); assert(!apoioAberto);
  }
  printf("account_support_review: %d actual phone panel draws, QR bounds, body/close/outside hit priority and Back OK\n",desenhos);
}
