// Isolated metadata fixtures do not run addon-list/manifest publication.
// Keep this external-service boundary typed against the public declaration;
// an unexpected call must fail rather than pretend ownership was validated.
#include "../src/addons.h"

static void metadataManifestNotificationUnexpected(const char *api) {
  fprintf(stderr, "metadata fixture unexpectedly called %s\n", api);
  abort();
}

int addons_identidade_recebida(const char *base, unsigned versao, uint64_t sessao,
                               const char *corpo) {
  (void)base; (void)versao; (void)sessao; (void)corpo;
  metadataManifestNotificationUnexpected("addons_identidade_recebida");
  return 0;
}

int addons_manifesto_recebido(const char *base, unsigned versao, uint64_t sessao,
                              const char *corpo) {
  (void)base; (void)versao; (void)sessao; (void)corpo;
  metadataManifestNotificationUnexpected("addons_manifesto_recebido");
  return 0;
}
