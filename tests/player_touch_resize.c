/* Real player geometry; only the hardware video-window messages are mocked. */
#define NV_ANDROID 1
#define NV_TOUCH_UI 1
#define SDL_MAIN_HANDLED 1
#include <time.h>
#ifdef _WIN32
static struct tm *localtime_r(const time_t *value, struct tm *dst) {
  struct tm *result = localtime(value);
  if (result) *dst = *result;
  return result ? dst : NULL;
}
#endif
#include "../src/player.c"
#include <assert.h>

float nv_layout_w = 2340, nv_layout_h = 1080;
static int testMobile = 1;
int layout_modo_mobile(void) { return testMobile; }
float gfx_escala(void) { return 1; }
float gfx_escala_ui(void) { return 1; }
int video_largura(void) { return 1920; }
int video_altura(void) { return 1080; }
int video_recorte_fonte(void) { return 0; }
static int windows;
static PlrRect last;
void video_janela(int x, int y, int w, int h) {
  windows++; last = (PlrRect){x,y,w,h};
}
void video_janela_fonte(int sx, int sy, int sw, int sh, int x, int y, int w, int h) {
  (void)sx; (void)sy; (void)sw; (void)sh;
  video_janela(x,y,w,h);
}
static void within(void) {
  assert(last.x >= 0 && last.y >= 0 && last.w > 0 && last.h > 0);
  assert(last.x + last.w <= nv_layout_w + 1 && last.y + last.h <= nv_layout_h + 1);
  assert(fabsf(last.w / last.h - 16.0f / 9.0f) < .01f);
}
int main(void) {
  comVideo = 0; playerTelaAtualizar(); assert(!windows);
  comVideo = 1; mini = retido = janAtiva = aspPendente = 0;
  aspecto = PLR_ASP_ORIGINAL; encolhe = 1;
  playerTelaAtualizar(); assert(windows == 1); within();
  assert(last.x == 210 && last.y == 0 && last.w == 1920 && last.h == 1080);
  playerTelaAtualizar(); assert(windows == 1);
  nv_layout_w = 1080; nv_layout_h = 2340;
  playerTelaAtualizar(); assert(windows == 2); within();
  assert(last.x == 0 && last.w == 1080 && last.y > 800);
  /* Guide/ordinary PiP stays alive with the full-screen player closed. */
  mini = 1; aberto = 0; miniGuia = 0;
  nv_layout_w = 2340; nv_layout_h = 1080;
  playerTelaAtualizar(); assert(windows == 3); within();
  float landscapeX = last.x;
  nv_layout_w = 1080; nv_layout_h = 2340;
  playerTelaAtualizar(); assert(windows == 4); within();
  assert(last.x < landscapeX && last.y > 1800);
  playerTelaAtualizar(); assert(windows == 4);
  /* The existing transition and DV first-frame gates retain their authority. */
  nv_layout_w = 2340; nv_layout_h = 1080; janAtiva = 1;
  playerTelaAtualizar(); assert(windows == 4);
  janAtiva = 0; aplicarAspecto(); assert(windows == 5); within();
  nv_layout_w = 1080; nv_layout_h = 2340; mini = 0; aspPendente = 1;
  playerTelaAtualizar(); assert(windows == 5);
  aspPendente = 0; aplicarAspecto(); assert(windows == 6); within();
  /* Hidden retained sessions keep their window until the existing restore. */
  retido = 1; nv_layout_w = 2340; nv_layout_h = 1080;
  playerTelaAtualizar(); assert(windows == 6);
  retido = 0; playerTelaAtualizar(); assert(windows == 7); within();
  puts("player_touch_resize: uniform full-screen/PiP refresh, dedupe and lifecycle gates PASS");
  return 0;
}
