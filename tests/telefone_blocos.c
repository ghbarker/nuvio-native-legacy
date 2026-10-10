/* Production drawing and data measurement for the secondary Detail blocks.
 * No window, network, decoder, account data or disk writes. */
#define NV_TOUCH_PREVIEW 1
#define SDL_MAIN_HANDLED 1
#include "notasui.h"
#include "temporadas_grafico.h"
#include "serieaud.h"
#include "layout.h"
#include "ajustes.h"
#include "tex_cache.h"
#include "idioma.h"
#include "svdesenho.h"
#include "plrui.h"
#include <SDL2/SDL.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#if defined(TESTE_NOTAS)
#include "../src/notasui.c"
#include "../src/notasfontes.c"
#elif defined(TESTE_TEMPORADAS)
#include "../src/temporadas_grafico.c"
#elif defined(TESTE_AUDIENCIA)
#include "../src/serieaud.c"
#else
#error select a Detail block
#endif

float nv_layout_w = 1080, nv_layout_h = 2340;
float gfx_tex_aspect_atual, gfx_card_forcar_cover_atual;
int txt_pendentes, anim_politica_reduzida;
static GfxRect cards[100];
static int nCards, nTextos;
static float fimTexto;
static void rect(GfxRect r) {
  assert(isfinite(r.x) && isfinite(r.y) && isfinite(r.w) && isfinite(r.h));
  assert(r.w >= 0 && r.h >= 0);
  assert(r.x >= 0 && r.x + r.w <= nv_layout_w);
}
float gfx_escala_ui(void) { return 1; }
int ajustes_vidro(void) { return 1; }
int ajustes_idioma(void) { return 0; }
int ajustes_idioma_ingles(void) { return 1; }
int ajustes_borda_foco(void) { return 0; }
int ajustes_animacoes_reduzidas(void) { return 1; }
void ajustes_acento(float *r, float *g, float *b) { *r = .4f; *g = .7f; *b = 1; }
const char *i18n(const char *s) { return s; }
Uint32 SDL_GetTicks(void) { return 5000; }
int txt_largura(TxtEstilo e, const char *s) { (void)e; return (int)strlen(s) * 12; }
TxtLinha txt_linha(TxtEstilo e, const char *s, int r, int g, int b, int a) {
  (void)r; (void)g; (void)b; (void)a;
  return (TxtLinha){0, txt_largura(e, s), e == TXT_V2_NUM150 ? 150 : 28};
}
TxtLinha txt_linha_corta(TxtEstilo e, const char *s, int r, int g, int b, int a, float w) {
  assert(w > 0);
  TxtLinha l = txt_linha(e, s, r, g, b, a);
  if (l.w > w) l.w = (int)w;
  return l;
}
void txt_desenhar_alpha(TxtLinha l, float x, float y, float a) {
  if (a <= 0) return;
  rect((GfxRect){x, y, l.w, l.h}); nTextos++;
  if (y + l.h > fimTexto) fimTexto = y + l.h;
}
void txt_desenhar(TxtLinha l, float x, float y) { txt_desenhar_alpha(l, x, y, 1); }
float txt_bloco_corta(TxtEstilo e, const char *s, int r, int g, int b, float x, float y, float w, float lead, float a, int max) {
  assert(w > 0);
  int n = (int)ceilf(txt_largura(e, s) / w); if (n > max) n = max;
  if (a > 0 && n) txt_desenhar_alpha((TxtLinha){0, (int)w, (int)(n * lead)}, x, y, a);
  (void)r; (void)g; (void)b; return n * lead;
}
float txt_bloco(TxtEstilo e, const char *s, int r, int g, int b, float x, float y, float w, float lead, float a, int max) {
  return txt_bloco_corta(e, s, r, g, b, x, y, w, lead, a, max);
}
void gfx_cor(GfxRect r, float raio, float cr, float cg, float cb, float a) {
  (void)raio; (void)cr; (void)cg; (void)cb; if (a > 0) rect(r);
}
void gfx_rect(GfxRect r, GLuint t, GfxModo m, float f, float px, float py, float raio, float cr, float cg, float cb, float a) {
  (void)t; (void)m; (void)f; (void)px; (void)py; gfx_cor(r, raio, cr, cg, cb, a);
}
void gfx_vidro_painel(GfxRect r, float raio, float f, float a) {
  (void)raio; (void)f; if (a > 0) { rect(r); cards[nCards++] = r; }
}
void gfx_icone(GfxRect r, const char *n, float cr, float cg, float cb, float a) { (void)n; gfx_cor(r, 0, cr, cg, cb, a); }
void gfx_anel(GfxRect r, float raio, float esp, float cr, float cg, float cb, float a) { (void)esp; gfx_cor(r, raio, cr, cg, cb, a); }
void gfx_anel_fora(GfxRect r, float raio, float esp, float gap, float cr, float cg, float cb, float a) { (void)gap; gfx_anel(r, raio, esp, cr, cg, cb, a); }
GLuint tex_obter(const char *u) { (void)u; return 0; }
float tex_aspecto(const char *u) { (void)u; return 2; }
int tex_marca_escura(const char *u) { (void)u; return 0; }
const char *extras_caminho_marca_nome(const char *u) { return u; }
#if !defined(TESTE_NOTAS)
void notasui_painel(GfxRect r, float raio, float a) { gfx_vidro_painel(r, raio, 0, a); }
#endif
#if defined(TESTE_TEMPORADAS)
void plrui_trilho(GfxRect r, float pos, float cr, float cg, float cb, float a) { (void)pos; gfx_cor(r, 0, cr, cg, cb, a); }
void plrui_linha_foco(GfxRect r, float raio, float a) { gfx_cor(r, raio, 1, 1, 1, a); }
void svd_avatar(GfxRect r, const char *u, const char *n, const char *id, float a) { (void)u; (void)n; (void)id; gfx_cor(r, 0, 1, 1, 1, a); }
void svd_ponto_vivo(float x, float y, float d, float halo, float a, Uint32 t) { (void)halo; (void)t; gfx_cor((GfxRect){x - d/2,y-d/2,d,d}, 0, 1, 1, 1, a); }
void amigostitulo_primeiro_nome(const char *s, char *dst, size_t cap) { snprintf(dst, cap, "%.15s", s); }
#elif defined(TESTE_AUDIENCIA)
void notasui_cor_rampa(int d, float *r, float *g, float *b) { (void)d; *r = *g = *b = .5f; }
void notasui_mapa_card(const NotasSecao *s, GfxRect r, int t, int i, float a) { (void)s; (void)t; (void)i; notasui_painel(r, 0, a); }
#endif
static void perto(float a, float b) { assert(fabsf(a - b) < .01f); }
static void rodar(float W, float H) {
  nv_layout_w = W; nv_layout_h = H; nCards = nTextos = 0; fimTexto = 0;
#if defined(TESTE_NOTAS)
  gateFontes.pronto = 1; gateFontes.inicio = 1;
  NotasSecao s = {0}; for (int i = 0; i < EX_NFONTES; i++) s.cru[i] = 70;
  float h = notasui_fontes_altura(&s), desenhado = notasui_fontes_desenhar(&s, 96, 80, 1, NULL);
  perto(h, desenhado); assert(nCards == EX_NFONTES + 1 && nTextos > 10);
  if (H > W) { assert(cards[0].w == W - 192); assert(cards[1].y >= cards[0].y + cards[0].h + 28); }
  else { assert(cards[0].w == 520); assert(cards[1].x > cards[0].x + cards[0].w); }
  for (int i = 0; i < nCards; i++) assert(cards[i].y + cards[i].h <= 80 + h + .01f);
#elif defined(TESTE_TEMPORADAS)
  TgDados d = {0}; d.n = 24; d.sabe = 1; d.vistos = 3; d.exibidos = 240; d.nAmg = 3;
  strcpy(d.imdb, "tt-test");
  for (int i = 0; i < d.n; i++) { d.t[i].numero = i + 1; d.t[i].exibidos = d.t[i].total = 10; }
  for (int i = 0; i < d.nAmg; i++) { snprintf(d.amg[i].nome, sizeof d.amg[i].nome, "A deliberately long friend name %d", i); d.amg[i].temporada = 20; d.amg[i].episodio = 10; }
  float h = tgraf_altura_dados(&d); tgraf_desenhar(&d, 96, 80, W - 192, -1, 1, 1, 5000);
  assert(nTextos > 50);
  if (H > W) {
    assert(nCards == 3 && cards[1].x == cards[0].x && cards[2].x == cards[0].x);
    perto(cards[1].h, 52 + 24 * 46); perto(cards[2].y + cards[2].h, 80 + h);
    d.n = 4; d.nAmg = 0; assert(tgraf_altura_dados(&d) < h - 1000);
  } else { assert(nCards == 2); perto(h, 378); }
#elif defined(TESTE_AUDIENCIA)
  gateBloco.pronto = 1; gateBloco.inicio = 1;
  SaBloco b = {.temporada = 1, .tempIdx = -1, .sel = -1, .aberto = 0};
  float h = serieaud_bloco_altura(); perto(serieaud_bloco(96, 80, &b, 1), h);
  assert(nCards == 3 && nTextos > 0);
  if (H > W) { assert(cards[0].x == cards[1].x && cards[1].x == cards[2].x); perto(cards[2].y + cards[2].h, 80 + h); }
  else { assert(cards[2].x > cards[0].x + cards[0].w); perto(h, 904); }
#endif
}
int main(void) {
  rodar(1080, 1920); rodar(1080, 2340); rodar(2340, 1080); rodar(1920, 1080);
  puts("Detail phone blocks: PASS"); return 0;
}
