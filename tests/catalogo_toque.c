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
#if defined(TESTE_HOME)
static float railTeste;
float ajustes_conteudo_x(void) { return NV_CONTENT_PAD + railTeste; }
float ajustes_rail_largura_fixa(void) { return railTeste; }
#elif defined(TESTE_BUSCA)
static float buscaRailTeste;
float ajustes_conteudo_x(void) { return NV_CONTENT_PAD + buscaRailTeste; }
float ajustes_rail_largura_fixa(void) { return buscaRailTeste; }
#else
float ajustes_conteudo_x(void) { return 104.0f; }
float ajustes_rail_largura_fixa(void) { return 0.0f; }
#endif
int ajustes_posteres_deitados(void) { return 0; }
int ajustes_rotulos_poster(void) { return 1; }
int ajustes_borda_foco(void) { return 1; }
float ajustes_espaco_titulos(void) { return 1.0f; }
float ajustes_espaco_fileiras(void) { return 1.0f; }
void ajustes_area_conteudo(float esq, float dir, float *x, float *w) {
  if (x) *x = esq;
  if (w) *w = NV_TELA_W - esq - dir;
}
float fil_tipo_fator(int t) { (void)t; return 1.0f; }
#if defined(TESTE_BIBLIOTECA)
static float escalaBibTeste = 1.0f;
float gfx_escala_ui(void) { return escalaBibTeste; }
static GfxRect alvoBibTeste;
static int alvosBibTeste, textosBibTeste, fonteBibLarga;
int lst_n(void) { return 12345678; }
int lst_fixada(const LstLista *l) { (void)l; return 0; }
int lst_na_home(const LstLista *l) { (void)l; return 0; }
int lst_aceita_home(const LstLista *l, const char **porque) { (void)l; (void)porque; return 1; }
int ajustes_vidro(void) { return 0; }
int ajustes_tinta_foco(void) { return 24; }
int ajustes_tinta_foco2(void) { return 60; }
void ajustes_acento(float *r, float *g, float *b) { *r = 1; *g = .6f; *b = .2f; }
void ponteiro_alvo(float x, float y, float w, float h, PonteiroFn focar, PonteiroFn ativar, int a, int b) {
  (void)ativar; (void)a; (void)b;
  assert(focar == ponteiroModo || focar == ponteiroPicker || focar == ponteiroAcao);
  alvoBibTeste = (GfxRect){x,y,w,h}; alvosBibTeste++;
  if (bibRetrato()) assert(x >= hdrX() && x + w <= hdrDir() + .01f);
}
void gfx_cor(GfxRect r, float raio, float cr, float cg, float cb, float ca) {
  (void)r; (void)raio; (void)cr; (void)cg; (void)cb; (void)ca;
}
void gfx_vidro_pilula_cheia(GfxRect r, float raio, float foco, float a) { (void)r; (void)raio; (void)foco; (void)a; }
void gfx_rect(GfxRect r, GLuint tex, GfxModo modo, float foco, float parx, float pary, float raio,
              float cr, float cg, float cb, float ca) {
  (void)tex; (void)foco; (void)parx; (void)pary; (void)raio;
  (void)cr; (void)cg; (void)cb; (void)ca;
  if (modo == GFX_TEXTO) {
    textosBibTeste++;
    if (bibRetrato()) {
      assert(r.x >= alvoBibTeste.x && r.x + r.w <= alvoBibTeste.x + alvoBibTeste.w + .51f);
      assert(r.y >= alvoBibTeste.y && r.y + r.h <= alvoBibTeste.y + alvoBibTeste.h + .51f);
    }
  }
}
TxtLinha txt_linha(TxtEstilo estilo, const char *s, int r, int g, int b, int a) {
  (void)estilo; (void)r; (void)g; (void)b; (void)a;
  return (TxtLinha){.tex=1, .w=(int)strlen(s)*(fonteBibLarga ? 40 : 13), .h=24};
}
TxtLinha txt_linha_corta(TxtEstilo estilo, const char *s, int r, int g, int b, int a, float maxW) {
  assert(maxW > 0);
  TxtLinha t = txt_linha(estilo,s,r,g,b,a);
  if (t.w > maxW) t.w = (int)maxW;
  return t;
}
#else
float gfx_escala_ui(void) { return 1.0f; }
#endif
int teclado_aberto(void) { return 0; }
#if defined(TESTE_BUSCA)
static int imeBuscaTeste, vozBuscaTeste, celBuscaTeste, alvosCampo, textosCampo, recortesCampo;
static int fonteLargaTeste, pilulaBuscaTeste, recorteBuscaAtivo;
static GfxRect campoBuscaTeste, clipBuscaTeste, cursorBuscaTeste;
static char avisoBuscaTeste[160];
static int resultadosBuscaTeste, alvosResultadosTeste, desenhosResultadosTeste, recentesBuscaTeste;
static CatItem itensBuscaTeste[BU_MAX_POR_FIL];
static GfxRect alvosBuscaTeste[128];
static GfxRect ultimoRotuloBusca;
static int pedidosImeBusca;
float gfx_tex_aspect_atual, gfx_varre_atual;
const CatItem *cat_item(int i) { return i >= 0 && i < BU_MAX_POR_FIL ? &itensBuscaTeste[i] : NULL; }
const char *posterprov_card_addon(const char *origem, const char *imdb, long tmdb, const char *tipo, const char *url) {
  (void)origem; (void)imdb; (void)tmdb; (void)tipo; return url;
}
GLuint tex_obter_larg(const char *url, float w) { (void)url; (void)w; return 0; }
float tex_aspecto(const char *url) { (void)url; return 1.5f; }
int tex_falhou(const char *url) { (void)url; return 0; }
int focus_indice(const Foco *f, int r, int c) { return f->fileira == r && f->coluna == c; }
int buscasrec_n(void) { return recentesBuscaTeste; }
const char *buscasrec_termo(int i) { (void)i; return "A long recent query that must stay inside the phone results viewport"; }
static void desenhoBusca(GfxRect r) {
  if (!resultadosBuscaTeste) return;
  assert(recorteBuscaAtivo && r.w >= 0 && r.h >= 0);
  assert(clipBuscaTeste.x >= 0 && clipBuscaTeste.x + clipBuscaTeste.w <= NV_TELA_W + .01f);
  assert(clipBuscaTeste.y >= 0 && clipBuscaTeste.y + clipBuscaTeste.h <= NV_TELA_H + .01f);
  desenhosResultadosTeste++;
}
float gfx_opacidade_grupo = 1;
int menu_pilula_rect(float *x, float *y, float *w, float *h) {
  *x = 40; *y = 44; *w = 300; *h = 60; return pilulaBuscaTeste;
}
int st_ime_disponivel(void) { return imeBuscaTeste; }
int st_ime_abrir(int dono, const char *inicial, int max) {
  assert(dono == ST_BUSCA && inicial == consulta && max == BU_MAX_CONSULTA-1);
  pedidosImeBusca++; return 1;
}
int st_voz_iniciar(int dono) { (void)dono; return 0; }
void st_fechar(int dono) { (void)dono; }
int celb_abrir(int dono, const char *titulo) { (void)dono; (void)titulo; return 0; }
int st_voz_disponivel(void) { return vozBuscaTeste; }
int celb_disponivel(void) { return celBuscaTeste; }
int st_dono(void) { return ST_BUSCA; }
int st_estado(void) { return ST_PARADO; }
const char *st_aviso(void) { return avisoBuscaTeste; }
float st_nivel(void) { return 0; }
const char *i18n(const char *s) { return s; }
int ajustes_tinta_foco(void) { return 24; }
void ajustes_acento(float *r, float *g, float *b) { *r = 1; *g = .6f; *b = .2f; }
void ajustes_ui_ilha(GfxRect r, float raio, int modal) {
  (void)raio; (void)modal;
  if (resultadosBuscaTeste) desenhoBusca(r); else campoBuscaTeste = r;
}
void ajustes_ui_foco_linha(GfxRect r, float raio) { (void)r; (void)raio; }
void ajustes_ui_neutro(GfxRect r, float raio, float a) { (void)r; (void)raio; (void)a; }
int ponteiro_ativo(void) { return 1; }
void ponteiro_alvo(float x, float y, float w, float h, PonteiroFn focar,
                   PonteiroFn ativar, int a, int b) {
  (void)ativar; (void)a; (void)b;
  if (resultadosBuscaTeste) {
    assert(focar == ponteiroResultado || focar == ponteiroRecente);
    assert(w > 0 && h > 0 && x >= BU_RES_X - .01f && x + w <= BU_DIR + .01f);
    assert(y >= BU_RES_Y - 20 - .01f && y + h <= NV_TELA_H - 48 + .01f);
    assert(alvosResultadosTeste < 128);
    alvosBuscaTeste[alvosResultadosTeste++] = (GfxRect){x,y,w,h};
    return;
  }
  assert(focar == focarCampoPonteiro);
  if (buRetrato()) assert(x >= campoBuscaTeste.x && x + w <= campoBuscaTeste.x + campoBuscaTeste.w);
  alvosCampo++;
}
void celb_botao(int dono, GfxRect r, int focado, PonteiroFn focar, int a, int b, float alpha) {
  (void)dono; (void)focado; (void)alpha;
  ponteiro_alvo(r.x, r.y, r.w, r.h, focar, NULL, a, b);
}
void gfx_recorte(float x, float y, float w, float h) {
  clipBuscaTeste = (GfxRect){x,y,w,h}; recorteBuscaAtivo = 1; recortesCampo++;
  assert(w > 0);
  if (resultadosBuscaTeste) {
    assert(x == BU_RES_X && x + w <= BU_DIR + .01f && y + h <= NV_TELA_H - 48 + .01f);
  } else assert(x >= campoBuscaTeste.x && x + w <= campoBuscaTeste.x + campoBuscaTeste.w);
}
void gfx_sem_recorte(void) { assert(recorteBuscaAtivo); recorteBuscaAtivo = 0; }
void gfx_cor(GfxRect r, float raio, float cr, float cg, float cb, float ca) {
  (void)raio; (void)cr; (void)cg; (void)cb; (void)ca;
  desenhoBusca(r);
  if (r.w == 2 && r.h == 30) {
    cursorBuscaTeste = r;
    if (buRetrato()) assert(recorteBuscaAtivo && r.x + r.w <= clipBuscaTeste.x + clipBuscaTeste.w);
  }
}
void gfx_rect(GfxRect r, GLuint tex, GfxModo modo, float foco, float parx, float pary, float raio,
                float cr, float cg, float cb, float ca) {
  (void)tex; (void)modo; (void)foco; (void)parx; (void)pary; (void)raio;
  (void)cr; (void)cg; (void)cb; (void)ca;
  desenhoBusca(r);
}
void gfx_esqueleto(GfxRect r, float raio, float cr, float cg, float cb, float ca) {
  (void)raio; (void)cr; (void)cg; (void)cb; (void)ca; desenhoBusca(r);
}
void gfx_icone(GfxRect r, const char *nome, float cr, float cg, float cb, float ca) {
  (void)r; (void)nome; (void)cr; (void)cg; (void)cb; (void)ca;
}
TxtLinha txt_linha_corta(TxtEstilo estilo, const char *s, int r, int g, int b, int a, float maxW) {
  (void)estilo; (void)r; (void)g; (void)b; (void)a;
  assert(maxW > 0);
  if (resultadosBuscaTeste) {
    GLuint marca = estilo == TXT_ILHA_NOME && s == itensBuscaTeste[BU_MAX_POR_FIL-1].titulo ? 2 : 0;
    return (TxtLinha){marca, (int)fminf(strlen(s)*(fonteLargaTeste ? 40 : 24),maxW),30,0,0};
  }
  return (TxtLinha){0, fonteLargaTeste ? (int)maxW + 180 : (int)fminf(strlen(s)*24, maxW), 30, 0, 0};
}
TxtLinha txt_linha(TxtEstilo estilo, const char *s, int r, int g, int b, int a) {
  (void)estilo; (void)r; (void)g; (void)b; (void)a;
  return (TxtLinha){0, (int)strlen(s)*24,30,0,0};
}
void txt_desenhar(TxtLinha l, float x, float y) {
  if (resultadosBuscaTeste) {
    GfxRect r = {x,y,(float)l.w,(float)l.h};
    if (l.tex == 2) ultimoRotuloBusca = r;
    desenhoBusca(r); return;
  }
  textosCampo++;
  if (buRetrato()) assert(recorteBuscaAtivo && x < clipBuscaTeste.x + clipBuscaTeste.w && l.w > 0);
}
void txt_desenhar_alpha(TxtLinha l, float x, float y, float a) { (void)a; txt_desenhar(l,x,y); }
#endif
void ctxhold_cancelar(CtxHold *h) { memset(h, 0, sizeof *h); }
static int paginas;
int desc_vertudo_n(void) { return 80; }
void desc_vertudo_mais(void) { paginas++; }
void lst_itens_mais(void) { paginas++; }
#if defined(TESTE_HOME)
/* The page-scroll fixture has no carousel catalogue. Horizontal drags in
   rows still exercise Home's real row path after its hero capture declines. */
#ifndef TESTE_HOME_ROWS_REVIEW
int cat_n(void) { return 0; }
const CatItem *cat_item(int i) { (void)i; return NULL; }
#endif
int cat_n_fileiras(void) { return 0; }
const CatFileira *cat_fileira(int i) { (void)i; return NULL; }
unsigned fil_revisao(void) { return 0; }
const char *fil_hero_fonte(void) { return ""; }
void desc_sinopse_hero(const int *i, int n) { (void)i; (void)n; }
int amigosfil_indice_cat(int c) { (void)c; return -1; }
int detail_aberto(void) { return 0; }
int player_aberto(void) { return 0; }
int amigosfil_rolagem(const PonteiroRolagem *e) { (void)e; return 0; }
void amigosfil_retomar_foco(int *coluna) { (void)coluna; }
Uint32 SDL_GetTicks(void) { return 100; }
static int alvosHero;
static GfxRect alvoHero;
static int alvosCard;
static GfxRect alvoCartao;
int ponteiro_ativo(void) { return 1; }
void ponteiro_alvo(float x, float y, float w, float h, PonteiroFn focar,
                   PonteiroFn segurar, int a, int b) {
  (void)segurar; (void)a; (void)b;
  if (focar == ponteiroHero) { alvoHero = (GfxRect){x, y, w, h}; alvosHero++; }
  else { assert(focar == ponteiroCard); alvoCartao = (GfxRect){x, y, w, h}; alvosCard++; }
}
#endif

static void perto(float real, float esperado) { assert(fabsf(real - esperado) < 0.01f); }

#if defined(TESTE_BUSCA)
static void testaResultadosBuscaTelefone(void) {
  const float telas[][2] = {{1080,1920},{1080,2340},{2340,1080},{2520,1080}};
  for (int d = 0; d < 4; d++) for (int fonte = 0; fonte < 2; fonte++) for (int rail = 0; rail < 2; rail++) {
    nv_layout_w = telas[d][0]; nv_layout_h = telas[d][1];
    imeBuscaTeste = 1; vozBuscaTeste = celBuscaTeste = 0; buscaRailTeste = rail ? 140 : 0;
    fonteLargaTeste = fonte; pilulaBuscaTeste = 0; resultadosBuscaTeste = 0;
    nConsulta = 0; consulta[0] = 0; avisoBuscaTeste[0] = 0;
    assert(buTecladoNativo()); desenhaCampo(0);
    perto(BU_RES_X, buX()); perto(BU_RES_Y, buTopoEsq() + BU_CAMPO_H + 44);
    perto(campoBuscaTeste.x, BU_RES_X); perto(campoBuscaTeste.x + campoBuscaTeste.w, BU_DIR);
    assert(BU_RES_AREA_H > 700 && BU_CAMPO_H >= 100 && buCartazW() >= 200);
    int desenhosAntes = textosCampo; desenhaTeclado(); assert(textosCampo == desenhosAntes);
    pedidosImeBusca = 0; focarCampoPonteiro(1,0); campoOk();
    assert(pedidosImeBusca == 1 && campoFoco == 1 && painel == 0);
    memset(fil,0,sizeof fil); memset(pess,0,sizeof pess); memset(animRes,0,sizeof animRes);
    memset(scrollX,0,sizeof scrollX); memset(filEntraEm,0,sizeof filEntraEm);
    memset(filNova,0,sizeof filNova); toqueBuscaLimpar(); scrollY = scrollAlvo = 0;
    sugestao = 0; nFil = 6; nPess = BU_MAX_PESS; recentesBuscaTeste = 0;
    nConsulta = 3; snprintf(consulta,sizeof consulta,"abc");
    fil[0].melhor = 1; fil[0].n = 1;
    fil[1].pessoas = 1; fil[1].n = BU_MAX_PESS; fil[2].n = BU_MAX_POR_FIL;
    for (int r = 3; r < nFil; r++) fil[r].n = BU_MAX_POR_FIL;
    for (int r = 0; r < nFil; r++) {
      fil[r].titulo = "A very long catalogue title that must not push its source outside the phone screen";
      fil[r].origem = "A similarly long addon name";
      for (int c = 0; c < fil[r].n; c++) { fil[r].itens[c] = c; animRes[r][c] = 1; }
    }
    for (int c = 0; c < BU_MAX_POR_FIL; c++) {
      memset(&itensBuscaTeste[c],0,sizeof itensBuscaTeste[c]);
      snprintf(itensBuscaTeste[c].titulo,sizeof itensBuscaTeste[c].titulo,"A long title for poster %d",c);
      snprintf(itensBuscaTeste[c].meta,sizeof itensBuscaTeste[c].meta,"2026 · Movie");
      snprintf(itensBuscaTeste[c].genero,sizeof itensBuscaTeste[c].genero,"Adventure");
    }
    for (int c = 0; c < BU_MAX_PESS; c++) snprintf(pess[c].nome,sizeof pess[c].nome,"A long performer name %d",c);
    resultadosBuscaTeste = 1; alvosResultadosTeste = desenhosResultadosTeste = 0;
    painel = 1; focoRes.fileira = 2; focoRes.coluna = 0; pedido = -1;
    desenhaResultados(1000);
    assert(!recorteBuscaAtivo && alvosResultadosTeste >= 4 && desenhosResultadosTeste >= 10);
    PonteiroRolagem e = {PONT_ROL_INICIO,0,0,0,BU_RES_X+100,BU_RES_Y+filTopo(2)+filKickH(2)+20};
    assert(toqueBuscaRolar(&e)); e.fase = PONT_ROL_MOVER; e.delta = -37.5f;
    assert(toqueBuscaRolar(&e)); perto(scrollX[2],37.5f); assert(pedido == -1);
    e.delta = -1e6f; toqueBuscaRolar(&e); perto(scrollX[2],toqueBuscaMaxX(2));
    alvosResultadosTeste = 0; desenhaResultados(1100); assert(alvosResultadosTeste > 0 && !recorteBuscaAtivo);
    /* Stale landscape scroll offsets cannot leave a blank portrait page. */
    scrollY = scrollAlvo = 100000;
    for (int r = 1; r < nFil; r++) scrollX[r] = 100000;
    ultimoRotuloBusca = (GfxRect){0};
    alvosResultadosTeste = 0; desenhaResultados(1200);
    perto(scrollY,toqueBuscaMaxY()); perto(scrollX[2],toqueBuscaMaxX(2));
    assert(alvosResultadosTeste > 0 && !recorteBuscaAtivo);
    assert(ultimoRotuloBusca.h > 0 && ultimoRotuloBusca.y + ultimoRotuloBusca.h <= NV_TELA_H - 48 + .01f);
    /* Recent-only and empty states use the same cropped viewport. */
    nFil = 0; nConsulta = 0; consulta[0] = 0; recentesBuscaTeste = BUSCASREC_MAX;
    scrollY = scrollAlvo = 0; alvosResultadosTeste = 0; desenhaResultados(1300);
    assert(nRecLayout == BUSCASREC_MAX+1 && alvosResultadosTeste > 0 && !recorteBuscaAtivo);
    e = (PonteiroRolagem){PONT_ROL_INICIO,1,0,0,BU_RES_X+50,BU_RES_Y+60};
    assert(toqueBuscaRolar(&e)); e.fase = PONT_ROL_MOVER; e.delta = -1e6f;
    toqueBuscaRolar(&e); perto(scrollY,toqueBuscaMaxY());
    alvosResultadosTeste = 0; desenhaResultados(1400); assert(alvosResultadosTeste > 0 && !recorteBuscaAtivo);
    recentesBuscaTeste = 0; nConsulta = 3; desenhaResultados(1500); assert(!recorteBuscaAtivo);
    resultadosBuscaTeste = 0;
  }
  nv_layout_w = 2400; nv_layout_h = 1080; buscaRailTeste = 0; imeBuscaTeste = 0;
  fonteLargaTeste = 0; recentesBuscaTeste = 0; nConsulta = 0; consulta[0] = 0;
  scrollY = scrollAlvo = 0; memset(scrollX,0,sizeof scrollX); memset(fil,0,sizeof fil); nFil = 0;
}
#endif

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
    assert(homeConteudoX() + homeTextoLargura(1200) + homeMargemDireita() <= NV_TELA_W);
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
      alvosHero = 0; alvoAcaoHero((GfxRect){homeConteudoX(), p.action, 240, 72}, 0, 0, 1);
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

static void testaHomeMargensRetrato(void) {
  const float alturas[] = {1920, 2340};
  const float rails[] = {0, 48, 113.6f};
  for (int h = 0; h < 2; h++) {
    nv_layout_w = 1080; nv_layout_h = alturas[h];
    for (int b = 0; b < 3; b++) {
      railTeste = rails[b];
      float margem = railTeste > 0 ? NV_CONTENT_PAD + railTeste : 48;
      perto(homeConteudoX(), margem); perto(homeMargemDireita(), margem);
      perto(homeConteudoX() + homeLarguraUtil() * 0.5f, NV_TELA_W * 0.5f);
      perto(homeTextoLargura(1200), homeLarguraUtil());
      for (int lay = 0; lay < 3; lay++) {
        layoutTeste = lay; toqueHomeLimpar(); scrollY = -empurraHero(); scrollX[0] = 0;
        fileiras[0].tipo = FILEIRA_NORMAL; fileiras[0].n = 12; fileiras[0].verTudo = 0; fileiras[0].escala = 1;
        perto(homeFileiraX(0, 0), margem);
        float fim = 11 * passoFil(0) + larguraFil(0);
        perto(toqueHomeMaxX(0), fim - homeLarguraUtil());
        scrollX[0] = toqueHomeMaxX(0);
        perto(homeFileiraX(0, 11) + larguraFil(0), homeConteudoDireita());
        alvosHero = 0;
        alvoAcaoHero((GfxRect){margem, 400, 240, 72}, 0, 0, 1);
        assert(alvosHero == 1); perto(alvoHero.x, margem);
        alvoAcaoHero((GfxRect){margem - 1, 400, 240, 72}, 0, 0, 1);
        alvoAcaoHero((GfxRect){homeConteudoDireita() - 200, 400, 240, 72}, 0, 0, 1);
        assert(alvosHero == 1);
        float cy = topoFileiras() - scrollY + NV_LEGACY_ROW_HEAD_H + 20;
        alvosCard = 0;
        alvoCard(margem - 20, cy, 120, 100, 0, 0);
        assert(alvosCard == 1); perto(alvoCartao.x, margem); perto(alvoCartao.w, 100);
        alvoCard(homeConteudoDireita() - 100, cy, 120, 100, 0, 0);
        assert(alvosCard == 2); perto(alvoCartao.x + alvoCartao.w, homeConteudoDireita());
        alvoCard(margem - 120, cy, 100, 100, 0, 0); assert(alvosCard == 2);
        PonteiroRolagem e = {PONT_ROL_INICIO, 1, 0, 0, margem - 1, 300};
        assert(!toqueHomeRolar(&e)); e.x = homeConteudoDireita(); assert(!toqueHomeRolar(&e));
        e.x = margem; assert(toqueHomeRolar(&e));
        e.fase = PONT_ROL_FIM; toqueHomeRolar(&e);
      }
    }
  }
  // Rotating to wide landscape restores its existing asymmetric pinned rail.
  nv_layout_w = 2400; nv_layout_h = 1080; layoutTeste = HOME_LAYOUT_MODERNA;
  railTeste = 113.6f;
  perto(homeConteudoX(), 217.6f); perto(homeMargemDireita(), NV_HOME_SAFE_RIGHT);
  perto(homeLarguraUtil(), NV_TELA_W - 217.6f - NV_HOME_SAFE_RIGHT);
  nv_layout_w = 1080; nv_layout_h = 1728; assert(!homeRetratoTelefone());
  perto(homeConteudoX(), 217.6f); perto(homeMargemDireita(), NV_HOME_SAFE_RIGHT);
  railTeste = 0; nv_layout_w = 2400; nv_layout_h = 1080;
  toqueHomeLimpar(); scrollY = scrollX[0] = 0;
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
  testaHomeMargensRetrato();
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
    for (int grande = 0; grande < 2; grande++) for (int ampliado = 0; ampliado < 2; ampliado++) {
      fonteBibLarga = grande; escalaBibTeste = ampliado ? 2.0f : 1.0f;
      contaModo[0] = contaModo[1] = 12345678;
      for (int m = 0; m < BIB_N_MODOS; m++) {
        modo = m; temAberta = 0; tipo = TIPO_SERIE; ordem = ORD_ANO; fonte = FONTE_FIXADAS;
        alvosBibTeste = textosBibTeste = 0;
        desenhaModos();
        for (int p = 0; p < 3; p++) {
          desenhaPicker(p, 0);
          perto(alvoBibTeste.y, pickerY());
          assert(alvoBibTeste.y >= BIB_FAIXA_Y + BIB_SEG_H + BIB_FAIXA_GAP);
        }
        assert(alvosBibTeste == 6 && textosBibTeste == 12);
        assert((pickerY() + BIB_CHIP_H) * bibHS() < gradeY() - BIB_CLIP_SOBE * bibHS());
        perto(gradeY(), (BIB_GRADE_Y + BIB_SEG_H + BIB_FAIXA_GAP) * bibHS());
        foco.fileira = BIB_FIL_MODO; foco.coluna = 0; pickSel = 0;
        ponteiroPicker(2, 0);
        assert(pickSel == 2 && foco.fileira == BIB_FIL_PICK && foco.coluna == 2);
        assert(colunaSobFaixa() >= 0);
        foco.nColunas[BIB_FIL_GRADE] = colunas();
        descerDaFaixa();
        assert(foco.fileira == BIB_FIL_GRADE && foco.coluna >= 0 && foco.coluna < colunas());
        /* List actions also retain all three accessible targets. */
        modo = MODO_LISTAS; temAberta = 1; alvosBibTeste = textosBibTeste = 0;
        desenhaAcoes(); assert(alvosBibTeste == 3 && textosBibTeste == 3);
        perto(gradeY(), BIB_GRADE_Y_ABERTA * bibHS());
      }
    }
  }
  fonteBibLarga = 0; escalaBibTeste = 1; temAberta = 0;
  nv_layout_w = 2400; nv_layout_h = 1080;
  perto(bibDireita(), NV_BIB_DIR); assert(colunasLista() == BIB_LC_COLS);
  perto(pickerY(), BIB_FAIXA_Y + 5); perto(gradeY(), BIB_GRADE_Y * bibHS());
  perto(pickerX(0), hdrX() + larguraSeletor() + BIB_FAIXA_GAP * 2 + 1);
#elif defined(TESTE_BUSCA)
  nv_layout_w = 1080; nv_layout_h = 2340;
  perto(BU_COL_W, 984);
  perto(buX(), 48); perto(buDir(), 1032);
  for (int modos = 0; modos < 8; modos++) for (int entrada = 0; entrada < 2; entrada++) {
    imeBuscaTeste = modos & 1; vozBuscaTeste = (modos >> 1) & 1; celBuscaTeste = (modos >> 2) & 1;
    fonteLargaTeste = entrada;
    nConsulta = entrada ? BU_MAX_CONSULTA - 1 : 0;
    memset(consulta, 'W', nConsulta); consulta[nConsulta] = 0;
    snprintf(avisoBuscaTeste, sizeof avisoBuscaTeste, "%s", "A long translated input hint must fit inside the search field");
    alvosCampo = textosCampo = recortesCampo = 0; cursorBuscaTeste = (GfxRect){0};
    painel = 0; campoFoco = 1; animFocoCampo = 1; pilulaBuscaTeste = entrada;
    desenhaCampo(0);
    perto(campoBuscaTeste.x + campoBuscaTeste.w*.5f, NV_TELA_W*.5f);
    assert(recortesCampo == 1 && !recorteBuscaAtivo && textosCampo == 1);
    assert(alvosCampo == imeBuscaTeste + vozBuscaTeste + celBuscaTeste);
    assert(cursorBuscaTeste.x >= campoBuscaTeste.x && cursorBuscaTeste.x + 2 < campoBuscaTeste.x + campoBuscaTeste.w);
  }
  buscaRailTeste = 140; desenhaCampo(0);
  perto(campoBuscaTeste.x + campoBuscaTeste.w*.5f, NV_TELA_W*.5f);
  buscaRailTeste = 0; fonteLargaTeste = 0; pilulaBuscaTeste = 0;
  imeBuscaTeste = 0;
  nv_layout_w = 1920; nv_layout_h = 1080; recortesCampo = 0;
  desenhaCampo(0); assert(!recortesCampo);
  perto(campoBuscaTeste.x, 96); perto(campoBuscaTeste.w, 520);
  nv_layout_w = 1080; nv_layout_h = 2340; desenhaCampo(0);
  perto(campoBuscaTeste.x, 48); perto(campoBuscaTeste.w, 984);
  consulta[0] = 0; nConsulta = 0; campoFoco = 0; avisoBuscaTeste[0] = 0;
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
  testaResultadosBuscaTelefone();
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
