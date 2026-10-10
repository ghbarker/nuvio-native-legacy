/* Actual Home ownership gate, without a decoder or a GL window. */
#define NV_TOUCH_PREVIEW 1
#define SDL_MAIN_HANDLED 1
#include "../src/home.c"
#include <assert.h>

float nv_layout_w = 2340, nv_layout_h = 1080;
static int abertoTeste, cheiaTeste, donoTeste, rects;
static int layoutTeste = HOME_LAYOUT_MODERNA;
static GfxRect ultimo;
int ajustes_home_layout(void) { return layoutTeste; }
int ajustes_hero_ligado(void) { return 1; }
int trailer_aberto(void) { return abertoTeste; }
int trailer_cheia(void) { return cheiaTeste; }
int trailer_dono(void) { return donoTeste; }
void trailer_rect(GfxRect r) { rects++; ultimo = r; }

static void perto(float a, float b) { assert(fabsf(a - b) < 0.01f); }

static void conferir(GfxRect r) {
  int antes = rects;
  heroTrailerAtualizaRect(r, 0);
  assert(rects == antes + 1);
  assert(ultimo.x == r.x && ultimo.y == r.y && ultimo.w == r.w && ultimo.h == r.h);
}

static void origem(GfxRect r) {
  float x, y, w, h;
  heroArteRect = r;
  home_hero_rect(&x, &y, &w, &h);
  perto(x, r.x); perto(y, r.y); perto(w, r.w); perto(h, r.h);
  conferir(r);
}

static void geometriaTelefone(float w, float h) {
  GfxRect r, overlay;
  nv_layout_w = w; nv_layout_h = h;
  toqueLivreY = 0; temItemFoco = 0;
  layoutTeste = HOME_LAYOUT_MODERNA;
  scrollY = -empurraHero();
  r = heroArtworkRect(layoutTeste, 1);
  perto(r.x, 0); perto(r.y, 0); perto(r.w, w);
  perto(r.h, h > w ? 600.0f : 729.25f);
  perto(r.y + r.h, topoFileiras() - scrollY - 20.0f);
  assert(heroBaseCopia(layoutTeste, r, -scrollY, 0) < r.y + r.h);
  origem(r);
  /* The narrow background preference also remains inside the compact hero. */
  r = heroArtworkRect(layoutTeste, 0);
  assert(r.x >= 0 && r.x + r.w <= w && r.h <= (h > w ? 600.0f : 729.25f));
  if (h > w) { perto(r.x, 0); perto(r.w, w); perto(r.h, 600); }
  else { perto(r.x, 555); perto(r.w, 1421); perto(r.h, 670); }
  origem(r);
  scrollY *= .5f;
  r = heroArtworkRect(layoutTeste, 1);
  perto(r.y + r.h, topoFileiras() - scrollY - 20.0f);
  origem(r);
  scrollY = 600;
  r = heroArtworkRect(layoutTeste, 1);
  perto(r.h, 132);
  origem(r);

  layoutTeste = HOME_LAYOUT_DINAMICA;
  scrollY = -empurraHero();
  r = heroArtworkRect(layoutTeste, 0);
  perto(r.y, 0); perto(r.h, h > w ? 600.0f : 715.0f);
  perto(r.y + r.h, topoFileiras() - scrollY - 20.0f);
  assert(heroBaseCopia(layoutTeste, r, -scrollY, 0) < r.y + r.h);
  origem(r);
  scrollY += 137.5f;
  r = heroArtworkRect(layoutTeste, 1);
  perto(r.y, -137.5f);
  perto(r.y + r.h, topoFileiras() - scrollY - 20.0f);
  origem(r);

  layoutTeste = HOME_LAYOUT_PADRAO;
  scrollY = 0;
  r = heroArtworkRect(layoutTeste, 1);
  perto(r.y, 0); perto(r.w, w); perto(r.h, 528);
  perto(topoFileiras() - (r.y + r.h), 16);
  assert(heroBaseCopia(layoutTeste, r, 0, 0) < r.y + r.h);
  origem(r);
  toqueLivreY = 1; scrollY = 13.5f;
  r = heroArtworkRect(layoutTeste, 0);
  perto(r.y, -13.5f); perto(r.h, 528);
  origem(r);

  /* Artwork overlays are bounded without imposing a clip on copy/actions. */
  layoutTeste = HOME_LAYOUT_MODERNA;
  scrollY = -empurraHero();
  r = heroArtworkRect(layoutTeste, 1);
  GfxRect retrato = {w*.4f, -20, w*.7f, 1120};
  overlay = heroArtworkOverlayRect(retrato, r);
  assert(overlay.x >= r.x && overlay.y >= r.y);
  assert(overlay.x + overlay.w <= r.x + r.w && overlay.y + overlay.h <= r.y + r.h);
  perto(overlay.y + overlay.h, r.y + r.h);
  perto(overlay.w / overlay.h, retrato.w / retrato.h);
  GfxRect editorial = heroEditorialRect(2, .5f);
  overlay = heroArtworkOverlayRect(editorial, r);
  assert(overlay.h > 0 && overlay.y + overlay.h <= r.y + r.h);
  perto(overlay.w / overlay.h, .5f);
  /* A wide editorial image retains its source shape in the narrow hero. */
  scrollY = 0;
  r = heroArtworkRect(layoutTeste, 0);
  for (int tipo = 1; tipo <= 2; tipo++) for (int asp = 1; asp <= 5; asp += 2) {
    editorial = heroEditorialRect(tipo, (float)asp);
    overlay = heroArtworkOverlayRect(editorial, r);
    assert(overlay.w > 0 && overlay.h > 0);
    assert(overlay.x >= r.x && overlay.y >= r.y);
    assert(overlay.x + overlay.w <= r.x + r.w && overlay.y + overlay.h <= r.y + r.h);
    perto(overlay.w / overlay.h, editorial.w / editorial.h);
    assert(overlay.w <= editorial.w && overlay.h <= editorial.h);
  }
  if (w > h) {
    editorial = heroEditorialRect(2, 3);
    overlay = heroArtworkOverlayRect(editorial, r);
    perto(overlay.x, 555); perto(overlay.y, 0);
    perto(overlay.w, 1365); perto(overlay.h, 455);
  }
  toqueLivreY = 0;
}

static void geometriaTablet(void) {
  nv_layout_w = 1920; nv_layout_h = 1080;
  layoutTeste = HOME_LAYOUT_MODERNA; scrollY = -empurraHero();
  GfxRect r = heroArtworkRect(layoutTeste, 1);
  perto(r.w, 1920); perto(r.h, 1080); origem(r);
  r = heroArtworkRect(layoutTeste, 0);
  perto(r.x, 555); perto(r.w, 1421); perto(r.h, 670); origem(r);
  layoutTeste = HOME_LAYOUT_DINAMICA; scrollY = -empurraHero();
  r = heroArtworkRect(layoutTeste, 0);
  perto(r.y, 0); perto(r.h, 1080); origem(r);
  layoutTeste = HOME_LAYOUT_PADRAO;
  r = heroArtworkRect(layoutTeste, 0);
  perto(r.y, 0); perto(r.h, 528); origem(r);
  GfxRect overlay = {840, -20, 1080, 1120};
  GfxRect inteiro = heroArtworkOverlayRect(overlay, r);
  perto(inteiro.x, overlay.x); perto(inteiro.y, overlay.y);
  perto(inteiro.w, overlay.w); perto(inteiro.h, overlay.h);
}

int main(void) {
  GfxRect paisagem = {0, 0, 2340, 1080}, retrato = {0, 0, 1080, 1080};
  abertoTeste = 1; donoTeste = TRAILER_DONO_HOME;
  heroAtual = heroTrailerItem = 7;
  geometriaTelefone(1080, 2340);
  geometriaTelefone(2340, 1080);
  geometriaTelefone(1080, 1920);
  geometriaTelefone(2520, 1080);
  geometriaTablet();
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
  puts("home_trailer_resize: compact artwork, transitions, rotation and ownership guards PASS");
  return 0;
}
