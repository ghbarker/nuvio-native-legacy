/* Callbacks reais das telas auxiliares, sem desenho ou servicos externos. */
#ifdef _WIN32
#include <time.h>
static struct tm *utilidades_localtime_r(const time_t *t, struct tm *out) {
  struct tm *r = localtime(t);
  if (r) *out = *r;
  return r ? out : NULL;
}
#define localtime_r utilidades_localtime_r
#endif
#define NV_TOUCH_PREVIEW 1
#define SDL_MAIN_HANDLED 1
#if defined(TESTE_SPOTLIGHT)
#include "../src/spotlight.c"
#elif defined(TESTE_EXPLORAR)
#include "../src/explorar.c"
#elif defined(TESTE_AGENDA)
#include "../src/agendaui.c"
#elif defined(TESTE_NOTAS)
#include "../src/atualizacao.c"
#elif defined(TESTE_DIAGNOSTICO)
#include "../src/diagnostico.c"
#elif defined(TESTE_LIVETV)
#include "../src/livetvdiag.c"
#elif defined(TESTE_REGISTRO)
#include "../src/registro.c"
#else
#error escolha TESTE_* utilidade
#endif
#include <assert.h>

float nv_layout_w = 2400.0f;
#if defined(TESTE_AGENDA)
static Noticia manchetesTeste[8];
static float escalaAgendaTeste = 1.5f;
static PonteiroRolagemFn rolagemAgendaTeste;
float gfx_escala(void) { return escalaAgendaTeste; }
void ponteiro_rolagem(PonteiroRolagemFn fn) { rolagemAgendaTeste = fn; }
const Noticia *noticias_item(const char *imdb, int i) { (void)imdb; return i >= 0 && i < 8 ? &manchetesTeste[i] : NULL; }
#endif
#if defined(TESTE_EXPLORAR)
static PonteiroAlvo ultimoAlvo;
static int alvos;
void ponteiro_alvo(float x, float y, float w, float h, PonteiroFn focar, PonteiroFn ativar, int a, int b) {
  ultimoAlvo = (PonteiroAlvo){x, y, w, h, focar, ativar, a, b, 0, NULL};
  alvos++;
}
#endif
static void perto(float a, float b) { assert(fabsf(a - b) < .001f); }
static void exercitar(ToqueRolagem *r, float *offset, PonteiroRolagemFn fn, int eixoY) {
  PonteiroRolagem e = {PONT_ROL_INICIO, eixoY, 0, 0, 220, 480};
  *offset = 0;
  toquerol_vincular(r, (GfxRect){100, 200, 300, 400}, 2, 0, 250, eixoY, offset);
  assert(fn(&e) && r->livre);
  e.fase = PONT_ROL_MOVER; e.delta = -37;
  assert(fn(&e)); perto(*offset, 18.5f);
  e.fase = PONT_ROL_SOLTAR; assert(fn(&e));
  e.fase = PONT_ROL_INERCIA; e.delta = -11;
  assert(fn(&e)); perto(*offset, 24);
  e.delta = -10000; fn(&e); perto(*offset, 250);
  assert(!fn(&e));
  e.delta = 10000; fn(&e); perto(*offset, 0);
  assert(!fn(&e));
  e.fase = PONT_ROL_MOVER; e.delta = -37; fn(&e);
  e.fase = PONT_ROL_FIM; fn(&e); perto(*offset, 18.5f); assert(r->livre);
  e.fase = PONT_ROL_CANCELAR; fn(&e); perto(*offset, 18.5f);
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
#if defined(TESTE_SPOTLIGHT)
  focoL = 3; temPedido = 0; okPress = okLongo = 1; velY = 123;
  exercitar(&toqueSpot, &scrollY, toqueSpotRolar, 1);
  assert(focoL == 3 && !temPedido && !okPress && !okLongo && !velY);
  /* O callback mantem o destino da mola junto com o deslocamento direto. */
  perto(scrollAlvo, 18.5f);
#elif defined(TESTE_EXPLORAR)
  modo = MODO_CLIMA; caLinha = 0; caCol[0] = 3; caCol[1] = 2; pediuAbrir = 0;
  exercitar(&toqueCa[0], &caRolar[0], toqueExplorarRolar, 0);
  assert(caLinha == 0 && caCol[0] == 3 && caCol[1] == 2 && !pediuAbrir);
  exercitar(&toqueCa[1], &caRolar[1], toqueExplorarRolar, 0);
  assert(toqueCaAtiva == -1 && !pediuAbrir);
  modo = MODO_VIZ; vzLinha = 0; vzCol = 3;
  exercitar(&toqueVz[2], &toqueVzOffset[2], toqueExplorarRolar, 0);
  exercitar(&toqueVz[3], &toqueVzOffset[3], toqueExplorarRolar, 0);
  assert(toqueVzAtiva == -1 && vzLinha == 0 && vzCol == 3 && !pediuAbrir);
  /* O quinto vizinho cabe depois de arrastar, sem levar o foco junto. */
  memset(&viz, 0, sizeof viz); memset(toqueVz, 0, sizeof toqueVz);
  memset(toqueVzOffset, 0, sizeof toqueVzOffset);
  viz.g[2].n = MAPA_VIZ_ITENS; vzLinha = 0; vzCol = 4;
  toqueVz[2].livre = 1; toqueVzOffset[2] = 37.5f;
  perto(vizRolarFileira(2, 0, 1000), 278);
  perto(toqueVzOffset[2], 37.5f); assert(vzCol == 4);
  toquerol_limpar(&toqueVz[2]);
  vizRolarFileira(2, 0, 1000); perto(toqueVzOffset[2], 278);
  /* As zonas tocaveis acompanham o recorte nos dois extremos. */
  explorarAlvoFileira(90, 200, 246, 146, 100, 1100, ponteiroViz, 0, 4);
  perto(ultimoAlvo.x, 100); perto(ultimoAlvo.w, 236);
  toqueVz[2].livre = 1; vzCol = 0; ultimoAlvo.focar(ultimoAlvo.a, ultimoAlvo.b);
  assert(vzCol == 4 && !toqueVz[2].livre && !pediuAbrir);
  explorarAlvoFileira(1000, 200, 246, 146, 100, 1100, ponteiroViz, 0, 4);
  perto(ultimoAlvo.x, 1000); perto(ultimoAlvo.w, 100);
  int antes = alvos;
  explorarAlvoFileira(1200, 200, 246, 146, 100, 1100, ponteiroViz, 0, 4);
  assert(alvos == antes);
  nv_layout_w = 1920; perto(EX_DIR, 1840);
  nv_layout_w = 2400; perto(EX_DIR, 2320);
#elif defined(TESTE_AGENDA)
  foco = 3; calEvento = 2; ctxAberto = 0; pediuAbrir[0] = 0;
  exercitar(&toqueAgenda, &scrollY, toqueAgendaRolar, 1);
  ctxAberto = 3;
  exercitar(&toqueNoticia, &notRol, toqueAgendaRolar, 1);
  ctxAberto = 0;
  exercitar(&toqueCalendario, &toqueCalOffset, toqueAgendaRolar, 1);
  assert(!toqueAgAtiva && foco == 3 && calEvento == 2 && !ctxAberto && !pediuAbrir[0]);
  {
    AgItem it = {0};
    GfxRect janela = {100, 200, 300, 300};
    PonteiroRolagem e = {PONT_ROL_INICIO, 1, 0, 0, 220, 450};
    snprintf(it.imdb, sizeof it.imdb, "tt123");
    for (int i = 0; i < 8; i++) snprintf(manchetesTeste[i].titulo, sizeof manchetesTeste[i].titulo, "Headline %d", i);
    notFoco = 3; ctxAberto = 2;
    toqueManchetesReiniciar();
    perto(toqueMancheteY(3), 3 * AGN_LINHA);
    perto(toqueMancheteY(4), 3 * AGN_LINHA + AGN_FOCO);
    perto(toqueMancheteY(8), 7 * AGN_LINHA + AGN_FOCO);
    perto(toqueManchetePreparar(&it, 8, janela, 0, 1), 0);
    assert(rolagemAgendaTeste == toqueAgendaRolar);
    perto(toqueManchetes.maximo, 7 * AGN_LINHA + AGN_FOCO - janela.h);
    assert(toqueAgendaRolar(&e) && toqueAgAtiva == &toqueManchetes);
    e.fase = PONT_ROL_MOVER; e.delta = -37.5f;
    assert(toqueAgendaRolar(&e)); perto(toqueManchetesOffset, 25);
    assert(notFoco == 3 && !pediuAbrir[0] && ctxAberto == 2);
    /* Focus-window calculations cannot pull a captured list back. */
    perto(toqueManchetePreparar(&it, 8, janela, 3, 1), 25);
    e.fase = PONT_ROL_INERCIA; e.delta = -1e6f;
    toqueAgendaRolar(&e); perto(toqueManchetesOffset, toqueManchetes.maximo);
    assert(!toqueAgendaRolar(&e));
    assert(toqueMancheteIndice(3 * AGN_LINHA + 10, 8) == 3);
    assert(toqueMancheteIndice(3 * AGN_LINHA + AGN_FOCO + 1, 8) == 4);
    /* A refreshed headline with the same list size resets the offset. */
    snprintf(manchetesTeste[0].titulo, sizeof manchetesTeste[0].titulo, "New headline");
    perto(toqueManchetePreparar(&it, 8, janela, 1, 1), AGN_LINHA);
    assert(!toqueManchetes.livre && !toqueAgAtiva);
    e.fase = PONT_ROL_INICIO; assert(toqueAgendaRolar(&e));
    e.fase = PONT_ROL_MOVER; e.delta = -75; toqueAgendaRolar(&e);
    e.fase = PONT_ROL_CANCELAR; toqueAgendaRolar(&e);
    assert(!toqueAgAtiva && notFoco == 3);
    /* A modal never captures its still-drawn background list. */
    toquerol_vincular(&toqueAgenda, (GfxRect){0, 0, 100, 100}, 1, 0, 300, 1, &scrollY);
    e.fase = PONT_ROL_INICIO; e.x = e.y = 30; assert(!toqueAgendaRolar(&e));
    ctxAberto = 1; e.x = 220; e.y = 450; assert(!toqueAgendaRolar(&e));
    ctxAberto = 2;
    toquerol_limpar(&toqueManchetes);
    perto(toqueManchetePreparar(&it, 8, janela, 2, 1), 2 * AGN_LINHA);
    toqueManchetesReiniciar(); assert(!toqueManchetes.livre);
    perto(toqueManchetesOffset, 0);
  }
#elif defined(TESTE_NOTAS)
  foco = 1;
  exercitar(&toqueNotas, &rolar, toqueNotasRolar, 1);
  assert(foco == 1); perto(rolarAlvo, 18.5f);
#elif defined(TESTE_DIAGNOSTICO)
  focoModo = 1; focoLinha = 3; vz.rolagem = 2; vz.botao = 1;
  exercitar(&toqueRanking, &toqueRankingOffset, toqueRankingRolar, 1);
  assert(focoModo == 1 && focoLinha == 3 && vz.rolagem == 2 && vz.botao == 1);
#elif defined(TESTE_LIVETV)
  L.estado = E_PRONTO; L.botao = 1; L.atual = 2;
  exercitar(&toqueLtd, &toqueLtdOffset, toqueLtdRolar, 1);
  assert(L.estado == E_PRONTO && L.botao == 1 && L.atual == 2 && !L.aplicado);
#elif defined(TESTE_REGISTRO)
  foco = 3; focoEtapa = 2; marcaLida = 1234; pausado = 0;
  exercitar(&toqueRegistro, &toqueRegistroOffset, toqueRegistroRolar, 1);
  assert(pausado && marcaPausa == marcaLida && foco == 3 && focoEtapa == 2);
  exercitar(&toqueEtapas, &toqueEtapasOffset, toqueRegistroRolar, 1);
  assert(foco == 3 && focoEtapa == 2 && !detalhe && !focoEnviar);
#endif
  puts("utilidades_toque: OK");
  return 0;
}
