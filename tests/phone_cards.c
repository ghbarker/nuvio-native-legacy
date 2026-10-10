/* Real phone card data, layout, scrolling and decision paths. Asset previews
 * are intentionally unavailable here; GL fixtures verify pixels/fonts. */
#ifdef _WIN32
#include <time.h>
static struct tm *card_localtime_r(const time_t *t, struct tm *out) { struct tm *r = localtime(t); if (r) *out = *r; return r ? out : NULL; }
#define localtime_r card_localtime_r
#endif
#define NV_TOUCH_UI 1
#define SDL_MAIN_HANDLED 1
#if defined(TESTE_PIP)
#include "../src/pipintro.c"
#elif defined(TESTE_RELEASE)
#include "../src/novidades_cartao.c"
#include "../src/dvtela.c" /* Real state machine retained by the scene table. */
#elif defined(TESTE_N20)
#include "../src/novidades20.c"
#else
#error choose TESTE_PIP, TESTE_RELEASE or TESTE_N20
#endif
#include <assert.h>

float nv_layout_w = 1080, nv_layout_h = 2340;
static int testMobile = 1;
int layout_modo_mobile(void) { return testMobile; }
static float escala = 1, ui = 1;
static char gravadoNome[100], gravadoTexto[16];
static int escritas, videoFechado, longos;
static PonteiroRolagemFn rolar;
static int alvos;
const char *i18n(const char *s) {
  if (longos && !strcmp(s, "Canal em miniatura")) return "Um titulo muito longo para conferir que o telefone conserva a leitura sem sair das margens";
  return s;
}
float txt_bloco_corta(TxtEstilo e, const char *s, int r, int g, int b,
                      float x, float y, float w, float lead, float a, int max) {
  (void)e; (void)r; (void)g; (void)b; (void)x; (void)y; (void)a; s = i18n(s);
  assert(w > 0); int n = (int)ceilf(strlen(s) * 16.0f / w);
  if (max > 0 && n > max) n = max;
  return n * lead;
}
float gfx_escala(void) { return escala; }
float gfx_escala_ui(void) { return ui; }
float gfx_escala_entrar(void) { float antes = escala; escala = ui; return antes; }
void gfx_escala_sair(float s) { escala = s; }
void gfx_cor(GfxRect r, float raio, float cr, float cg, float cb, float ca) { (void)r; (void)raio; (void)cr; (void)cg; (void)cb; (void)ca; }
void gfx_rect(GfxRect r, GLuint tex, GfxModo modo, float foco, float px, float py, float raio, float cr, float cg, float cb, float ca) {
  (void)tex; (void)modo; (void)foco; (void)px; (void)py; gfx_cor(r, raio, cr, cg, cb, ca);
}
void gfx_anel(GfxRect r, float raio, float esp, float cr, float cg, float cb, float ca) { (void)esp; gfx_cor(r, raio, cr, cg, cb, ca); }
void gfx_icone(GfxRect r, const char *nome, float cr, float cg, float cb, float ca) { (void)nome; gfx_cor(r, 0, cr, cg, cb, ca); }
void gfx_recorte(float x, float y, float w, float h) { (void)x; (void)y; assert(w > 0 && h > 0); }
void gfx_sem_recorte(void) {}
void ponteiro_camada(void) { alvos = 0; }
void ponteiro_alvo(float x, float y, float w, float h, PonteiroFn f, PonteiroFn a, int i, int b) {
  (void)x; (void)y; (void)w; (void)h; (void)f; (void)a; (void)i; (void)b; alvos++;
}
void ponteiro_rolagem(PonteiroRolagemFn fn) { rolar = fn; }
int ajustes_animacoes_reduzidas(void) { return 1; }
char *dados_ler(const char *s) { return !strcmp(s, gravadoNome) ? strdup(gravadoTexto) : NULL; }
int dados_gravar(const char *s, const char *t) { snprintf(gravadoNome, sizeof gravadoNome, "%s", s); snprintf(gravadoTexto, sizeof gravadoTexto, "%s", t); escritas++; return 1; }
GLuint tex_obter_hero(const char *s) { (void)s; return 0; }
#if defined(TESTE_RELEASE) || defined(TESTE_N20)
int gfx_mini_alvo(GfxMini *m, int w, int h) { (void)m; (void)w; (void)h; return 0; }
void gfx_mini_comecar(GfxMini *m, float x, float y, float s) { (void)m; (void)x; (void)y; (void)s; assert(!"GPU preview unexpectedly executed"); }
void gfx_mini_terminar(void) { assert(!"GPU preview unexpectedly executed"); }
void gfx_mini_desenhar(const GfxMini *m, GfxRect r, float raio, float a) { (void)m; (void)r; (void)raio; (void)a; }
#endif
#if defined(TESTE_PIP)
void player_fechar_mini(void) { videoFechado++; }
#elif defined(TESTE_RELEASE)
int apoio_n(void) { return 2; }
int apoio_qual(int i) { return i; }
const char *apoio_nome(int i) { (void)i; return ""; }
const char *apoio_url_curta(int i) { (void)i; return ""; }
int apoio_qr(int i, float x, float y, float lado, float a) { (void)i; (void)x; (void)y; (void)lado; (void)a; assert(!"GPU preview unexpectedly executed"); return 0; }
/* Scene function pointers remain in the production content table. The CPU
 * measurement path must never execute their GPU-only preview functions. */
float dvtela_previa_largura(void) { assert(!"GPU preview unexpectedly executed"); return 0; }
float dvtela_previa_altura(void) { assert(!"GPU preview unexpectedly executed"); return 0; }
void dvtela_previa_zerar(void) { assert(!"GPU preview unexpectedly executed"); }
void dvtela_previa_avancar(const DvtelaEstado *e, float dt) { (void)e; (void)dt; assert(!"GPU preview unexpectedly executed"); }
void dvtela_previa_desenhar(float x, float y, float s, const DvtelaEstado *e, Uint32 agora, float a) {
  (void)x; (void)y; (void)s; (void)e; (void)agora; (void)a; assert(!"GPU preview unexpectedly executed");
}
void ajustes_acento(float *r, float *g, float *b) { *r = .5f; *g = .6f; *b = .8f; }
int ajustes_relogio_12h(void) { return 0; }
void plrui_linha_foco(GfxRect r, float raio, float a) { gfx_cor(r, raio, 1, 1, 1, a); }
TxtLinha txt_linha(TxtEstilo e, const char *s, int r, int g, int b, int a) { (void)e; (void)r; (void)g; (void)b; (void)a; return (TxtLinha){.w = (int)strlen(s) * 16, .h = 30}; }
TxtLinha txt_linha_corta(TxtEstilo e, const char *s, int r, int g, int b, int a, float w) { TxtLinha l = txt_linha(e, s, r, g, b, a); if (l.w > w) l.w = (int)w; return l; }
void txt_desenhar_alpha(TxtLinha l, float x, float y, float a) { (void)l; (void)x; (void)y; (void)a; }
#elif defined(TESTE_N20)
PtvPlataforma ptv_plataforma(void) { return PTV_ANDROID; }
void ajustes_acento(float *r, float *g, float *b) { *r = .5f; *g = .6f; *b = .8f; }
int ajustes_vidro(void) { return 0; }
int ajustes_tinta_foco(void) { return 0; }
void ponteiro_alvo_faixa(float x, float y, float w, float h, float topo, float fim, PonteiroFn f, PonteiroFn a, int i, int b) {
  float cima = fmaxf(y, topo), baixo = fminf(y + h, fim);
  if (baixo > cima) ponteiro_alvo(x, cima, w, baixo - cima, f, a, i, b);
}
void ilha_avisar(const char *chave, int tipo, const char *icone, const char *titulo, unsigned duracao, int som) {
  (void)chave; (void)tipo; (void)icone; (void)titulo; (void)duracao; (void)som;
}
#endif
static void perto(float a, float b) { assert(fabsf(a - b) < .01f); }
static void limites(GfxRect r, float w, float h) {
  assert(r.x >= 0 && r.y >= 0 && r.w > 0 && r.h > 0);
  assert(r.x + r.w <= w + .01f && r.y + r.h <= h + .01f);
}
static void layout_e_arrasto(TelefoneCartao *c, float total, int n, PonteiroRolagemFn fn, int chave) {
  float w = nv_layout_w / ui, h = nv_layout_h / ui;
  telefonecartao_medir(c, w, h, n); limites(c->painel, w, h); limites(c->corpo, w, h);
  for (int i = 0; i < n; i++) {
    GfxRect r = telefonecartao_botao_r(c, i); limites(r, w, h);
    assert(r.y >= c->corpo.y + c->corpo.h + 24 - .01f);
    if (i) assert(r.y >= telefonecartao_botao_r(c, i - 1).y + 88 - .01f);
  }
  escala = ui; telefonecartao_comecar(c, fmaxf(total, c->corpo.h + 500), chave, 1, fn, 1);
  assert(rolar == fn && alvos == 1);
  PonteiroRolagem e = {PONT_ROL_INICIO, 1, 0, 0, (c->corpo.x + 50) * ui, (c->corpo.y + 50) * ui};
  int antes = escritas; assert(fn(&e));
  escala = 1; e.fase = PONT_ROL_MOVER; e.delta = -13.25f * ui; assert(fn(&e)); perto(c->offset, 13.25f);
  e.fase = PONT_ROL_SOLTAR; fn(&e); e.fase = PONT_ROL_FIM; fn(&e);
  assert(escritas == antes); float off = c->offset;
  escala = ui; telefonecartao_comecar(c, fmaxf(total, c->corpo.h + 500), chave, 1, fn, 1); perto(c->offset, off);
  telefonecartao_comecar(c, fmaxf(total, c->corpo.h + 500), chave + 1, 1, fn, 1); perto(c->offset, 0);
}
int main(void) {
  const float telas[][2] = {{1080, 1920}, {1080, 2340}, {2340, 1080}, {2520, 1080}};
  const float zoom[] = {1, 1.2f, 1.3f, 1.5f};
#if defined(TESTE_PIP)
  pipintro_abrir(); assert(aberto && pipintro_decisao() == -1);
  longos = 0; float curto = piTelefoneConteudo(0, 0, 600, 0);
  longos = 1; assert(piTelefoneConteudo(0, 0, 600, 0) > curto); /* Translate before measuring. */
  for (int t = 0; t < 4; t++) for (int s = 0; s < 4; s++) {
    nv_layout_w = telas[t][0]; nv_layout_h = telas[t][1]; ui = zoom[s]; longos = s & 1;
    assert(telefoneui_ativo());
    float total = piTelefoneConteudo(0, 0, nv_layout_w / ui - 104, 0); assert(total > 0);
    layout_e_arrasto(&piTelefone, total, 2, piTelefoneRolar, t * 100 + s * 2);
    assert(aberto && focoBtn == 0 && !videoFechado);
  }
  piTelefoneEscolher(2, 0); assert(aberto && !escritas);
  piTelefoneEscolher(0, 0); assert(!aberto && !strcmp(gravadoNome, PI_ARQ) && !strcmp(gravadoTexto, "1\n") && !videoFechado);
  pipintro_abrir(); assert(!aberto); /* Decision is retained. */
  gravadoNome[0] = 0; pipintro_abrir(); piTelefoneEscolher(1, 0);
  assert(!aberto && !strcmp(gravadoTexto, "0\n") && videoFechado == 1);
#elif defined(TESTE_RELEASE)
  novcartao_teste_plataforma(NOV_ANDROID); novcartao_abrir(); assert(nVis > 0 && nCen == 1);
  for (int t = 0; t < 4; t++) for (int s = 0; s < 4; s++) {
    nv_layout_w = telas[t][0]; nv_layout_h = telas[t][1]; ui = zoom[s]; assert(telefoneui_ativo());
    for (pagina = 0; pagina <= paginaApoio(); pagina++) {
      telefonecartao_medir(&novTelefone, nv_layout_w / ui, nv_layout_h / ui, 2);
      float total = novTelefoneConteudo(0, 0, novTelefone.corpo.w, 0); assert(total > 0);
      int f = foco, p = pagina; layout_e_arrasto(&novTelefone, total, 2, novTelefoneRolar, pagina + t * 100 + s * 10);
      assert(aberto && foco == f && pagina == p);
    }
  }
  pagina = 0; novTelefoneEscolher(1, -1); assert(pagina == 0);
  for (int i = 1; i <= paginaApoio(); i++) { novTelefoneEscolher(1, pagina); assert(aberto && pagina == i); }
  novTelefoneEscolher(0, pagina); assert(pagina == paginaApoio() - 1);
  novTelefoneEscolher(1, pagina); novTelefoneEscolher(1, pagina);
  assert(!aberto && !strcmp(gravadoNome, CT.arquivo));
#elif defined(TESTE_N20)
  novidades20_abrir(1); assert(nLista == N20_NCAP + 3 && novo);
  for (int t = 0; t < 4; t++) for (int s = 0; s < 4; s++) {
    nv_layout_w = telas[t][0]; nv_layout_h = telas[t][1]; ui = zoom[s]; assert(telefoneui_ativo());
    for (idx = 0; idx < nLista; idx++) {
      int n = tipo() == N20_FIM ? nBotoesFim() : 3;
      telefonecartao_medir(&n20Telefone, nv_layout_w / ui, nv_layout_h / ui, n);
      float total = n20TelefoneConteudo(0, 0, n20Telefone.corpo.w, 0); assert(total > 0);
      int tela = idx; layout_e_arrasto(&n20Telefone, total, n, n20TelefoneRolar, idx + t * 100 + s * 10); assert(aberto && idx == tela);
    }
  }
  idx = 0; ptHeroOk(1, 0); assert(essencial && nLista == 12 && idx == 1);
  for (int i = 0; i < nLista; i++) assert(lista[i] != 8 && lista[i] != 9);
  n20TelefoneEstado(1, tipo()); assert(est[tipo()] == 1);
  n20TelefoneEstado(9, tipo()); assert(est[tipo()] == 1);
  n20TelefoneAcao(2, 3); assert(saindo && aberto);
  float total = n20TelefoneConteudo(0, 0, n20Telefone.corpo.w, 0); assert(total > 0);
  ptSairOk(0, 0); assert(!saindo && aberto);
  ptAcao(4, 0); assert(tipo() == N20_RESUMO); ptAcao(1, 0); assert(tipo() == N20_FIM);
  ptFimOk(0, 0); assert(!aberto && novidades20_pedido() == N20_PEDIU_GUIA && !strcmp(gravadoNome, N20_ARQ));
  novidades20_abrir(0); irPara(nLista - 1); ptFimOk(1, 0); assert(aberto && idx == 0); /* Rewatch. */
  ptAcao(3, 0); ptSairOk(1, 0); assert(!aberto && !strcmp(gravadoNome, N20_ARQ));
#endif
  testMobile = 0;
  nv_layout_w = 1920; nv_layout_h = 1080; assert(!telefoneui_ativo());
  nv_layout_w = 1080; nv_layout_h = 1728; assert(!telefoneui_ativo());
  testMobile = 1;
  assert(telefoneui_ativo());
  nv_layout_w = 1920; nv_layout_h = 1080; assert(telefoneui_ativo());
  puts("phone_cards: real content, four viewports/zooms, bounds, fractional drag, page/decision gates OK");
}
