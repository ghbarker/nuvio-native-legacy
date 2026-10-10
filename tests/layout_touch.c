#include "layout.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

// Exercise the layout macros without loading a graphics driver.
#define NV_GFX_H
static float scale = 1.5f;
float gfx_escala_ui(void) { return scale; }
float gfx_escala(void) { return scale; }
void gfx_escala_sair(float value) { scale = value; }
#include "escala.h"
static void active_scale(void);

static void viewport(int width, int height, float expectedW, float expectedH) {
  layout_tela_definir(width, height);
  assert(fabsf(NV_TELA_W - expectedW) < 0.01f);
  assert(fabsf(NV_TELA_H - expectedH) < 0.01f);
  assert(fabsf(width / NV_TELA_W - height / NV_TELA_H) < 0.00001f);
  assert(fabsf(NV_VTELA_W * scale - NV_TELA_W) < 0.01f);
  assert(fabsf(NV_VTELA_H * scale - NV_TELA_H) < 0.01f);
  // A touch at the right edge maps to the same physical edge as rendering.
  assert(fabsf((0.97f * NV_TELA_W) * width / NV_TELA_W - 0.97f * width) < 0.01f);
  assert(fabsf((0.97f * NV_TELA_H) * height / NV_TELA_H - 0.97f * height) < 0.01f);
}

int main(void) {
  // A normal APK starts in TV, even on a portrait drawable. A mode change
  // recalculates immediately from the last size without another resize event.
  assert(!layout_modo_mobile());
  layout_tela_definir(1080, 2340);
  assert(NV_TELA_W == 1920.0f && NV_TELA_H == 1080.0f);
  layout_modo_definir(1);
  assert(layout_modo_mobile());
  assert(NV_TELA_W == 1080.0f && NV_TELA_H == 2340.0f);
  layout_modo_definir(0);
  assert(!layout_modo_mobile());
  assert(NV_TELA_W == 1920.0f && NV_TELA_H == 1080.0f);
  layout_modo_definir(1);
  viewport(1920, 1080, 1920.0f, 1080.0f);
  viewport(2340, 1080, 2340.0f, 1080.0f);
  viewport(3120, 1440, 2340.0f, 1080.0f);
  viewport(2560, 1600, 1728.0f, 1080.0f);
  viewport(1080, 2340, 1080.0f, 2340.0f);
  viewport(1440, 3120, 1080.0f, 2340.0f);
  viewport(1080, 1920, 1080.0f, 1920.0f);
  viewport(1600, 2560, 1080.0f, 1728.0f);
  float before = NV_TELA_W;
  float beforeH = NV_TELA_H;
  layout_tela_definir(0, 1080);
  layout_tela_definir(2340, 0);
  assert(NV_TELA_W == before);
  assert(NV_TELA_H == beforeH);
  layout_modo_definir(0);
  assert(NV_TELA_W == 1920.0f && NV_TELA_H == 1080.0f);
  layout_tela_definir(2560, 1600);
  assert(NV_TELA_W == 1920.0f && NV_TELA_H == 1080.0f);
  layout_modo_definir(1);
  assert(NV_TELA_W == 1728.0f && NV_TELA_H == 1080.0f);
  active_scale();
  puts("layout_touch: TV default, immediate switching, mobile aspect ratio and touch mapping PASS");
  return 0;
}

#define NV_ESCALA_TELA_ATIVA
#include "escala.h"
static void active_scale(void) {
  assert(fabsf(NV_TELA_W * scale - nv_layout_w) < 0.01f);
  assert(fabsf(NV_TELA_H * scale - nv_layout_h) < 0.01f);
  layout_tela_definir(2340, 1080);
  ESCALA_SE_COUBER_INI(1450, 500);
  assert(scale == 1.5f);
  ESCALA_SE_COUBER_FIM();
  layout_tela_definir(1080, 2340);
  { ESCALA_SE_COUBER_INI(600, 1200);
  assert(scale == 1.5f);
  ESCALA_SE_COUBER_FIM(); }
  { ESCALA_SE_COUBER_INI(800, 500);
  assert(scale == 1.0f);
  ESCALA_SE_COUBER_FIM(); }
}

