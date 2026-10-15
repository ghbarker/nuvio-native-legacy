// Review-only neighbours for the existing profile-selector reduced fixture.
// Capture the existing fixture identity, then forward authenticated snapshot
// RPCs to its unchanged sessao_rpc fake. This preserves profile, owner, offline
// and PIN responses/assertions; it does not short-circuit profile pulls.
#include "sessao.h"
#include "nuvem.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static const char fixture_jwt[] = "perfilsel-fixture-jwt";
char *perfilsel_fixture_anon_rpc(const char *, const char *, const char *, int *);
int sessao_identidade_local_copiar(char *user, size_t user_size,
                                  char *jwt, size_t jwt_size) {
  const char *identity = sessao_usuario();
  assert(identity && *identity && user && jwt);
  assert(strlen(identity) < user_size && sizeof fixture_jwt <= jwt_size);
  snprintf(user, user_size, "%s", identity);
  snprintf(jwt, jwt_size, "%s", fixture_jwt);
  return 1;
}
void sessao_token_recusado(const char *jwt) {
  assert(jwt && !strcmp(jwt, fixture_jwt));
  // The unchanged fake never returns401; no refresh or new account is invented.
}
char *nuvem_rpc_com(const char *function, const char *body,
                    const char *bearer, int *status) {
  if (bearer && *bearer) {
    assert(!strcmp(bearer, fixture_jwt));
    return sessao_rpc(function, body, status);
  }
  return perfilsel_fixture_anon_rpc(function, body, bearer, status);
}
