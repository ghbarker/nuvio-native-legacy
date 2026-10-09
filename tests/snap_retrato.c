// A narrow snapshot must ignore bright bytes left by its wider predecessor.
#define GL_APICALL
#define SDL_MAIN_HANDLED
#include "../src/gfx.c"
#include <assert.h>

static unsigned char canal;
static int lidas;
void GL_APIENTRY glReadPixels(GLint x, GLint y, GLsizei w, GLsizei h,
                             GLenum formato, GLenum tipo, void *out) {
  assert(x == 0 && y >= 0 && h == 1 && w <= snapW);
  assert(formato == GL_RGBA && tipo == GL_UNSIGNED_BYTE);
  memset(out, canal, (size_t)w * 4);
  lidas++;
}

int main(void) {
  int maximo;
  snapFbo = snapAtivo = 1;
  snapW = 1920; snapH = 1080; canal = 255;
  assert(!gfx_snap_vazio(&maximo) && maximo == 255 && lidas == 3);
  snapW = 1080; snapH = 2340; canal = 0;
  assert(gfx_snap_vazio(&maximo) && maximo == 0 && lidas == 6);
  snapW = 1; canal = 3;
  assert(gfx_snap_vazio(&maximo) && maximo == 3);
  canal = 4;
  assert(!gfx_snap_vazio(&maximo) && maximo == 4);
  snapAtivo = 0;
  assert(!gfx_snap_vazio(NULL) && lidas == 12);
  puts("snap_retrato: landscape-to-portrait black fallback PASS");
  return 0;
}
