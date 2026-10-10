#ifndef NV_TELEFONEUI_H
#define NV_TELEFONEUI_H
#include "layout.h"
#include <math.h>

/* Decide before UI scaling. A tall phone can rotate without becoming a
 * tablet; the ordinary 16:9 landscape and 16:10 tablet keep their layout. */
static inline int telefoneui_ativo(void) {
#ifdef NV_TOUCH_PREVIEW
  return nv_layout_w > 0 && nv_layout_h > 0 &&
    (nv_layout_h >= nv_layout_w ? nv_layout_h / nv_layout_w >= 1.7f
                               : nv_layout_w / nv_layout_h >= 1.95f);
#else
  return 0;
#endif
}
/* The caller supplies bounds in its own (possibly scaled) drawing layer. */
static inline float telefoneui_largura(float original, float tela, float margem) {
  return telefoneui_ativo() ? fminf(original, fmaxf(1, tela - 2 * margem)) : original;
}
#endif
