#include "layout.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
  assert(!layout_modo_mobile());
  layout_tela_definir(1080, 2340);
  layout_modo_definir(1);
  assert(!layout_modo_mobile());
  assert(NV_TELA_W == 1920.0f && NV_TELA_H == 1080.0f);
  puts("layout_mode_tv: platforms without mobile capability keep TV layout PASS");
  return 0;
}
