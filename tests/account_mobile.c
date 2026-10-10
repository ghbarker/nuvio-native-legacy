/* Focused regressions for the actual account UI helpers. Full GPU snapshots
 * and pointer dispatch are verified separately; providers never run here. */
#if defined(TESTE_PLUGINS)
#define NV_TOUCH_PREVIEW 1
#define SDL_MAIN_HANDLED 1
#include "../src/pluginsui.c"
#include <assert.h>
static int disponivel = 1, repos = 2, teclado, trocasRepo, trocasScraper, removidos;
int plugins_disponivel(void) { return disponivel; }
int plugins_n_repos(void) { return repos; }
int plugins_repo_scrapers(int i, int *ligados) { assert(i >= 0 && i < repos); if (ligados) *ligados = 2; return 3; }
int plugins_alternar_repo(int i) { assert(i >= 0 && i < repos); trocasRepo++; return 1; }
int plugins_alternar_scraper(int i, int j) { assert(i >= 0 && i < repos && j >= 0 && j < 3); trocasScraper++; return 1; }
int plugins_remover_repo(int i) { assert(i >= 0 && i < repos); removidos++; repos--; return 1; }
int plugins_ligado(void) { return 1; }
void plugins_definir_ligado(int ligado) { (void)ligado; }
int teclado_aberto(void) { return teclado; }
void teclado_evento(const SDL_Event *e) { (void)e; }
void teclado_contexto(const char *s) { (void)s; }
void teclado_abrir_com(const char *a, const char *b, int n, const char *c, const char *d) { (void)a; (void)b; (void)n; (void)c; (void)d; teclado = 1; }
const char *i18n(const char *s) { return s; }
static void key(SDL_Keycode k) { SDL_Event e = {0}; e.type = SDL_KEYDOWN; e.key.keysym.sym = k; pluginsui_evento(&e); }
int main(void) {
  nivel = foco = repo = armado = sair = 0;
  pluginsui_detalhes(-1, 0); pluginsui_detalhes(1, 0); pluginsui_detalhes(4, 0); assert(nivel == 0);
  pluginsui_detalhes(3, 0); assert(nivel == 1 && repo == 1 && foco == 0 && !trocasRepo);
  pluginsui_detalhes(2, 0); assert(repo == 1 && nivel == 1);
  pluginsui_ponteiro(2, 0); key(SDLK_RETURN); assert(trocasScraper == 1 && !trocasRepo);
  key(SDLK_AC_BACK); assert(nivel == 0 && foco == 3);
  key(SDLK_RETURN); assert(trocasRepo == 1 && nivel == 0); /* TV OK still toggles. */
  key(SDLK_RIGHT); assert(nivel == 1 && repo == 1 && foco == 0); /* TV Right still opens. */
  key(SDLK_RETURN); assert(armado && !removidos);
  key(SDLK_RETURN); assert(!armado && removidos == 1 && nivel == 0);
  teclado = 1; pluginsui_detalhes(2, 0); assert(nivel == 0);
  teclado = 0; disponivel = 0; pluginsui_detalhes(2, 0); assert(nivel == 0);
  puts("account_mobile Plugins: details, scraper, Back, TV keys, removal confirmation OK");
}
#else
#define main account_mobile_existing_main
#include "menus_toque.c"
#undef main

#if defined(TESTE_AJUSTES)
static void bounds(GfxRect r) {
  assert(r.x >= 0 && r.y >= 0 && r.w > 0 && r.h > 0);
  assert(r.x + r.w <= NV_VTELA_W + .01f && r.y + r.h <= NV_VTELA_H + .01f);
}
static void guide_drag(int id, float *offset, GfxRect r, float s) {
  int idx = gv.idx, ent = gv.ent, col = gv.col, exemplo = guiaExemplo, tela = pediuTela;
  ajToqueLimpar(); ajToqueCamada(); escalaTeste = s; *offset = 0;
  ajToqueRegistrar(id, r, offset, 500, 0, id + 2000);
  PonteiroRolagem e = {PONT_ROL_INICIO, 1, 0, 0, (r.x + 50) * s, (r.y + 50) * s};
  assert(rolarRegistrado == ajToqueRolar && rolarRegistrado(&e));
  escalaTeste = 1; e.fase = PONT_ROL_MOVER; e.delta = -13.75f * s;
  assert(rolarRegistrado(&e)); perto(*offset, 13.75f);
  e.fase = PONT_ROL_SOLTAR; rolarRegistrado(&e);
  e.fase = PONT_ROL_INERCIA; e.delta = -1e6f; assert(rolarRegistrado(&e)); perto(*offset, 500);
  assert(!rolarRegistrado(&e));
  e.fase = PONT_ROL_FIM; rolarRegistrado(&e);
  assert(gv.idx == idx && gv.ent == ent && gv.col == col && guiaExemplo == exemplo && pediuTela == tela);
}
int main(void) {
  const float telas[][2] = {{1080, 2340}, {1080, 1920}, {2340, 1080}, {2520, 1080}};
  guiaAberto = 1; guiaExemplo = 0; gv.idx = GI_CAP0; gv.col = GC_LISTA; gv.ent = guiaPrimeira(gv.idx);
  for (int t = 0; t < 4; t++) for (int tamanho = 0; tamanho < 3; tamanho++) {
    nv_layout_w = telas[t][0]; nv_layout_h = telas[t][1]; valor[AJ_TAMANHO_AJUSTES] = tamanho;
    assert(guiaTelefone()); bounds(guiaIdxR()); bounds(guiaListaR()); bounds(guiaInspR());
    guiaRol = guiaRolAlvo = 0; guiaRolar(); assert(guiaRolAlvo >= 0);
    for (int n = 0; n <= 2; n++) for (int linhas = 1; linhas <= 3; linhas++) {
      ApoioTelefone a = apoioTelefoneMedir(n, linhas * 28.5f);
      bounds(a.painel); assert(a.lado > 0 && a.lado <= 300 && a.cols >= 1);
      assert(a.painel.x * ajustes_tamanho_ajustes() >= 48 - .01f);
      float qrY = a.painel.y + 84 + linhas * 28.5f + 34;
      float total = a.cols * a.lado + (a.cols - 1) * a.gap;
      float qrX = a.painel.x + (a.painel.w - total) * .5f;
      for (int i = 0; i < n; i++) {
        float x = qrX + (i % a.cols) * (a.lado + a.gap);
        float y = qrY + (i / a.cols) * (a.lado + 22 + 64 + 12 + 30 + a.gap);
        bounds((GfxRect){x, y, a.lado, a.lado});
        assert(y + a.lado + 22 + 64 + 12 + 30 <= a.painel.y + a.painel.h - a.pad + .01f);
      }
    }
    guide_drag(AJT_INDICE, &guiaIdxRol, guiaIdxR(), ajustes_tamanho_ajustes());
    guide_drag(AJT_LISTA, &guiaRol, guiaListaR(), ajustes_tamanho_ajustes());
    guide_drag(AJT_EDITOR, &guiaInspRol, guiaInspR(), ajustes_tamanho_ajustes());
  }
  /* A tapped resource is selected by identity; stale/other chapter targets fail. */
  gv.idx = GI_CAP0 + 2; gv.col = GC_LISTA; gv.ent = guiaPrimeira(gv.idx);
  int e = gv.ent; guiaPontEntAbrir(e, 0); assert(gv.ent == e && gv.col == GC_BOTOES);
  guiaPontEnt(-1, 0); guiaPontEnt(GUIA_NENT, 0); assert(gv.ent == e);
  guiaPontEnt(guiaPrimeira(GI_CAP0), 0); assert(gv.ent == e);
  nv_layout_w = 1728; nv_layout_h = 1080; valor[AJ_TAMANHO_AJUSTES] = 2;
  assert(!guiaTelefone()); perto(guiaIdxR().w, GU_IDX_W);
  nv_layout_w = 1080; nv_layout_h = 1728; assert(!guiaTelefone());
  nv_layout_h = 2340; guiaDesenhandoMini = 1; assert(!guiaTelefone());
  puts("account_mobile Settings: phone pages, two QR providers, gutters, fractional scroll and tablet/mini gates OK");
}
#elif defined(TESTE_PERFILSEL)
static float pinMax;
const char *i18n(const char *s) { return s; }
TxtLinha txt_linha_corta(TxtEstilo estilo, const char *s, int r, int g, int b, int a, float max) {
  (void)r; (void)g; (void)b; (void)a; assert(estilo == TXT_TITULO3);
  pinMax = max; int w = (int)strlen(s) * 48;
  return (TxtLinha){.w = w > max ? (int)max : w, .h = 57};
}
int main(void) {
  const float telas[][2] = {{1080, 2340}, {1080, 1920}, {2340, 1080}, {2520, 1080}};
  char nome[64]; memset(nome, 'W', 63); nome[63] = 0;
  for (int t = 0; t < 4; t++) {
    nv_layout_w = telas[t][0]; nv_layout_h = telas[t][1];
    pinDe = 1; pinFoco = PS_PIN_INICIO; snprintf(pin, sizeof pin, "123");
    TxtLinha l = pinTitulo(nome); float x = (NV_TELA_W - l.w) * .5f;
    perto(pinMax, NV_TELA_W - 96); assert(x >= 48 && x + l.w <= NV_TELA_W - 48);
    assert(pinDe == 1 && pinFoco == PS_PIN_INICIO && !strcmp(pin, "123"));
    l = pinTitulo(""); assert(l.w <= NV_TELA_W - 96);
  }
  puts("account_mobile PIN: valid 63 byte profile title stays inside gutters; PIN state preserved");
}
#else
#error choose TESTE_AJUSTES, TESTE_PERFILSEL or TESTE_PLUGINS
#endif
#endif
