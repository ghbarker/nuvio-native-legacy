/* Runs the callbacks and offsets of the real media consumers, without a
 * window, network, decoder or synthesized keyboard events. */
#define NV_TOUCH_UI 1
#define SDL_MAIN_HANDLED 1
#if defined(TESTE_DETAIL)
#include "../src/detail.c"
#elif defined(TESTE_EPISODIOS)
#include "../src/episodios.c"
#elif defined(TESTE_STREAMS)
#include "../src/streams.c"
#elif defined(TESTE_FAIXAS)
#include "../src/faixas.c"
#elif defined(TESTE_LEGENDAS)
#include "../src/legendasui.c"
#elif defined(TESTE_CENTRAL)
#include "../src/central.c"
#elif defined(TESTE_SALVOS)
#include "../src/salvospainel.c"
#elif defined(TESTE_AVISOS)
#include "../src/avisos.c"
#else
#error choose TESTE_* media consumer
#endif
#include "../src/ctxlista.h"
#include <assert.h>

float nv_layout_w = 2400.0f;
static int testMobile = 1;
int layout_modo_mobile(void) { return testMobile; }
float nv_layout_h = 1080.0f;
float gfx_escala_ui(void) { return 1.0f; }
void ctxhold_cancelar(CtxHold *h) { memset(h, 0, sizeof *h); }
int ctx_aberto(void) { return 0; }
#if defined(TESTE_DETAIL)
static int testeTrailerAberto, testeTrailerCheia, testeTrailerDono, testeTrailerWindows;
static GfxRect testeTrailerRect;
float trocaarte_visivel(void) { return 0; }
int trailer_cheia(void) { return testeTrailerCheia; }
int trailer_aberto(void) { return testeTrailerAberto; }
int trailer_dono(void) { return testeTrailerDono; }
void trailer_rect(GfxRect r) { testeTrailerRect = r; testeTrailerWindows++; }
int ajustes_home_layout(void) { return HOME_LAYOUT_DINAMICA; }
TxtLinha txt_linha(TxtEstilo estilo, const char *texto, int r, int g, int b, int a) {
  (void)estilo; (void)texto; (void)r; (void)g; (void)b; (void)a;
  return (TxtLinha){0, 1600, 32};
}
#endif
#if defined(TESTE_LEGENDAS)
int faixas_estilo_topo(void) { return 0; }
#endif
#if defined(TESTE_SALVOS)
static GfxRect veuTeste;
int reacao_painel_aberta(void) { return 0; }
int ctx_inline_painel_ativo(void) { return 0; }
int ajustes_vidro(void) { return 0; }
float gfx_escala(void) { return 1.0f; }
void gfx_escala_sair(float s) { (void)s; }
void gfx_cor(GfxRect r, float raio, float cr, float cg, float cb, float a) {
  (void)raio; (void)cr; (void)cg; (void)cb; (void)a; veuTeste = r;
}
#endif

static void perto(float a, float b) { assert(fabsf(a - b) < .001f); }
static void exercitar(ToqueRolagem *r, float *offset, PonteiroRolagemFn fn, int eixoY) {
  PonteiroRolagem e = {PONT_ROL_INICIO, eixoY, 0, 0, 220, 480};
  *offset = 0;
  toquerol_vincular(r, (GfxRect){100, 200, 300, 400}, 2, 0, 250, eixoY, offset);
  assert(fn(&e)); assert(r->livre);
  e.fase = PONT_ROL_MOVER; e.delta = -37;
  assert(fn(&e)); perto(*offset, 18.5f); /* fractional position, scaled layer */
  e.fase = PONT_ROL_SOLTAR; assert(fn(&e));
  e.fase = PONT_ROL_INERCIA; e.delta = -11;
  assert(fn(&e)); perto(*offset, 24);
  e.delta = -10000; fn(&e); perto(*offset, 250);
  assert(!fn(&e)); /* inertia stops on the last visible row */
  e.delta = 10000; fn(&e); perto(*offset, 0);
  assert(!fn(&e));
  e.fase = PONT_ROL_MOVER; e.delta = -37; fn(&e);
  e.fase = PONT_ROL_FIM; fn(&e); perto(*offset, 18.5f); assert(r->livre);
  e.fase = PONT_ROL_CANCELAR; fn(&e); perto(*offset, 18.5f); assert(r->livre);
  /* A redraw uses newly measured bounds, preserving the free position. */
  toquerol_vincular(r, (GfxRect){100, 200, 300, 400}, 2, 0, 10, eixoY, offset);
  perto(*offset, 10); assert(r->livre);
  toquerol_limpar(r); assert(!r->livre);
  e.fase = PONT_ROL_INICIO; e.x = 190; assert(!fn(&e));
  e.x = 220; e.eixoY = !eixoY; assert(!fn(&e));
  e.eixoY = eixoY;
  toquerol_vincular(r, (GfxRect){100, 200, 300, 400}, 2, 0, 0, eixoY, offset);
  assert(!fn(&e));
}

int main(void) {
#if defined(TESTE_DETAIL)
  aberto = 1; saindo = pessoaAberta = colListaAberta = 0;
  foco.fileira = foco.coluna = 0; pedAbrir = -1;
  exercitar(&toqueDetalhe, &scrollY, toqueDetalheRolar, 1);
  assert(pedAbrir == -1 && foco.fileira == 0 && foco.coluna == 0 && !okDesceEm);
  exercitar(&toqueSec[0], &scrollSec[0], toqueDetalheRolar, 0);
  assert(pedAbrir == -1 && foco.coluna == 0);
  colListaAberta = 1; exercitar(&toqueColecao, &colListaScroll, toqueColecaoRolar, 1);
  colListaAberta = 0; pessoaAberta = 1;
  exercitar(&toquePessoa, &toquePessoaOffset, toquePessoaRolar, 1);
  nv_layout_w = 1080; nv_layout_h = 2340; pg = scrollY = 0;
  assert(detalheRetrato() && !acoesAgrupadas());
  perto(larguraTextoHero(), 888); perto(larguraLogoHero(), 888); perto(larguraDetalhes(), 888);
  perto(corpoAlpha(.8f), .8f); assert(!corpoOculto(0) && !corpoOculto(1));
  float bx = 96, cy = 500;
  GfxRect ac[7];
  ac[0] = heroAcaoRect(&bx, &cy, larguraPrimario("long label"), NV_DETW2_BTN_H);
  bx += ac[0].w + NV_DETW2_BTN_GAP;
  ac[1] = heroAcaoRect(&bx, &cy, larguraSecundario("long label"), NV_DETW2_BTN_H);
  bx += ac[1].w + NV_DETW2_BTN_GAP;
  for (int i = 2; i < 7; i++) {
    ac[i] = heroAcaoRect(&bx, &cy, NV_DETW2_CIRC, NV_DETW2_CIRC);
    bx += ac[i].w + NV_DETW2_BTN_GAP;
  }
  for (int i = 0; i < 7; i++) {
    assert(ac[i].x >= 96 && ac[i].x + ac[i].w <= 984);
    assert(ac[i].y >= 464 && ac[i].y + ac[i].h <= 626);
    for (int j = 0; j < i; j++)
      assert(ac[i].x >= ac[j].x + ac[j].w || ac[i].y >= ac[j].y + ac[j].h);
  }
  GfxRect quote = frasesColuna(96, 1200, 0, 0);
  GfxRect facts = frasesColuna(96, 1200, 740, 1);
  perto(quote.w, 888); perto(facts.w, 888); perto(facts.x, 96); perto(facts.y, 1988);
  assert(pessoaColunas() == 2);
  for (int i = 0; i < 30; i++) {
    float x = PES_COL_X + i % pessoaColunas() * (PES_CARD_W + PES_CARD_GAP);
    assert(x >= PES_COL_X && x + PES_CARD_W <= 984);
  }
  GfxRect parts = colecaoListaRect();
  perto(parts.x, 96); perto(parts.y, 480); perto(parts.w, 888); perto(parts.h, 1800);
  float textW = parts.w - 13 - COLL_PO_W - 32 - 32;
  assert(textW > 600); /* current-part badge leaves positive title width */
  perto(colecaoTopo(), 480);
  trailerDetalheTela(); assert(!testeTrailerWindows);
  testeTrailerAberto = 1; testeTrailerDono = 0;
  trailerDetalheTela(); assert(testeTrailerWindows == 1);
  perto(testeTrailerRect.w, 1080); perto(testeTrailerRect.h, 2340);
  testeTrailerDono = TRAILER_DONO_HOME; trailerDetalheTela(); assert(testeTrailerWindows == 1);
  testeTrailerCheia = 1; trailerDetalheTela(); assert(testeTrailerWindows == 2);
  testeTrailerCheia = 0; testeTrailerDono = TRAILER_DONO_DETALHE;
  trailerDetalheTela(); assert(testeTrailerWindows == 3);
  testeTrailerAberto = 0;
  /* Redraw after playback rotation keeps the same document and free offset. */
  scrollY = 237.5f; toqueDetalhe.livre = 1; int oldIdx = idx;
  toquerol_vincular(&toqueDetalhe, (GfxRect){0,0,1080,2340}, 1, 0, 800, 1, &scrollY);
  nv_layout_w = 2340; nv_layout_h = 1080;
  toquerol_vincular(&toqueDetalhe, (GfxRect){0,0,2340,1080}, 1, 0, 2060, 1, &scrollY);
  assert(idx == oldIdx && toqueDetalhe.livre); perto(scrollY, 237.5f);
  assert(!detalheRetrato() && acoesAgrupadas());
  perto(larguraTextoHero(), 1040); perto(larguraLogoHero(), 1000); perto(larguraDetalhes(), 1040);
  pg = 0; scrollY = 0; perto(corpoAlpha(.8f), 0); assert(corpoOculto(0));
  quote = frasesColuna(96, 1200, 0, 0); facts = frasesColuna(96, 1200, 740, 1);
  perto(quote.w, 1040); perto(facts.x, 1232); perto(facts.y, 1200); perto(facts.w, 1012);
  assert(pessoaColunas() == 6); parts = colecaoListaRect();
  perto(parts.x, 620); perto(parts.y, 120); perto(parts.w, 1624); perto(parts.h, 900);
#elif defined(TESTE_EPISODIOS)
  aberto = 1; vmAberto = 0; pedidoE = 0; foco = 3; vmSegurando = 1; velScroll = 123;
  exercitar(&toqueEp, &scroll, toqueEpRolar, 1);
  assert(!pedidoE && foco == 3 && !vmSegurando && !velScroll);
  exercitar(&toqueEpTemporadas, &toqueEpTemporadasOffset, toqueEpRolar, 0);
  assert(!pedidoE && foco == 3);
#elif defined(TESTE_STREAMS)
  aberta = 1; foco = 3; escolha = -1; velRol = 123;
  exercitar(&toqueFontes, &rolagem, toqueFontesRolar, 1);
  assert(foco == 3 && escolha == -1 && !velRol);
  exercitar(&toqueFontesAbas, &toqueFontesAbasOffset, toqueFontesRolar, 0);
  assert(foco == 3 && escolha == -1);
#elif defined(TESTE_FAIXAS)
  aberta = 1; modo = 0; coluna = 0; foco[0] = 3;
  exercitar(&toqueAudio, &toqueAudioOffset, toqueAudioRolar, 1);
  assert(foco[0] == 3 && aberta && coluna == 0);
#elif defined(TESTE_LEGENDAS)
  aberto = 1; foco = 3; mais = ver = 0;
  exercitar(&toqueLegendas, &toqueLegendasOffset, toqueLegendasRolar, 1);
  assert(foco == 3 && aberto && !mais && !ver);
  perto(larguraSecundaria(), 2280); nv_layout_w = 1920;
  perto(larguraSecundaria(), 1800); nv_layout_w = 2400;
#elif defined(TESTE_CENTRAL)
  aberta = editando = 1; focoEd = 3;
  exercitar(&toqueCentral, &rolaEd, toqueCentralRolar, 1);
  assert(focoEd == 3 && aberta && editando);
#elif defined(TESTE_SALVOS)
  aberto = 1; pop = tecladoPara = editando = 0; foco = 3; okDesde = 123; velY = 123;
  exercitar(&toquePainel, &scrollY, toquePainelRolar, 1);
  assert(foco == 3 && !okDesde && !velY && aberto);
  pop = POP_CATS; popFoco = 3;
  exercitar(&toquePop, &popRol, toquePopRolar, 1);
  assert(pop == POP_CATS && popFoco == 3);
  pop = 0; editando = 1; editLin = 3;
  exercitar(&toqueEditar, &toqueEditarOffset, toqueEditarRolar, 1);
  assert(editando && editLin == 3);
  entrada = .5f; veuInteiro(); perto(veuTeste.w, 2400); perto(veuTeste.h, 1080);
#elif defined(TESTE_AVISOS)
  aberto = 1; cartao = 0; foco = 3; okDesde = 123;
  exercitar(&toqueAvisos, &rol, toqueAvisosRolar, 1);
  assert(aberto && foco == 3 && !okDesde && !pediuCodigo);
#endif
  { SDL_Event e = {0}; e.type = SDL_KEYDOWN; e.key.keysym.sym = SDLK_RETURN;
    assert(!toquerol_navegacao(&e)); e.key.keysym.sym = SDLK_DOWN; assert(toquerol_navegacao(&e)); }
  puts("media_toque: scaled fractional drag, release, inertia and limits PASS");
  return 0;
}
