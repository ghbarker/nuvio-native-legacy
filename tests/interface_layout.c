// The shared resolver is the only native source of the active interface.
#include "../src/layout.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
  for(int automatic=0;automatic<2;automatic++) {
    assert(layout_modo_resolver(0,automatic)==0);
    assert(layout_modo_resolver(1,automatic)==1);
    assert(layout_modo_resolver(2,automatic)==automatic);
    assert(layout_modo_resolver(-1,automatic)==automatic);
    assert(layout_modo_resolver(3,automatic)==automatic);
  }
  layout_tela_definir(1080,2340);layout_modo_definir(0);
  assert(NV_TELA_W==1920 && NV_TELA_H==1080);
#ifdef NV_TOUCH_UI
  layout_modo_definir(layout_modo_resolver(2,1));
  assert(layout_modo_mobile() && NV_TELA_W==1080 && NV_TELA_H==2340);
  layout_tela_definir(0,0);assert(NV_TELA_W==1080 && NV_TELA_H==2340);
  layout_modo_definir(layout_modo_resolver(0,1));
  assert(!layout_modo_mobile() && NV_TELA_W==1920 && NV_TELA_H==1080);
  layout_modo_definir(layout_modo_resolver(1,0));
  assert(layout_modo_mobile() && NV_TELA_W==1080 && NV_TELA_H==2340);
#endif
  puts("interface_layout: device classification supplies Auto; explicit TV/Mobile always override PASS");
  return 0;
}
