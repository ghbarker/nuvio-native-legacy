/* Runs the callbacks and offsets of the real media consumers, without a
 * window, network, decoder or synthesized keyboard events. */
#define NV_TOUCH_PREVIEW 1
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
float gfx_escala_ui(void) { return 1.0f; }
void ctxhold_cancelar(CtxHold *h) { memset(h, 0, sizeof *h); }
int ctx_aberto(void) { return 0; }
#if defined(TESTE_DETAIL)
int trailer_cheia(void) { return 0; }
#endif
#if defined(TESTE_LEGENDAS)
int faixas_estilo_topo(void) { return 0; }
#endif
#if defined(TESTE_SALVOS)
static GfxRect veuTeste;
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
