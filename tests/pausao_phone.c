/* Full production pause-overlay draw, progress bar and time formatter.
 * Font/GL/catalog boundaries are doubles, not framebuffer or device coverage.
 * Times, progress and hit targets have strict viewport bounds. Other body text
 * is observed separately: its existing fixed 900px width exceeds phone portrait
 * bounds, outside this test's elapsed/total anchor regression.
 */
#define NV_TOUCH_PREVIEW 1
#define SDL_MAIN_HANDLED 1
#ifdef _WIN32
#include <time.h>
static struct tm *pausao_localtime_r(const time_t *t, struct tm *out) {
  struct tm *p = localtime(t); if (p) *out = *p; return p ? out : NULL;
}
#define localtime_r pausao_localtime_r
#endif
#include "../src/pausao.c"
#include "../src/plrui.c"
#include <assert.h>

float nv_layout_w = 1080, nv_layout_h = 2340;
float gfx_tex_aspect_atual;
static float escala = 1, zoom = 1;
static int vidroTeste, logoTeste, ligado = 1;
static CatItem itemTeste;
static char linhaEpTeste[220];
static struct { char texto[256]; TxtEstilo estilo; } linhasTeste[128];
static int nLinhasTeste, nTempos, nBarras, nAlvos, nCorposFora, nBlocos;
static GfxRect temposTeste[2], barraTeste;
static char temposTexto[2][32];
static float telaW(void) { return nv_layout_w / escala; }
static float telaH(void) { return nv_layout_h / escala; }
static int dentro(GfxRect r) {
  assert(isfinite(r.x) && isfinite(r.y) && isfinite(r.w) && isfinite(r.h));
  assert(r.w >= 0 && r.h >= 0);
  return r.x >= -.01f && r.y >= -.01f &&
    r.x + r.w <= telaW() + .01f && r.y + r.h <= telaH() + .01f;
}
static void verificar(GfxRect r) {
  if (!dentro(r)) {
    fprintf(stderr, "outside %.1fx%.1f: %.1f %.1f %.1f %.1f\n",
            telaW(), telaH(), r.x, r.y, r.w, r.h);
    assert(0);
  }
}
float gfx_escala(void) { return escala; }
float gfx_escala_ui(void) { return zoom; }
float gfx_escala_entrar(void) { float antiga = escala; escala = zoom; return antiga; }
void gfx_escala_sair(float s) { escala = s; }
void gfx_cor(GfxRect r, float raio, float cr, float cg, float cb, float a) {
  (void)raio; (void)cr; (void)cg; (void)cb;
  if (a <= 0) return;
  verificar(r);
  if (cr == 1 && cg == 1 && cb == 1 && fabsf(r.y - PAUSAO_BARRA_Y) < .01f && fabsf(r.h - 6) < .01f &&
      fabsf(r.w - (telaW() - 192)) < .01f) { barraTeste = r; nBarras++; }
}
void gfx_rect(GfxRect r, GLuint t, GfxModo m, float f, float px, float py,
              float raio, float cr, float cg, float cb, float a) {
  (void)t; (void)f; (void)px; (void)py;
  if (m != GFX_SOMBRA) gfx_cor(r, raio, cr, cg, cb, a);
}
void gfx_icone(GfxRect r, const char *s, float cr, float cg, float cb, float a) {
  (void)s; gfx_cor(r, 0, cr, cg, cb, a);
}
void gfx_veu_css(GfxRect r, int b, float c, float f, float a) {
  (void)b; (void)c; (void)f; if (a > 0) verificar(r);
}
void gfx_luz_canto(GfxRect r, float raio, float x, float y, float alcance,
                   float cr, float cg, float cb, float a) {
  (void)x; (void)y; (void)alcance; gfx_cor(r, raio, cr, cg, cb, a);
}
void gfx_sombra_sob(GfxRect s, float f, float px, float raio, float cr, float cg,
                    float cb, float a, GfxRect painel, float rp, float ap) {
  (void)s; (void)f; (void)px; (void)raio; (void)cr; (void)cg; (void)cb;
  (void)a; (void)rp; if (ap > 0) verificar(painel);
}
const char *i18n(const char *s) { return s; }
int ajustes_pausa_overlay(void) { return ligado; }
int ajustes_vidro(void) { return vidroTeste; }
int ajustes_idioma(void) { return 0; }
void ajustes_acento(float *r, float *g, float *b) { *r = .4f; *g = .7f; *b = 1; }
const CatItem *cat_item(int i) { return i == 0 ? &itemTeste : NULL; }
int cat_indice_vivo(int i, const char *id) {
  return i == 0 && !strcmp(id, itemTeste.imdb) ? 0 : -1;
}
const char *artehero_logo_sessao(const CatItem *c) { (void)c; return logoTeste ? "fixture-logo" : NULL; }
GLuint tex_obter_larg_qualquer(const char *s, float w) { (void)s; (void)w; return logoTeste ? 1 : 0; }
GLuint tex_obter_larg(const char *s, float w) { (void)s; (void)w; return 0; }
float tex_aspecto(const char *s) { (void)s; return 3.5f; }
int tex_marca_escura(const char *s) { (void)s; return 0; }
void plrilha_pedir(const PlrIlhaPedido *p) {
  assert(!strcmp(p->icone, "pl_pause-f") && !strcmp(p->texto, "Pausado"));
}
void ponteiro_alvo(float x, float y, float w, float h, PonteiroFn focar,
                  PonteiroFn ativar, int a, int b) {
  (void)focar; (void)ativar; (void)a; (void)b;
  verificar((GfxRect){ x, y, w, h }); nAlvos++;
}
int txt_largura(TxtEstilo e, const char *s) {
  /* 18 per time character exceeds bundled 24px bold metrics at UI100..150. */
  return (int)strlen(s) * (e == TXT_ILHA_NOME ? 18 : 12);
}
TxtLinha txt_linha(TxtEstilo e, const char *s, int r, int g, int b, int a) {
  (void)r; (void)g; (void)b; (void)a;
  assert(nLinhasTeste < (int)(sizeof linhasTeste / sizeof *linhasTeste));
  assert(strlen(s) < sizeof linhasTeste[0].texto);
  snprintf(linhasTeste[nLinhasTeste].texto, sizeof linhasTeste[0].texto, "%s", s);
  linhasTeste[nLinhasTeste].estilo = e;
  return (TxtLinha){ (GLuint)++nLinhasTeste, txt_largura(e, s), e == TXT_TITULO2 ? 57 : e == TXT_MINI ? 18 : 30, 0, 0 };
}
TxtLinha txt_linha_corta(TxtEstilo e, const char *s, int r, int g, int b, int a, float w) {
  assert(w > 0); TxtLinha t = txt_linha(e, s, r, g, b, a);
  if (t.w > w) t.w = (int)w;
  return t;
}
void txt_desenhar_alpha(TxtLinha t, float x, float y, float a) {
  if (a <= 0) return;
  GfxRect r = { x, y, (float)t.w, (float)t.h };
  assert(t.tex > 0 && t.tex <= (GLuint)nLinhasTeste);
  if (linhasTeste[t.tex - 1].estilo == TXT_ILHA_NOME) {
    assert(nTempos < 2); verificar(r); temposTeste[nTempos] = r;
    assert(strlen(linhasTeste[t.tex - 1].texto) < sizeof temposTexto[0]);
    snprintf(temposTexto[nTempos++], sizeof temposTexto[0], "%s", linhasTeste[t.tex - 1].texto);
  } else if (!dentro(r)) nCorposFora++;
}
float txt_tracking(TxtEstilo e, const char *s, int r, int g, int b,
                    float x, float y, float a, float tracking) {
  (void)tracking; TxtLinha t = txt_linha(e, s, r, g, b, 255);
  if (a > 0 && x >= 0) txt_desenhar_alpha(t, x, y, a);
  return t.w;
}
float txt_bloco_corta(TxtEstilo e, const char *s, int r, int g, int b, float x,
                      float y, float w, float lead, float a, int max) {
  (void)r; (void)g; (void)b; assert(w > 0);
  int n = (int)ceilf(txt_largura(e, s) / w); if (max > 0 && n > max) n = max;
  if (a > 0) { nBlocos++; if (!dentro((GfxRect){ x, y, w, n * lead })) nCorposFora++; }
  return n * lead;
}
static void preencher(char *dst, size_t n, const char *trecho) {
  size_t w = 0, len = strlen(trecho); assert(n > 0 && len > 0);
  while (w < n - 1) { size_t q = n - 1 - w; if (q > len) q = len; memcpy(dst + w, trecho, q); w += q; }
  dst[w] = 0;
}
static void zerarDesenho(void) {
  nLinhasTeste = nTempos = nBarras = nAlvos = nBlocos = nCorposFora = 0;
}
static void rodar(float w, float h, float z) {
  static const PausaoCena cenas[] = {
    { 0, 59, 0, 0, 0 }, { 3599, 7201, 0, 0, 0 },
    { 359000000, 360000000, 0, 0, 0 }, { 2147483520.0f, 2147483520.0f, 0, 0, 0 }
  };
  nv_layout_w = w; nv_layout_h = h; zoom = z;
  assert(escala == 1); pausao_fechar();
  pausao_atualizar(1.0f / 60, 100, 1, 0, itemTeste.imdb, linhaEpTeste);
  pausao_atualizar(1.0f / 60, 5099, 1, 0, itemTeste.imdb, linhaEpTeste);
  assert(!pausao_visivel()); zerarDesenho(); pausao_desenhar(5099, &cenas[0]);
  assert(nTempos == 0 && nBarras == 0 && nAlvos == 0);
  for (int f = 0; f < 120; f++) pausao_atualizar(1.0f / 60, 5100 + f, 1, 0, itemTeste.imdb, linhaEpTeste);
  assert(pausao_visivel());
  for (vidroTeste = 0; vidroTeste <= 1; vidroTeste++) for (logoTeste = 0; logoTeste <= 1; logoTeste++) {
    for (size_t i = 0; i < sizeof cenas / sizeof *cenas; i++) {
      char atual[24], dur[24], total[32];
      plrui_tempo(atual, sizeof atual, cenas[i].pos); plrui_tempo(dur, sizeof dur, cenas[i].dur);
      snprintf(total, sizeof total, "/ %s", dur);
      zerarDesenho(); pausao_desenhar(6000, &cenas[i]); assert(escala == 1);
      assert(nTempos == 2 && nBarras == 1 && nAlvos == 1 && nBlocos == 1);
      assert(!strcmp(temposTexto[0], total) && !strcmp(temposTexto[1], atual));
      float esperado = telefoneui_ativo() ? h / z - 142 : 938;
      assert(fabsf(temposTeste[0].y - esperado) < .01f && temposTeste[1].y == temposTeste[0].y);
      assert(fabsf(barraTeste.y - (h / z - 90)) < .01f);
      assert(fabsf(temposTeste[0].x + temposTeste[0].w - (w / z - 96)) < .01f);
      assert(fabsf(temposTeste[1].x + temposTeste[1].w + 8 - temposTeste[0].x) < .01f);
    }
  }
  PausaoCena semDuracao = { 12, 0, 0, 0, 0 };
  zerarDesenho(); pausao_desenhar(6000, &semDuracao);
  assert(nTempos == 0 && nBarras == 0 && nAlvos == 1);
  printf("PASS %.0fx%.0f UI%.0f timeY=%.2f; body bounds observations=%d\n",
         w, h, z * 100, telefoneui_ativo() ? h / z - 142 : 938, nCorposFora);
  pausao_fechar();
}
int main(void) {
  snprintf(itemTeste.imdb, sizeof itemTeste.imdb, "%s", "fixture-paused-title");
  preencher(itemTeste.titulo, sizeof itemTeste.titulo, "Long title ");
  preencher(itemTeste.meta, sizeof itemTeste.meta, "2026 rating genre ");
  preencher(itemTeste.sinopse, sizeof itemTeste.sinopse, "A long pause summary with enough words to use all three available lines. ");
  preencher(linhaEpTeste, sizeof linhaEpTeste, "Season 10 episode 120 extended name ");
  itemTeste.nElenco = PAUSAO_ELENCO_MAX;
  for (int i = 0; i < itemTeste.nElenco; i++) preencher(itemTeste.elenco[i].nome, sizeof itemTeste.elenco[i].nome, "Actor with a long name ");
  const float telas[][2] = { {1080,1920}, {1080,2340}, {2160,1080}, {2340,1080} };
  const float escalas[] = { 1, 1.2f, 1.3f, 1.5f };
  for (size_t i = 0; i < sizeof telas / sizeof *telas; i++) for (size_t z = 0; z < sizeof escalas / sizeof *escalas; z++) {
    rodar(telas[i][0], telas[i][1], escalas[z]);
  }
  rodar(1920, 1080, 1); /* TV keeps the original y938. */
  rodar(1920, 1200, 1); /* 16:10 tablet keeps the original y938. */
  puts("PASS: real paused overlay, elapsed/total anchors, long durations and preserved TV/tablet anchor");
  return 0;
}
