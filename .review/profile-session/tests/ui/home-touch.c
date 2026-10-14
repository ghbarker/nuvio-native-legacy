#define NV_TOUCH_UI 1
#define NV_ANDROID 1
#include "common.h"
#include "ajustes.h"
int ajustes_hero_ligado(void) { return 0; }
int ajustes_home_layout(void) { return HOME_LAYOUT_MODERNA; }
int ajustes_posteres_deitados(void) { return 0; }
int ajustes_rotulos_poster(void) { return 0; }
int ajustes_largura_poster_dp(void) { return 126; }
float ajustes_espaco_fileiras(void) { return 1; }
float ajustes_espaco_titulos(void) { return 1; }
float ajustes_conteudo_x(void) { return 120; }
float ajustes_rail_largura_fixa(void) { return 0; }
int ajustes_borda_foco(void) { return 0; }
int ajustes_ocultar_filmes_vistos(void) { return 0; }
#include "home.c"
#include "ponteiro.c"
static Uint32 clockNow = 1000;
static int assertionsFailed, keys, keyUps, arrows, taps, oldMouseActions, profileTapActions;
static int switchOnReturn, oldBorderActions;
static void boundary(int profile);
#define CHECK(c, why) do { if (!(c)) { fprintf(stderr, "FAIL touch session: %s (line %d)\n", why, __LINE__); assertionsFailed++; } } while (0)
static Uint32 testClock(void) { return clockNow; }
unsigned cat_revisao(void) { return 1; }
unsigned cat_revisao_itens(void) { return 1; }
int cat_visto(const CatItem *item) { (void)item; return 0; }
int trailer_aberto(void) { return 0; }
int trailer_dono(void) { return TRAILER_DONO_HOME; }
void trailer_fechar(void) {}
int cw_retido_definir(const char *s) { (void)s; return 1; }
int amigosfil_rolagem(const PonteiroRolagem *e) { (void)e; return 0; }
int amigosfil_indice_cat(int i) { return i; }
int cat_n(void) { return 0; }
const CatItem *cat_item(int i) { (void)i; return NULL; }
int cat_n_fileiras(void) { return 0; }
const CatFileira *cat_fileira(int i) { (void)i; return NULL; }
float fil_tipo_fator(int type) { (void)type; return 1; }
unsigned fil_revisao(void) { return 1; }
const char *fil_hero_fonte(void) { return ""; }
void desc_sinopse_hero(const int *indices, int n) { (void)indices; (void)n; }
float gfx_escala(void) { return 1; }
float gfx_opacidade_grupo = 1;
void gfx_sem_recorte(void) {}
void gfx_cor(GfxRect r, float radius, float a, float b, float c, float d) { (void)r; (void)radius; (void)a; (void)b; (void)c; (void)d; }
int SDL_GetNumTouchDevices(void) { return 0; }
SDL_Window *SDL_GL_GetCurrentWindow(void) { return NULL; }
SDL_Window *SDL_GetWindowFromID(Uint32 id) { (void)id; return NULL; }
SDL_Window *SDL_GetMouseFocus(void) { return NULL; }
void SDL_GetWindowSize(SDL_Window *w, int *x, int *y) { (void)w; *x = 1920; *y = 1080; }
void SDL_GL_GetDrawableSize(SDL_Window *w, int *x, int *y) { SDL_GetWindowSize(w, x, y); }
Uint32 SDL_GetMouseState(int *x, int *y) { *x = *y = 0; return 0; }
static void deliver(const SDL_Event *e) {
  if (e->type == SDL_KEYUP) { keyUps++; return; }
  if (e->type != SDL_KEYDOWN) return;
  keys++;
  if (e->key.keysym.sym == SDLK_UP || e->key.keysym.sym == SDLK_DOWN) arrows++;
  if (e->key.keysym.sym == SDLK_RETURN) {
    if (switchOnReturn) { switchOnReturn = 0; profileTapActions++; boundary(2); }
    else { taps++; home_pedir_abrir(); }
  }
}
static void populateHome(void) {
  nFileiras = 16; foco.nFileiras = 16;
  for (int i = 0; i < nFileiras; i++) {
    fileiras[i].tipo = FILEIRA_SERVICO; fileiras[i].n = 8;
    foco.nColunas[i] = 8;
  }
}
static void frame(void) {
  clockNow += 16; ponteiro_quadro(clockNow);
  ponteiro_alvo(300, 600, 400, 300, ponteiroCard, NULL, 0, 0);
  ponteiro_rolagem(toqueHomeRolar);
  ponteiro_desenhar();
}
static void frameTarget(PonteiroFn focus, PonteiroFn activate) {
  clockNow += 16; ponteiro_quadro(clockNow);
  ponteiro_alvo(300, 600, 400, 300, focus, activate, 0, 0);
  ponteiro_desenhar();
}
static void mouse(Uint32 kind, Uint8 button) {
  SDL_Event e; SDL_zero(e); e.type = kind; e.button.button = button;
  e.button.x = 250; e.button.y = 375; clockNow += 16;
  ponteiro_evento(&e, deliver);
}
static void oldMouse(int a, int b) { (void)a; (void)b; oldMouseActions++; }
static void chooseProfile(int a, int b) { (void)a; (void)b; profileTapActions++; boundary(2); }
static void oldBorder(void) { oldBorderActions++; }
static void finger(Uint32 kind, SDL_FingerID id, float x, float y) {
  SDL_Event e; SDL_zero(e); e.type = kind; e.tfinger.touchId = 4; e.tfinger.fingerId = id;
  e.tfinger.x = x / NV_TELA_W; e.tfinger.y = y / NV_TELA_H;
  clockNow += 16; ponteiro_evento(&e, deliver);
}
static void boundary(int profile) {
#ifdef PROFILE_TOUCH_APPLY_POINTER_RESET
  ponteiro_abrir_sessao();
#endif
  selectProfile(profile); home_abrir_sessao(); populateHome();
}
static void startSwipe(SDL_FingerID id) {
  finger(SDL_FINGERDOWN, id, 500, 850);
  finger(SDL_FINGERMOTION, id, 500, 750);
}
static void freshSwipeAndTap(SDL_FingerID id) {
  float before = scrollY; int oldTaps = taps;
  startSwipe(id);
  CHECK(scrollY > before + 50, "new vertical swipe reaches actual Home scroll callback");
  clockNow += 200; finger(SDL_FINGERUP, id, 500, 750);
  finger(SDL_FINGERDOWN, id + 1, 500, 750); finger(SDL_FINGERUP, id + 1, 500, 750);
  CHECK(taps == oldTaps + 1 && home_pediu_abrir(), "new tap focuses actual Home card and requests opening");
  CHECK(!focoHero && foco.fileira == 0 && foco.coluna == 0, "tap focuses the new profile's card");
}
int main(void) {
  setup(); alarm(10); layout_modo_definir(1); layout_tela_definir(1920, 1080);
  ponteiro_teste_relogio(testClock); ponteiro_teste_janela(960, 540);
  ponteiro_iniciar(); home_abrir_sessao(); populateHome(); frame();
  // The same Home callback is registered on both sides, so pointer identity
  // alone cannot recognize that its finger/capture belongs to another opening.
  startSwipe(10); CHECK(scrollY > 50, "outgoing swipe is captured");
  boundary(2); frame(); freshSwipeAndTap(20);
  // Also reject old inertia when reopening the same profile.
  ponteiro_cancelar_toque(); home_ir_topo(); frame(); startSwipe(30);
  finger(SDL_FINGERUP, 30, 500, 750);
  CHECK(inercia && inRolagem, "outgoing Home has real captured inertia");
  boundary(2); float resetOffset = scrollY; frame();
  CHECK(scrollY == resetOffset, "old inertia cannot move the freshly opened same-profile Home");
  freshSwipeAndTap(40);
  // A target callback may confirm the new profile inside FINGERUP. The
  // pointer must not remove a now-cleared finger twice or leak the old tap.
  frameTarget(NULL, chooseProfile);
  finger(SDL_FINGERDOWN, 50, 500, 750); finger(SDL_FINGERUP, 50, 500, 750);
  CHECK(profileTapActions == 1, "profile selection FINGERUP callback runs once");
  frame(); freshSwipeAndTap(51);
  // Mouse release has the same target identity on both sides. The old DOWN
  // must not activate it as B, including before a new frame is published.
  frameTarget(NULL, oldMouse); mouse(SDL_MOUSEBUTTONDOWN, SDL_BUTTON_LEFT);
  boundary(2); mouse(SDL_MOUSEBUTTONUP, SDL_BUTTON_LEFT);
  mouse(SDL_MOUSEBUTTONDOWN, SDL_BUTTON_LEFT); mouse(SDL_MOUSEBUTTONUP, SDL_BUTTON_LEFT);
  CHECK(!oldMouseActions, "previous targets and unframed mouse clicks cannot activate");
  frameTarget(NULL, oldMouse); mouse(SDL_MOUSEBUTTONUP, SDL_BUTTON_LEFT);
  CHECK(!oldMouseActions, "old pending mouse release cannot activate identical new target");
  frame(); int previousTaps = taps;
  mouse(SDL_MOUSEBUTTONDOWN, SDL_BUTTON_LEFT); mouse(SDL_MOUSEBUTTONUP, SDL_BUTTON_LEFT);
  CHECK(taps == previousTaps + 1, "new mouse click works with preserved window calibration");
  CHECK(ponteiro_tem_toque() && layout_modo_mobile(), "session reset preserves touch availability and device mode");
  // A held finger can reset input while delivering its synthetic KEYDOWN.
  // Its old delivery callback is cleared before the matching KEYUP.
  frameTarget(ponteiroCard, NULL); switchOnReturn = 1;
  finger(SDL_FINGERDOWN, 60, 500, 750); clockNow += NV_HOLD_MS + 1;
  frameTarget(ponteiroCard, NULL);
  CHECK(profileTapActions == 2, "long-press callback can reset input reentrantly");
  finger(SDL_FINGERUP, 60, 500, 750); frame(); freshSwipeAndTap(61);
  // No edge callback from the outgoing layer survives the boundary.
  ponteiro_quadro(clockNow); ponteiro_borda_esquerda(120, oldBorder); ponteiro_desenhar();
  finger(SDL_FINGERDOWN, 70, 20, 750); boundary(2);
  finger(SDL_FINGERMOTION, 70, 160, 750); finger(SDL_FINGERUP, 70, 160, 750);
  frame(); finger(SDL_FINGERDOWN, 71, 20, 750);
  finger(SDL_FINGERMOTION, 71, 160, 750); finger(SDL_FINGERUP, 71, 160, 750);
  CHECK(!oldBorderActions, "previous edge gesture cannot open a new profile's menu");
  // Native TV continues using remote arrows rather than Mobile capture.
  layout_modo_definir(0); boundary(2); frame(); int previousArrows = arrows;
  startSwipe(80); finger(SDL_FINGERMOTION, 80, 500, 450); finger(SDL_FINGERUP, 80, 500, 450);
  CHECK(arrows > previousArrows && scrollY == 0, "TV drag remains remote navigation");
  // A phone's portrait viewport uses the same actual gesture path, with
  // normalized SDL coordinates and enough content to scroll vertically.
  layout_modo_definir(1); layout_tela_definir(1080, 2400); boundary(1); frame();
  freshSwipeAndTap(90);
  if (assertionsFailed) return 1;
  puts("PASS touch session: capture/inertia discarded; fresh swipe/tap/mouse work; reentrant selection/hold safe; stale targets/edge cleared; TV arrows preserved");
}
