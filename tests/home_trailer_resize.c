/* Actual Home ownership gate, without a decoder or a GL window. */
#define NV_TOUCH_PREVIEW 1
#define SDL_MAIN_HANDLED 1
#include "../src/home.c"
#include <assert.h>

float nv_layout_w = 2340, nv_layout_h = 1080;
static int abertoTeste, cheiaTeste, donoTeste, rects;
static GfxRect ultimo;
int trailer_aberto(void) { return abertoTeste; }
int trailer_cheia(void) { return cheiaTeste; }
int trailer_dono(void) { return donoTeste; }
void trailer_rect(GfxRect r) { rects++; ultimo = r; }

static void conferir(GfxRect r) {
  int antes = rects;
  heroTrailerAtualizaRect(r, 0);
  assert(rects == antes + 1);
  assert(ultimo.x == r.x && ultimo.y == r.y && ultimo.w == r.w && ultimo.h == r.h);
}

int main(void) {
  GfxRect paisagem = {0, 0, 2340, 1080}, retrato = {0, 0, 1080, 1080};
  abertoTeste = 1; donoTeste = TRAILER_DONO_HOME;
  heroAtual = heroTrailerItem = 7;
  conferir(paisagem); conferir(retrato); conferir(paisagem);
  /* The same hook follows translated banners and narrow artwork, not the screen. */
  conferir((GfxRect){0, -13.5f, 1080, 528});
  conferir((GfxRect){555, 0, 1421, 670});
  int antes = rects;
  heroTrailerAtualizaRect(retrato, 0.5f); assert(rects == antes);
  donoTeste = TRAILER_DONO_DETALHE;
  heroTrailerAtualizaRect(retrato, 0); assert(rects == antes);
  donoTeste = TRAILER_DONO_NENHUM;
  heroTrailerAtualizaRect(retrato, 0); assert(rects == antes);
  donoTeste = TRAILER_DONO_HOME; cheiaTeste = 1;
  heroTrailerAtualizaRect(retrato, 0); assert(rects == antes);
  cheiaTeste = 0; heroTrailerItem = 6;
  heroTrailerAtualizaRect(retrato, 0); assert(rects == antes);
  heroAtual = heroTrailerItem = -1;
  heroTrailerAtualizaRect(retrato, 0); assert(rects == antes);
  heroAtual = heroTrailerItem = 7; abertoTeste = 0;
  heroTrailerAtualizaRect(retrato, 0); assert(rects == antes && !abertoTeste);
  puts("home_trailer_resize: artwork rotation/translation and ownership guards PASS");
  return 0;
}
