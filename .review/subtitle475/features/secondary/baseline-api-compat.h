#include "rolagemtoque.h"
#ifdef NV_TOUCH_UI
static inline int toquerol_livre(ToqueRolagem *r) {
  if (!layout_modo_mobile()) r->livre = 0;
  return r->livre;
}
#endif
