#include "layout.h"

#ifdef NV_TOUCH_PREVIEW
float nv_layout_w = NV_TELA_BASE_W;
float nv_layout_h = NV_TELA_BASE_H;

void layout_tela_definir(int width, int height) {
  if (width > 0 && height > 0) {
    float shortEdge = (float)(width < height ? width : height);
    nv_layout_w = NV_TELA_BASE_H * (float)width / shortEdge;
    nv_layout_h = NV_TELA_BASE_H * (float)height / shortEdge;
  }
}
#endif
