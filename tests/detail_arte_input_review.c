/* Real artwork state, Detail event route and pointer empty-layer fallback.
 * No GL window/provider/playback is opened. The empty late-animation layer
 * models trocaarte_desenhar's a <= .3 target gate, separately from pixels. */
#define main reviewDetailBodyFixtureMain
#include "detail_arte_body.c"
#undef main

static void reviewEmptyArtworkFrame(void) {
  ponteiro_quadro(SDL_GetTicks());
  ponteiro_camada();
  ponteiro_desenhar();
}
static void reviewTap(Uint32 type) {
  SDL_Event e; SDL_zero(e); e.type = type;
  e.tfinger.touchId = 3; e.tfinger.fingerId = 1;
  e.tfinger.x = .5f; e.tfinger.y = .5f;
  assert(ponteiro_evento(&e, detail_evento));
}
int main(void) {
  const int screens[][2] = {{1080,1920},{1080,2340},{2340,1080},{2520,1080}};
  const float zooms[] = {1,1.2f,1.3f,1.5f};
  CatItem c = {0};
  assert(SDL_Init(SDL_INIT_TIMER) == 0); SDL_Delay(2);
  assert(SDL_GetTicks() > 0);
  snprintf(c.imdb,sizeof c.imdb,"ensaio:arte-input");
  snprintf(c.tipo,sizeof c.tipo,"movie");
  snprintf(c.titulo,sizeof c.titulo,"Artwork input isolation fixture");
  cat_definir_tudo(&c,1,NULL,0); idx=0;
  for(int s=0;s<4;s++)for(int z=0;z<4;z++) {
    layout_tela_definir(screens[s][0],screens[s][1]); gfx_escala_ui_definir(zooms[z]);
    aberto=1;saindo=pessoaAberta=colListaAberta=focoAmigos=carro=0;
    nivel=botao=0;okDesceEm=0;pedReproduzir=pedFontes=pedMarcar=0;
    ponteiro_iniciar();ponteiro_teste_toque(1);
    ponteiro_teste_janela(screens[s][0],screens[s][1]);
    trocaarte_abrir(&c);trocaarte_atualizar(1);
    trocaarte_fechar();trocaarte_atualizar(.11f);
    assert(!trocaarte_aberto() && trocaarte_visivel() > .005f && trocaarte_visivel() <= .3f);
    assert(corpoSobArte());reviewEmptyArtworkFrame();
    reviewTap(SDL_FINGERDOWN);reviewEmptyArtworkFrame();reviewTap(SDL_FINGERUP);
    assert(!pedReproduzir&&!pedFontes&&!pedMarcar&&!okDesceEm);
    trocaarte_atualizar(1);assert(!corpoSobArte());
  }
  SDL_Quit();
  puts("Detail artwork input: 16 closing-animation empty-layer taps cannot activate the hidden body PASS");
}
