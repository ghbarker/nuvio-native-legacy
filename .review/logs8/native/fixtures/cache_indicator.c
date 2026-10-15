#include NV_STREAMS_SOURCE
#include <assert.h>
static int icons;
static GfxRect drawn;
static float drawnAlpha;
void gfx_icone(GfxRect r, const char *id, float red, float green, float blue, float alpha) {
  assert(!strcmp(id, "aj_zap"));
  assert(red > .8f && green > .7f && blue > .4f);
  drawn = r; drawnAlpha = alpha; icons++;
}
int main(void) {
  Stream s = {0};
  assert(desenharCache(&s, 100, 200, 1) == 0 && icons == 0);
  s.emCache = 1;
  assert(desenharCache(&s, 100, 200, 1) == 30 && icons == 1);
  assert(drawn.x == 100 && drawn.y == 204 && drawn.w == 22 && drawn.h == 22 && drawnAlpha == 1);
  s.foraCache = 1;
  assert(desenharCache(&s, 100, 200, 1) == 0 && icons == 1);
  s.foraCache = 0; s.urlP2P = 1;
  assert(desenharCache(&s, 100, 200, 1) == 0 && icons == 1);
  s.urlP2P = 0; s.naoVideo = 1;
  assert(desenharCache(&s, 100, 200, 1) == 0 && icons == 1);
  s.naoVideo = 0;
  assert(desenharCache(&s, 100, 200, .4f) == 30 && icons == 2 && drawnAlpha == .4f);
  puts("cache_indicator: actual production draw helper, later negatives/P2P/non-video, unchanged row height, focus alpha passed");
}
