/* Real Detail section entries and callbacks under the real artwork picker.
 * Linked with production modules; no GL window, provider request or playback.
 * The blocked renderer must return before consulting/drawing any section. */
#define NV_TOUCH_PREVIEW 1
#define SDL_MAIN_HANDLED 1
#include "../src/detail.c"
#include <assert.h>

static void bloqueado(void) {
  const PonteiroAlvo *alvos;
  PonteiroRolagem e = {PONT_ROL_INICIO, 1, 0, 0, 100, 200};
  assert(corpoSobArte());
  nivel = 1; foco.fileira = SEC_ELENCO; foco.coluna = 2;
  foco.colunaLembrada[SEC_ELENCO] = 2; scrollY = 123;
  toqueSecAtiva = -1; okDesceEm = 99;
  toquerol_vincular(&toqueDetalhe, (GfxRect){0, 0, NV_TELA_W, NV_TELA_H},
                   gfx_escala(), 0, 1000, 1, &scrollY);
  ponteiroDetalhe(-1, DET_PTR_AMIGOS);
  ponteiroDetalhe(SEC_ELENCO, 0);
  assert(nivel == 1 && foco.fileira == SEC_ELENCO && foco.coluna == 2);
  assert(foco.colunaLembrada[SEC_ELENCO] == 2 && !focoAmigos);
  assert(!toqueDetalheRolar(&e));
  e.fase = PONT_ROL_MOVER; e.delta = -50;
  assert(!toqueDetalheRolar(&e));
  assert(scrollY == 123 && okDesceEm == 99 && !toqueDetalhe.livre);
  ponteiro_quadro(SDL_GetTicks());
  gfx_n_rect = 0;
  for (int r = 0; r < N_SECOES; r++) desenhaSecao(r, 1, SDL_GetTicks());
  ponteiro_desenhar();
  assert(gfx_n_rect == 0 && ponteiro_teste_lista(&alvos) == 0);
}

int main(void) {
  static const int telas[][2] = {{1080,1920}, {1080,2340}, {2340,1080}, {2520,1080}};
  static const float zoom[] = {1, 1.2f, 1.3f, 1.5f};
  CatItem c = {0};
  assert(SDL_Init(SDL_INIT_TIMER) == 0);
  snprintf(c.imdb, sizeof c.imdb, "ensaio:arte-corpo");
  snprintf(c.tipo, sizeof c.tipo, "movie");
  snprintf(c.titulo, sizeof c.titulo, "Artwork body isolation fixture");
  cat_definir_tudo(&c, 1, NULL, 0); idx = 0;
  aberto = 1; saindo = pessoaAberta = colListaAberta = focoAmigos = 0;
  ponteiro_iniciar(); ponteiro_teste_toque(1);
  for (unsigned i = 0; i < sizeof telas / sizeof *telas; i++) {
    layout_tela_definir(telas[i][0], telas[i][1]);
    for (unsigned j = 0; j < sizeof zoom / sizeof *zoom; j++) {
      gfx_escala_ui_definir(zoom[j]);
      trocaarte_abrir(&c); assert(trocaarte_aberto());
      trocaarte_atualizar(.002f); bloqueado(); /* entering: visible .014 */
      trocaarte_atualizar(1); bloqueado(); /* settled */
      trocaarte_fechar(); assert(!trocaarte_aberto());
      trocaarte_atualizar(.1f); bloqueado(); /* closing .3: still blocks */
      trocaarte_atualizar(1); assert(!corpoSobArte());
      toquerol_limpar(&toqueDetalhe);
    }
  }
  trocaarte_abrir(&c); trocaarte_atualizar(1);
  layout_tela_definir(1920, 1080); assert(!corpoSobArte());
  layout_tela_definir(2560, 1600); assert(!corpoSobArte());
  trocaarte_fechar(); trocaarte_atualizar(1);
  SDL_Quit();
  puts("Detail artwork body: 48 actual section/callback isolation states PASS");
  return 0;
}
