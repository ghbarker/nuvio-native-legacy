#include "telefoneui.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
  const int sizes[][2] = {{1920, 1080}, {2560, 1600}, {1600, 2560}, {1080, 2340}};
  for (int i = 0; i < 4; i++) {
    layout_tela_definir(sizes[i][0], sizes[i][1]);
    layout_modo_definir(0);
    assert(!telefoneui_ativo());
    assert(telefoneui_largura(1400, 1080, 48) == 1400);
    layout_modo_definir(1);
    assert(telefoneui_ativo());
    assert(telefoneui_largura(1400, 1080, 48) == 984);
    layout_modo_definir(0);
    assert(!telefoneui_ativo());
  }
  puts("telefoneui_mode: explicit TV/Mobile selection works at 16:9, 16:10 and phone dimensions PASS");
  return 0;
}
