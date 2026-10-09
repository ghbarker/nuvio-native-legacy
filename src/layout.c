#include "layout.h"

#ifdef NV_TOUCH_PREVIEW
float nv_layout_w = NV_TELA_BASE_W;

void layout_tela_definir(int width, int height) {
  if (width > 0 && height > 0)
    nv_layout_w = NV_TELA_BASE_H * (float)width / (float)height;
}
#endif
