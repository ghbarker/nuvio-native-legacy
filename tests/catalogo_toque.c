// Exercita offsets das telas reais, sem janela, rede ou desenho.
#define NV_TOUCH_PREVIEW 1
#if defined(TESTE_HOME)
#include "../src/home.c"
#elif defined(TESTE_VERTUDO)
#include "../src/vertudo.c"
#elif defined(TESTE_BIBLIOTECA)
#include "../src/biblioteca.c"
#elif defined(TESTE_BUSCA)
#include "../src/busca.c"
#else
#error escolha uma tela TESTE_*
#endif
#include "../src/ctxlista.h"
#include <assert.h>

float nv_layout_w = 2400.0f;
int ajustes_home_layout(void) { return HOME_LAYOUT_MODERNA; }
int ajustes_hero_ligado(void) { return 1; }
float ajustes_conteudo_x(void) { return 104.0f; }
int ajustes_largura_poster_dp(void) { return 126; }
int ajustes_posteres_deitados(void) { return 0; }
int ajustes_rotulos_poster(void) { return 1; }
int ajustes_borda_foco(void) { return 1; }
float ajustes_espaco_titulos(void) { return 1.0f; }
float ajustes_espaco_fileiras(void) { return 1.0f; }
float ajustes_rail_largura_fixa(void) { return 0.0f; }
void ajustes_area_conteudo(float esq, float dir, float *x, float *w) {
  if (x) *x = esq;
  if (w) *w = NV_TELA_W - esq - dir;
}
float fil_tipo_fator(int t) { (void)t; return 1.0f; }
float gfx_escala_ui(void) { return 1.0f; }
int teclado_aberto(void) { return 0; }
void ctxhold_cancelar(CtxHold *h) { memset(h, 0, sizeof *h); }
static int paginas;
int desc_vertudo_n(void) { return 80; }
void desc_vertudo_mais(void) { paginas++; }
void lst_itens_mais(void) { paginas++; }

static void perto(float real, float esperado) { assert(fabsf(real - esperado) < 0.01f); }

int main(void) {
  PonteiroRolagem e = { PONT_ROL_INICIO, 1, 0.0f, 0.0f, 500.0f, 600.0f };
#if defined(TESTE_HOME)
  nFileiras = 5;
  for (int r = 0; r < nFileiras; r++) {
    memset(&fileiras[r], 0, sizeof fileiras[r]);
    fileiras[r].tipo = FILEIRA_NORMAL; fileiras[r].n = 12; fileiras[r].escala = 1.0f;
    foco.nColunas[r] = 12;
  }
  focoHero = 1; foco.fileira = foco.coluna = 0; pedidoAbrir = 0;
  assert(toqueHomeRolar(&e));
  e.fase = PONT_ROL_MOVER; e.delta = -37.0f;
  assert(toqueHomeRolar(&e)); perto(scrollY, 37.0f);
  assert(focoHero && foco.fileira == 0 && !pedidoAbrir);
  e.fase = PONT_ROL_FIM; toqueHomeRolar(&e); perto(scrollY, 37.0f);
  e.fase = PONT_ROL_MOVER; e.delta = -1e6f; toqueHomeRolar(&e); perto(scrollY, toqueHomeMaxY());
  e.fase = PONT_ROL_INERCIA; assert(!toqueHomeRolar(&e));
  e.delta = 1e6f; toqueHomeRolar(&e); perto(scrollY, -empurraHero());
  scrollY = 0.0f; e.fase = PONT_ROL_INICIO; e.eixoY = 0;
  e.y = topoFileiras() + NV_LEGACY_ROW_HEAD_H + 40.0f;
  assert(toqueHomeRolar(&e));
  e.fase = PONT_ROL_MOVER; e.delta = -37.0f;
  toqueHomeRolar(&e); perto(scrollX[0], 37.0f);
  assert(focoHero && !pedidoAbrir);
  e.fase = PONT_ROL_CANCELAR; toqueHomeRolar(&e); perto(scrollX[0], 37.0f);
  toqueHomeRetomarFoco(); assert(!toqueLivreY && !toqueLivreX[0]);
  assert(foco.fileira >= 0 && foco.fileira < nFileiras);
  fileiras[0].n = 1; scrollX[0] = 0.0f;
  e.fase = PONT_ROL_INICIO; assert(toqueHomeRolar(&e));
  e.fase = PONT_ROL_MOVER; e.delta = -37.0f; toqueHomeRolar(&e); perto(scrollX[0], 0.0f);
  e.fase = PONT_ROL_INICIO; e.x = 20.0f; assert(!toqueHomeRolar(&e));
#elif defined(TESTE_VERTUDO)
  foco = 0; pedAbrir = -1;
  assert(toqueVertudoRolar(&e));
  e.fase = PONT_ROL_MOVER; e.delta = -37.0f;
  toqueVertudoRolar(&e); perto(scrollY, 37.0f); assert(foco == 0 && pedAbrir == -1);
  e.delta = -1e6f; toqueVertudoRolar(&e); perto(scrollY, toqueVertudoMax()); assert(paginas);
  e.fase = PONT_ROL_INERCIA; assert(!toqueVertudoRolar(&e));
  e.fase = PONT_ROL_FIM; toqueVertudoRolar(&e); assert(toqueLivre);
  toqueVertudoRetomarFoco(); assert(!toqueLivre && foco >= 0 && foco < 80);
  e.fase = PONT_ROL_INICIO; e.y = 30.0f; assert(!toqueVertudoRolar(&e));
#elif defined(TESTE_BIBLIOTECA)
  nCelulas = 80; foco.fileira = foco.coluna = 0; pedido = -1; okDesde = 123;
  assert(toqueBibliotecaRolar(&e)); assert(!okDesde);
  e.fase = PONT_ROL_MOVER; e.delta = -37.0f;
  toqueBibliotecaRolar(&e); perto(scrollY, 37.0f); assert(foco.fileira == 0 && pedido == -1);
  e.delta = -1e6f; toqueBibliotecaRolar(&e); perto(scrollY, toqueBibliotecaMax());
  e.fase = PONT_ROL_INERCIA; assert(!toqueBibliotecaRolar(&e));
  e.fase = PONT_ROL_CANCELAR; toqueBibliotecaRolar(&e); assert(toqueLivre);
  toqueBibliotecaRetomarFoco(); assert(!toqueLivre && foco.fileira >= gradeIni());
  e.fase = PONT_ROL_INICIO; e.y = 20.0f; assert(!toqueBibliotecaRolar(&e));
#elif defined(TESTE_BUSCA)
  nFil = 3;
  for (int r = 0; r < nFil; r++) fil[r].n = 12;
  painel = 0; focoRes.fileira = focoRes.coluna = 0; pedido = -1;
  e.x = BU_RES_X + 100.0f;
  assert(toqueBuscaRolar(&e));
  e.fase = PONT_ROL_MOVER; e.delta = -37.0f;
  toqueBuscaRolar(&e); perto(scrollY, 37.0f); perto(scrollAlvo, 37.0f);
  assert(painel == 0 && pedido == -1);
  e.delta = -1e6f; toqueBuscaRolar(&e); perto(scrollY, toqueBuscaMaxY());
  e.fase = PONT_ROL_INERCIA; assert(!toqueBuscaRolar(&e));
  e.fase = PONT_ROL_FIM; toqueBuscaRolar(&e); assert(toqueLivreY);
  scrollY = scrollAlvo = 0.0f; e.fase = PONT_ROL_INICIO; e.eixoY = 0; e.y = BU_RES_Y + BU_KICK_H + 40.0f;
  assert(toqueBuscaRolar(&e));
  e.fase = PONT_ROL_MOVER; e.delta = -37.0f;
  toqueBuscaRolar(&e); perto(scrollX[0], 37.0f); assert(pedido == -1);
  toqueBuscaRetomarFoco(); assert(!toqueLivreY && !toqueLivreX[0]);
  fil[0].n = 1; scrollX[0] = 0.0f;
  e.fase = PONT_ROL_INICIO; assert(toqueBuscaRolar(&e));
  e.fase = PONT_ROL_MOVER; e.delta = -37.0f; toqueBuscaRolar(&e); perto(scrollX[0], 0.0f);
  e.fase = PONT_ROL_INICIO; e.x = BU_KB_X; assert(!toqueBuscaRolar(&e));
#endif
  puts("catalogo_toque: OK");
  return 0;
}
