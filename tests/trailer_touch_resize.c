/* Real trailer module; native video messages are counted without a decoder. */
#define NV_TOUCH_UI 1
#define SDL_MAIN_HANDLED 1
#include "../src/trailer.c"
#include <assert.h>

float nv_layout_w = 2340, nv_layout_h = 1080;
static int testMobile = 1;
int layout_modo_mobile(void) { return testMobile; }
static int windows, cropWindows, volumes, ready;
static GfxRect last;
Uint32 SDL_GetTicks(void) { return 1000; }
int video_tocando(void) { return 0; }
int video_ativo(void) { return 1; }
int video_pronto(void) { return ready; }
int video_largura(void) { return 1920; }
int video_altura(void) { return 1080; }
int video_recorte_fonte(void) { return 1; }
float ajustes_trailer_zoom(void) { return 1; }
void video_volume(int value) { (void)value; volumes++; }
void video_recorte_reaplicar(void) {}
void video_janela(int x, int y, int w, int h) {
  windows++; last = (GfxRect){x,y,w,h};
}
void video_janela_fonte(int sx, int sy, int sw, int sh, int x, int y, int w, int h) {
  (void)sx; (void)sy; (void)sw; (void)sh;
  cropWindows++; last = (GfxRect){x,y,w,h};
}
int main(void) {
  GfxRect portrait = {0,0,1080,2340};
  trailer_rect(portrait); assert(!windows); /* closed sessions stay closed */
  aberto = 1; rect = (GfxRect){0,0,2340,1080};
  volumePendente = 0;
  trailer_rect(rect); assert(!windows);
  trailer_rect(portrait); assert(windows == 1 && last.w == 1080 && last.h == 2340);
  for (int i = 0; i < 100; i++) trailer_rect(portrait);
  assert(windows == 1 && recortePendente); /* equality does not clear pending prepare */
  ready = 1; nativoAplicar(); assert(cropWindows == 1);
  assert(last.w == 1080 && last.h == 2340);
  trailer_rect((GfxRect){96,0,888,620});
  assert(windows == 2 && last.x == 96 && last.w == 888 && last.h == 620);
  trailer_rect((GfxRect){96,0,888,620}); assert(windows == 2);
  trailer_rect((GfxRect){96,10,888,620}); assert(windows == 3 && last.y == 10);
  assert(!volumes && aberto && !cheia); /* reposition does not alter mode or volume */
  puts("trailer_touch_resize: rotated/detail/Home windows, dedupe and pending crop PASS");
  return 0;
}
