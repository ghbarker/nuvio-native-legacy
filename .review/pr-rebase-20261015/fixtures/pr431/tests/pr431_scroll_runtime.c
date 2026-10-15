#include "../src/rolagemtoque.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
  ToqueRolagem r = {0}; float offset = 40;
  GfxRect region = {0,0,100,100};
  PonteiroRolagem start = {.fase=PONT_ROL_INICIO,.eixoY=1,.x=50,.y=50};
  layout_modo_definir(1);
  toquerol_vincular(&r,region,1,0,100,1,&offset);
  assert(toquerol_evento(&r,&start) && toquerol_livre(&r));
  layout_modo_definir(0);
  assert(!toquerol_livre(&r));
  r.livre = 1; offset = 400;
  toquerol_vincular(&r,region,1,0,100,1,&offset);
  assert(!r.livre && offset == 400);
  r.livre = 1;
  assert(!toquerol_evento(&r,&start) && !r.livre);
  puts("Scroll: Mobile interaction works; TV clears retained free state without clamping offsets PASS");
  return 0;
}
