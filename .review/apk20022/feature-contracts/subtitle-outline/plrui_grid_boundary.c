/* Terminal boundaries for the unchanged legacy player-Style geometry fixture.
 * Actual plrui.c supplies the newly referenced button producers. The fixture
 * does not enter standalone Settings mode: color/shadow calls must stay dead. */
#include "plrui.h"
#include "ajustes.h"
#include "text.h"
#include <stdio.h>
#include <stdlib.h>

static _Noreturn void unreached(const char *name) {
  fprintf(stderr, "UNEXPECTED dependency: %s\n", name);
  abort();
}
int txt_largura(TxtEstilo estilo, const char *s) {
  return txt_linha(estilo, s, 255, 255, 255, 255).w;
}
void ajustes_acento(float *r, float *g, float *b) {
  (void)r; (void)g; (void)b;
  unreached("ajustes_acento");
}
int ajustes_tinta_foco(void) {
  unreached("ajustes_tinta_foco");
}
void gfx_rect(GfxRect r, GLuint tex, GfxModo modo, float foco,
              float parx, float pary, float raio,
              float cr, float cg, float cb, float ca) {
  (void)r; (void)tex; (void)modo; (void)foco;
  (void)parx; (void)pary; (void)raio;
  (void)cr; (void)cg; (void)cb; (void)ca;
  unreached("gfx_rect");
}
