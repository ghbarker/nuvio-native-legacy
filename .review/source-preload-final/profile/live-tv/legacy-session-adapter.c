/* Review-only boundary for ownership fixtures that omit sessao.c.
 * Real perfis.c remains the producer of epochs and profile transitions.
 * These fixtures do not call perfis_puxar and keep their existing network
 * and storage doubles unchanged.
 * Abort even with NDEBUG if a transport helper is unexpectedly invoked.
 */
#include "sessao.h"
#include <assert.h>
#include <stdlib.h>

int sessao_identidade_local_copiar(char *usuario, size_t nUsuario,
                                  char *jwt, size_t nJwt) {
  (void)usuario; (void)nUsuario; (void)jwt; (void)nJwt;
  assert(!"ownership fixture unexpectedly captured session credentials");
  abort();
}

void sessao_token_recusado(const char *tok) {
  (void)tok;
  assert(!"ownership fixture unexpectedly refused or renewed a session token");
  abort();
}
