/* Production player targets cross the real pointer DOWN/redraw/UP route.
 * Drawing, SDL host services and media are inert; input identity is real. */
#define ponteiro_alvo reviewCapturarAlvo
#define ponteiro_camada reviewCapturarCamada
#define ponteiro_rolagem reviewCapturarRolagem
#define NV_AOVIVO_DOUBLES_ONLY 1
#include "aovivo_phone_toque.c"
#undef ponteiro_alvo
#undef ponteiro_camada
#undef ponteiro_rolagem
void ponteiro_alvo(float, float, float, float, PonteiroFn, PonteiroFn, int, int);
void ponteiro_camada(void);
void ponteiro_rolagem(PonteiroRolagemFn);
#define SDL_GetTicks reviewUnusedTicks
#include "ponteiro_sdl.h"
#undef SDL_GetTicks
float gfx_opacidade_grupo = 1;
/* The pointer operates on base logical coordinates outside the scaled UI. */
#undef NV_TELA_W
#undef NV_TELA_H
#define NV_TELA_W nv_layout_w
#define NV_TELA_H nv_layout_h
#define visivel reviewPointerVisivel
#include "../src/ponteiro.c"
#undef visivel

static Uint32 reviewNow = 5000;
static int reviewKeys;
static Uint32 reviewClock(void) { return reviewNow; }
static void reviewDeliver(const SDL_Event *e) { (void)e; reviewKeys++; }
enum { REVIEW_AV, REVIEW_AV_ERROR, REVIEW_VOD_ERROR, REVIEW_MINI };
static void reviewFrame(int mode) {
  testScale = 1;
  ponteiro_quadro(reviewNow);
  testScale = mode == REVIEW_MINI ? 1 : testUI;
  nTargets = layers = 0; scrolling = NULL; testClipOn = 0;
  if (mode == REVIEW_AV) {
    AoVivoOsd o = {0}; o.nBotoes = avBotoes(o.botoes);
    for (int i = 0; i < o.nBotoes; i++) if (o.botoes[i] == AV_B_FONTE) o.foco = i;
    avToquePreparar(0); fileiraBotoes(&o, nv_layout_h / testUI - 130, 1);
  } else if (mode == REVIEW_AV_ERROR) {
    avToquePreparar(1); aovivo_erro_desenharCorpo_("Channel", NULL, "Error", "Choose a source", 1);
  } else if (mode == REVIEW_VOD_ERROR) {
    corpoErro((GfxRect){96, 116, erroLargura(), alturaErro()}, 1, NULL);
  } else player_mini_desenhar(reviewNow);
  /* Preserve the exact production callback/arguments and layer ordering. */
  if (layers) ponteiro_camada();
  for (int i = 0; i < nTargets; i++) {
    const PonteiroAlvo *t = &targets[i];
    ponteiro_alvo(t->x, t->y, t->w, t->h, t->focar, t->ativar, t->a, t->b);
  }
  if (scrolling) ponteiro_rolagem(scrolling);
  testScale = 1;
  ponteiro_desenhar(); reviewNow += 16;
}
static PonteiroAlvo reviewTarget(int action) {
  const PonteiroAlvo *v; int n = ponteiro_teste_lista(&v);
  for (int i = n - 1; i >= 0; i--) if (v[i].ativar && v[i].a == action) return v[i];
  assert(!"production target missing"); return (PonteiroAlvo){0};
}
static void reviewEvent(Uint32 type, const PonteiroAlvo *target, int mouse) {
  SDL_Event e; SDL_zero(e);
  float x = target->x + target->w * .5f, y = target->y + target->h * .5f;
  e.type = type;
  if (mouse) { e.button.button = SDL_BUTTON_LEFT; e.button.x = (int)x; e.button.y = (int)y; }
  else { e.tfinger.touchId = 3; e.tfinger.fingerId = 1; e.tfinger.x = x / nv_layout_w; e.tfinger.y = y / nv_layout_h; }
  assert(ponteiro_evento(&e, reviewDeliver));
}
static void reviewStart(int mode) {
  ponteiro_iniciar(); ponteiro_teste_toque(1); ponteiro_teste_relogio(reviewClock);
  ponteiro_teste_janela((int)nv_layout_w, (int)nv_layout_h); reviewKeys = 0;
  if (mode == REVIEW_VOD_ERROR) resetError();
  else if (mode == REVIEW_MINI) abrir();
  else channel(mode == REVIEW_AV_ERROR);
  reviewFrame(mode);
}
int main(void) {
  const float screens[][2] = {{1080, 1920}, {1080, 2340}, {2340, 1080}, {2520, 1080}};
  const float zooms[] = {1, 1.2f, 1.3f, 1.5f};
  int cases = 0;
  for (int s = 0; s < 4; s++) for (int z = 0; z < 4; z++) {
  nv_layout_w = screens[s][0]; nv_layout_h = screens[s][1]; testScale = 1; testUI = zooms[z];
  for (int mouse = 0; mouse < 2; mouse++) {
    for (int mode = REVIEW_AV; mode <= REVIEW_VOD_ERROR; mode++) {
      reviewStart(mode);
      PonteiroAlvo t = reviewTarget(mode == REVIEW_VOD_ERROR ? 0 : AV_B_FONTE);
      reviewEvent(mouse ? SDL_MOUSEBUTTONDOWN : SDL_FINGERDOWN, &t, mouse);
      reviewFrame(mode); reviewFrame(mode);
      reviewEvent(mouse ? SDL_MOUSEBUTTONUP : SDL_FINGERUP, &t, mouse);
      assert(pedFontes && !reviewKeys);
      cases++;
    }
    reviewStart(REVIEW_MINI);
    PonteiroAlvo t = reviewTarget(0);
    /* Both Mini callbacks use a=0; choose the full island, outside Close. */
    const PonteiroAlvo *v; assert(ponteiro_teste_lista(&v) == 2); t = v[0];
    reviewEvent(mouse ? SDL_MOUSEBUTTONDOWN : SDL_FINGERDOWN, &t, mouse);
    reviewFrame(REVIEW_MINI); reviewFrame(REVIEW_MINI);
    reviewEvent(mouse ? SDL_MOUSEBUTTONUP : SDL_FINGERUP, &t, mouse);
    assert(!mini && aberto && !reviewKeys);
    cases++;
  }
  for (int mode = REVIEW_AV; mode <= REVIEW_AV_ERROR; mode++) {
    reviewStart(mode); PonteiroAlvo t = reviewTarget(AV_B_FONTE);
    reviewEvent(SDL_FINGERDOWN, &t, 0); permitido = 0; reviewFrame(mode);
    reviewEvent(SDL_FINGERUP, &t, 0); assert(!pedFontes);
    reviewStart(mode); t = reviewTarget(AV_B_FONTE);
    reviewEvent(SDL_FINGERDOWN, &t, 0);
    strcpy(itemCanal.imdb, "another-channel"); reviewFrame(mode);
    reviewEvent(SDL_FINGERUP, &t, 0); assert(!pedFontes);
    reviewStart(mode); t = reviewTarget(AV_B_FONTE);
    reviewEvent(SDL_FINGERDOWN, &t, 0);
    permitido = 0; reviewFrame(mode); permitido = 1; reviewFrame(mode);
    reviewEvent(SDL_FINGERUP, &t, 0); assert(!pedFontes);
    reviewStart(mode); t = reviewTarget(AV_B_FONTE);
    reviewEvent(SDL_FINGERDOWN, &t, 0);
    SDL_Event resize; SDL_zero(resize); resize.type = SDL_WINDOWEVENT;
    resize.window.event = SDL_WINDOWEVENT_SIZE_CHANGED;
    ponteiro_evento(&resize, reviewDeliver);
    nv_layout_w = screens[(s + 1) % 4][0]; nv_layout_h = screens[(s + 1) % 4][1];
    ponteiro_teste_janela((int)nv_layout_w, (int)nv_layout_h); reviewFrame(mode);
    reviewEvent(SDL_FINGERUP, &t, 0); assert(!pedFontes);
    nv_layout_w = screens[s][0]; nv_layout_h = screens[s][1];
  }
  }
  printf("player_touch_frames_review: %d real finger/mouse taps across redraw, AV/VOD/Mini, channel/modal/rotation rejection PASS\n", cases);
}
