// Existing catalog-parser fixture mocks the older download wrapper. Keep
// its canned payload while exercising the unchanged real parsing assertions.
#include "rede.h"
char *rede_baixar_medido_controle(const char *url, int sec, const char *const *headers,
                                const RedeControle *ctl, RedeMedida *med) {
  (void)headers; (void)ctl; (void)med; return rede_baixar(url, sec);
}
