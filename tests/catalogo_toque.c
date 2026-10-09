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
float nv_layout_h = 1080.0f;
#if defined(TESTE_HOME)
static int layoutTeste = HOME_LAYOUT_MODERNA;
static int heroTeste = 1, posterDpTeste = 126;
int ajustes_home_layout(void) { return layoutTeste; }
int ajustes_hero_ligado(void) { return heroTeste; }
int ajustes_largura_poster_dp(void) { return posterDpTeste; }
#else
int ajustes_home_layout(void) { return HOME_LAYOUT_MODERNA; }
int ajustes_hero_ligado(void) { return 1; }
int ajustes_largura_poster_dp(void) { return 126; }
#endif
float ajustes_conteudo_x(void) { return 104.0f; }
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
#if defined(TESTE_BUSCA)
int menu_pilula_rect(float *x, float *y, float *w, float *h) {
  (void)x; (void)y; (void)w; (void)h; return 0;
}
#endif
void ctxhold_cancelar(CtxHold *h) { memset(h, 0, sizeof *h); }
static int paginas;
int desc_vertudo_n(void) { return 80; }
void desc_vertudo_mais(void) { paginas++; }
void lst_itens_mais(void) { paginas++; }
#if defined(TESTE_HOME)
int amigosfil_rolagem(const PonteiroRolagem *e) { (void)e; return 0; }
void amigosfil_retomar_foco(int *coluna) { (void)coluna; }
Uint32 SDL_GetTicks(void) { return 100; }
static int alvosHero;
static GfxRect alvoHero;
void ponteiro_alvo(float x, float y, float w, float h, PonteiroFn focar,
                   PonteiroFn segurar, int a, int b) {
  (void)segurar; (void)a; (void)b;
  assert(focar == ponteiroHero);
  alvoHero = (GfxRect){x, y, w, h}; alvosHero++;
}
#endif

static void perto(float real, float esperado) { assert(fabsf(real - esperado) < 0.01f); }

#if defined(TESTE_HOME)
static void testaHeroTelefone(void) {
  // Tablet/TV retains the published anchor and height, including the short
  // Padrao banner. Wide phones reveal a useful part of row0 at hero rest.
  const float larguras[] = {1920.0f, 1728.0f, 2400.0f, 2520.0f};
  const int layouts[] = {HOME_LAYOUT_MODERNA, HOME_LAYOUT_PADRAO, HOME_LAYOUT_DINAMICA};
  for (int w = 0; w < 4; w++) {
    nv_layout_w = larguras[w];
    int compacto = w >= 2;
    assert(heroCompactoTelefone() == compacto);
    for (int l = 0; l < 3; l++) {
      layoutTeste = layouts[l]; toqueHomeLimpar();
      scrollY = -empurraHero();
      float primeira = topoFileiras() - scrollY;
      if (layoutTeste == HOME_LAYOUT_MODERNA)
        perto(primeira, compacto ? 749.25f : 999.0f);
      else if (layoutTeste == HOME_LAYOUT_DINAMICA)
        perto(primeira, compacto ? 735.0f : 980.0f);
      else perto(primeira, 544.0f);
      GfxRect arte = layoutTeste == HOME_LAYOUT_PADRAO ? padBannerRect()
                   : (GfxRect){0, dinHeroY(), NV_TELA_W, NV_TELA_H};
      float base = heroBaseCopia(layoutTeste, arte, -scrollY, 0);
      if (compacto) {
        assert(primeira + NV_LEGACY_ROW_HEAD_H + 200.0f < NV_TELA_H);
        // Rich real copy, translated caption, avatars and fallback names all
        // remain below the header; action and shelf cannot cover each other.
        for (int meta = 0; meta < 2; meta++)
          for (int sec = 0; sec < 2; sec++)
            for (int caption = 0; caption < 2; caption++)
              for (int friends = 0; friends < 2; friends++) {
                float sinH = 3.0f * NV_LD_HERO_SIN;
                float capH = caption ? NV_LD_HERO_META : 0;
                float amiH = friends ? NV_AMIGOS_HERO_H : 0;
                float gap = layoutTeste == HOME_LAYOUT_MODERNA ? NV_HOME_HERO_BOTAO_GAP : 22;
                HeroCopyLayout p = heroCopyTelefone(base, &sinH, meta, sec, &capH,
                    150, NV_HERO_BOTAO_COMPACTO_H, gap, 132, 1, &amiH, 76);
                assert(p.logo >= 132.0f - 0.01f && p.logoHeight >= 76.0f);
                assert(p.logo + p.logoHeight + gap <= p.caption + 0.01f);
                assert(p.synopsis + sinH + gap <= p.action + 0.01f);
                assert(p.action + NV_HERO_BOTAO_COMPACTO_H + 24.0f < primeira);
                alvosHero = 0;
                alvoAcaoHero((GfxRect){104, p.action, 240, NV_HERO_BOTAO_COMPACTO_H}, 0, 0, 1);
                assert(alvosHero == 1); perto(alvoHero.y, p.action);
              }
      }
    }
  }
  // A finger on the hero can start vertical browsing. The viewport follows
  // the same offset; after copy disappears, rows can reach the header edge.
  nv_layout_w = 2400; layoutTeste = HOME_LAYOUT_MODERNA;
  focoHero = 1; pedidoAbrir = 0;
  toqueHomeLimpar(); scrollY = -empurraHero();
  PonteiroRolagem e = {PONT_ROL_INICIO, 1, 0, 0, 500, 300};
  assert(toqueHomeRolar(&e)); perto(heroVisivelToque(), 1);
  float repouso = -scrollY;
  e.fase = PONT_ROL_MOVER; e.delta = -repouso * 0.5f; toqueHomeRolar(&e);
  perto(heroVisivelToque(), 0.5f);
  assert(corteFileiras() > 132 && corteFileiras() < NV_SHELF_TOP);
  e.delta = -repouso * 0.5f; toqueHomeRolar(&e);
  perto(heroVisivelToque(), 0); perto(corteFileiras(), 132);
  e.delta = -200; toqueHomeRolar(&e); perto(scrollY, 200);
  assert(focoHero && !pedidoAbrir);
  alvosHero = 0;
  alvoAcaoHero((GfxRect){104, 300, 240, 72}, 0, 0, 0); assert(!alvosHero);
  alvoAcaoHero((GfxRect){104, 100, 240, 72}, 0, 0, 1); assert(!alvosHero);
  alvoAcaoHero((GfxRect){104, topoFileiras() - scrollY, 240, 72}, 0, 0, 1); assert(!alvosHero);
  // Padrao artwork retains its size/aspect while moving with the shelves.
  layoutTeste = HOME_LAYOUT_PADRAO; scrollY = 32;
  perto(heroVisivelToque(), 0.5f);
  perto(padBannerRect().y, -32); perto(padBannerRect().h, NV_PAD_BANNER_H);
  scrollY = 64; perto(heroVisivelToque(), 0);
  scrollY = 600; perto(corteFileiras(), 132);
  layoutTeste = HOME_LAYOUT_DINAMICA; scrollY = -empurraHero();
  perto(dinHeroY(), 0); perto(corteFileiras(), 132);
  scrollY += 320; perto(dinHeroY(), -320);
  assert(alturaTextoHeroDin() + dinHeroY() < topoFileiras() - scrollY);
  // The same rich title also fits when Moderna's action slot collapses.
  layoutTeste = HOME_LAYOUT_MODERNA;
  float sinH = 90, capH = 26, amiH = 36;
  HeroCopyLayout p = heroCopyTelefone(NV_SHELF_TOP - NV_HERO_COPY_GAP - 24,
      &sinH, 1, 1, &capH, 150, 72, NV_HOME_HERO_BOTAO_GAP, 132, 0, &amiH, 76);
  assert(p.logo >= 132 && p.logoHeight >= 76);
  toqueHomeLimpar(); scrollY = 0;
}

static void testaHomeRetrato(void) {
  const float alturas[] = {1920, 2340};
  const int layouts[] = {HOME_LAYOUT_MODERNA, HOME_LAYOUT_PADRAO, HOME_LAYOUT_DINAMICA};
  for (int h = 0; h < 2; h++) {
    nv_layout_w = 1080; nv_layout_h = alturas[h];
    assert(homeRetratoTelefone() && heroCompactoTelefone());
    assert(ajustes_conteudo_x() + homeTextoLargura(1200) + 48 <= NV_TELA_W);
    for (int l = 0; l < 3; l++) {
      layoutTeste = layouts[l]; toqueHomeLimpar(); scrollY = -empurraHero();
      for (int r = 0; r < nFileiras; r++) {
        fileiras[r].tipo = FILEIRA_NORMAL; fileiras[r].n = 12;
        fileiras[r].escala = 1; fileiras[r].stackN = 0;
      }
      float primeira = topoFileiras() - scrollY;
      perto(primeira, layoutTeste == HOME_LAYOUT_PADRAO ? 544 : 620);
      float w = larguraFil(0), a = alturaFil(0);
      assert(w > 200 && 2 * passoFil(0) + w < homeLarguraUtil());
      perto(w / a, layoutTeste == HOME_LAYOUT_PADRAO ? NV_PAD_CARTAZ_W / NV_PAD_CARTAZ_H : NV_CARD_W / NV_CARD_H);
      float fim = primeira + NV_LEGACY_ROW_HEAD_H + alturaTotalFil(0);
      float segunda = fim + fileiraGap();
      assert(segunda + NV_LEGACY_ROW_HEAD_H + alturaTotalFil(1) < NV_TELA_H);
      perto(alfaFileiraHome(0, primeira, topoFileiras(), corteFileiras()), 1);
      perto(alfaFileiraHome(1, segunda, topoFileiras(), corteFileiras()), 1);
      GfxRect arte = layoutTeste == HOME_LAYOUT_PADRAO ? padBannerRect()
                   : (GfxRect){0, dinHeroY(), NV_TELA_W, 1080};
      float base = heroBaseCopia(layoutTeste, arte, -scrollY, 0);
      float sinH = 90, capH = 26, amiH = 36;
      HeroCopyLayout p = heroCopyTelefone(base, &sinH, 1, 1, &capH, 150, 72,
          layoutTeste == HOME_LAYOUT_MODERNA ? NV_HOME_HERO_BOTAO_GAP : 22, 132, 1, &amiH, 76);
      assert(p.logo >= 132 && p.action + 72 + 24 < primeira);
      alvosHero = 0; alvoAcaoHero((GfxRect){104, p.action, 240, 72}, 0, 0, 1);
      assert(alvosHero == 1 && alvoHero.x + alvoHero.w <= NV_TELA_W);
    }
    layoutTeste = HOME_LAYOUT_MODERNA; toqueHomeLimpar();
    scrollY = -empurraHero(); scrollX[0] = 0;
    focoHero = 1; foco.fileira = -1; pedidoAbrir = 0;
    PonteiroRolagem e = {PONT_ROL_INICIO, 1, 0, 0, 500, 300};
    assert(toqueHomeRolar(&e));
    e.fase = PONT_ROL_MOVER; e.delta = -empurraHero(); toqueHomeRolar(&e);
    perto(scrollY, 0); perto(heroVisivelToque(), 0); perto(corteFileiras(), 132);
    e.delta = -13.5f; toqueHomeRolar(&e); perto(scrollY, 13.5f);
    e.fase = PONT_ROL_FIM; toqueHomeRolar(&e); perto(scrollY, 13.5f);
    assert(focoHero && foco.fileira == -1 && !pedidoAbrir);
    // Rows stay visible even though the stale TV focus is still on the hero.
    perto(alfaFileiraHome(0, topoFileiras() - scrollY, topoFileiras(), corteFileiras()), 1);
    e.fase = PONT_ROL_INERCIA; e.delta = -1e6f; toqueHomeRolar(&e);
    perto(scrollY, toqueHomeMaxY()); assert(!toqueHomeRolar(&e));
    e.fase = PONT_ROL_INICIO; e.x = 20; assert(!toqueHomeRolar(&e));
    e.x = 500; e.y = 100; assert(!toqueHomeRolar(&e));
    scrollY = 0; e.eixoY = 0; e.y = topoFileiras() + NV_LEGACY_ROW_HEAD_H + 40;
    assert(toqueHomeRolar(&e)); e.fase = PONT_ROL_MOVER; e.delta = -13.5f;
    toqueHomeRolar(&e); perto(scrollX[0], 13.5f);
    e.fase = PONT_ROL_FIM; toqueHomeRolar(&e); assert(!pedidoAbrir);
    // User sizing remains meaningful, and every row style bounds its card
    // before drawing so artwork aspect and touch geometry stay identical.
    posterDpTeste = 72; float pequeno = larguraFil(0);
    posterDpTeste = 200; assert(larguraFil(0) > pequeno); posterDpTeste = 126;
    const TipoFileira tipos[] = {FILEIRA_NORMAL, FILEIRA_CONTINUE, FILEIRA_DESTAQUE,
        FILEIRA_LARGA, FILEIRA_DESTAQUE_QUADRADO, FILEIRA_COLECAO,
        FILEIRA_SERVICO, FILEIRA_SOCIAL, FILEIRA_RETORNO, FILEIRA_TOP10_NUM};
    for (int t = 0; t < (int)(sizeof tipos / sizeof tipos[0]); t++) {
      fileiras[0].tipo = tipos[t]; fileiras[0].escala = 1;
      float aspecto = larguraFil(0) / alturaFil(0);
      fileiras[0].escala = 5;
      assert(larguraFil(0) + xOffTipo(tipos[t]) <= homeLarguraUtil());
      perto(larguraFil(0) / alturaFil(0), aspecto);
    }
    fileiras[0].tipo = FILEIRA_NORMAL; fileiras[0].escala = 1;
    GfxRect editorial = heroEditorialRect(1, 0);
    assert(editorial.w <= NV_TELA_W); perto(editorial.w / editorial.h, 1920.0f / 500.0f);
    editorial = heroEditorialRect(2, 1.25f);
    assert(editorial.x >= 0 && editorial.x + editorial.w <= NV_TELA_W);
    perto(editorial.w / editorial.h, 1.25f);
    heroTeste = 0; toqueHomeLimpar(); perto(topoFileiras(), 150); perto(corteFileiras(), 132); perto(empurraHero(), 0);
    heroTeste = 1;
  }
  // A normal portrait tablet retains its original layout and card dimensions.
  nv_layout_w = 1080; nv_layout_h = 1728; layoutTeste = HOME_LAYOUT_MODERNA;
  toqueHomeLimpar(); assert(!homeRetratoTelefone());
  perto(topoFileiras() + empurraHero(), 999); perto(larguraFil(0), NV_CARD_W);
  nv_layout_w = 2400; nv_layout_h = 1080;
  perto(topoFileiras() + empurraHero(), 749.25f); perto(larguraFil(0), NV_CARD_W);
}
#endif

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
  testaHeroTelefone();
  testaHomeRetrato();
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
  const float alturasBib[] = {2340, 1920};
  for (int h = 0; h < 2; h++) {
    nv_layout_w = 1080; nv_layout_h = alturasBib[h];
    assert(bibX() >= 0 && bibW() > 0 && bibDireita() < NV_TELA_W);
    perto(bibX() + bibW(), bibDireita());
    modo = MODO_SALVOS; exibicao = VIS_CARTAZ; temAberta = 0;
    int cols = colunasCartaz(); assert(cols >= 2 && cols < BIB_COLUNAS_MAX);
    assert(bibX() + (cols - 1) * passoColuna() + BIB_CARD_W <= bibDireita());
    modo = MODO_LISTAS; cols = colunasLista(); assert(cols >= 2 && cols < BIB_LC_COLS);
    assert(larguraCartaoLista() >= 250);
    assert(bibX() + (cols - 1) * passoColuna() + larguraCartaoLista() <= bibDireita());
    exibicao = VIS_LISTA; assert(colunas() == 1);
  }
  nv_layout_w = 2400; nv_layout_h = 1080;
  perto(bibDireita(), NV_BIB_DIR); assert(colunasLista() == BIB_LC_COLS);
#elif defined(TESTE_BUSCA)
  nv_layout_w = 1080; nv_layout_h = 2340;
  perto(BU_COL_W, NV_TELA_W - 2.0f * BU_MARG);
  perto(BU_RES_X, BU_KB_X);
  for (int f = 0; f <= kbFil; f++) for (int c = 0; c < KB_COLUNAS[f]; c++) {
    GfxRect t = retanguloTecla(f, c);
    assert(t.x >= BU_KB_X && t.x + t.w <= BU_DIR);
    assert(t.y + t.h < BU_RES_Y);
  }
  float latinY = BU_RES_Y;
  kbFil = 7; assert(BU_RES_Y > latinY); kbFil = 6;
  nFil = 6;
  for (int r = 0; r < nFil; r++) fil[r].n = 12;
  e.x = BU_RES_X + 100; e.y = BU_RES_Y + 100;
  assert(toqueBuscaRolar(&e));
  e.fase = PONT_ROL_MOVER; e.delta = -37.5f;
  toqueBuscaRolar(&e); perto(scrollY, 37.5f);
  assert(focoRes.fileira == 0 && focoRes.coluna == 0);
  toqueBuscaRetomarFoco(); scrollY = scrollAlvo = 0;
  nv_layout_w = 2400; nv_layout_h = 1080;
  perto(BU_COL_W, 520); perto(BU_RES_X, buX() + 576); perto(BU_RES_Y, 64);
  e.fase = PONT_ROL_INICIO; e.eixoY = 1; e.y = BU_RES_Y + 100;
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
