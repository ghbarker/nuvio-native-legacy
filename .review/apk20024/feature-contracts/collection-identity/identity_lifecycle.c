#include "addons.h"
#include "perfis.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* The actual perfis.c session gate is linked; no account/network fixture. */
void fontecache_vod_limpar(void) {}
const char *rede_url_publica(const char *url, char *dst, unsigned tam) {
  (void)url; snprintf(dst, tam, "fixture.invalid"); return dst;
}

static AddonRemoto entry(const char *name, const char *base) {
  AddonRemoto a = {0};
  snprintf(a.nome, sizeof a.nome, "%s", name);
  snprintf(a.url, sizeof a.url, "%s", base);
  a.ativo = 1;
  return a;
}

static void expect(AddonIdentidadeEstado expected, const char *id) {
  AddonIdentidadeEstado actual = addons_identidade_estado(id);
  if (actual != expected) {
    fprintf(stderr, "identity state mismatch: expected=%d actual=%d\n", expected, actual);
    assert(actual == expected);
  }
}

int main(void) {
  const char *baseA = "https://a.invalid/private-fixture";
  const char *baseB = "https://b.invalid/private-fixture";
  const char *bodyA = "{\"catalogs\":[{\"id\":\"nested-catalog-id\"}],\"id\":\"fixture.a\",\"resources\":[\"catalog\"]}";
  const char *bodyB = "{\"id\":\"fixture.b\"}";
  AddonRemoto list[2] = {entry("A", baseA), entry("B", baseB)};

  expect(ADD_IDENTIDADE_AUSENTE, "fixture.a");
  assert(addons_definir_lista(list, 1));
  unsigned versionA = addons_versao();
  uint64_t sessionA = perfis_sessao_geracao();
  expect(ADD_IDENTIDADE_PENDENTE, "fixture.a");
  assert(!addons_sondado(0));
  assert(!addons_base_por_id("fixture.a")[0]);

  /* Neither an unrelated URL nor a stale list version may claim a slot. */
  addons_identidade_recebida(baseB, versionA, sessionA, bodyB);
  addons_identidade_recebida(baseA, versionA - 1u, sessionA, bodyA);
  assert(!addons_id_manifesto(0)[0]);
  expect(ADD_IDENTIDADE_PENDENTE, "fixture.a");

  assert(addons_identidade_recebida(baseA, versionA, sessionA, bodyA));
  expect(ADD_IDENTIDADE_PRONTA, "fixture.a");
  assert(!strcmp(addons_base_por_id("fixture.a"), baseA));
  assert(!strcmp(addons_id_manifesto(0), "fixture.a"));
  assert(!addons_base_por_id("nested-catalog-id")[0]);
  assert(!addons_sondado(0)); /* Identity is usable before capabilities/CW. */
  expect(ADD_IDENTIDADE_AUSENTE, "fixture.missing");
  char copiedBase[NV_ADDON_URL_MAX], tooSmall[2];
  assert(addons_base_por_id_copiar("fixture.a", copiedBase, sizeof copiedBase));
  assert(!strcmp(copiedBase, baseA));
  assert(!addons_base_por_id_copiar("fixture.a", tooSmall, sizeof tooSmall) && !tooSmall[0]);
  assert(!addons_manifesto_recebido(baseA, versionA - 1u, sessionA, bodyA));
  assert(!addons_sondado(0));
  assert(addons_manifesto_recebido(baseA, versionA, sessionA, bodyA));
  assert(addons_sondado(0));
  addons_identidade_recebida(baseA, versionA, sessionA, NULL);
  expect(ADD_IDENTIDADE_PRONTA, "fixture.a");
  assert(!strcmp(addons_base_por_id("fixture.a"), baseA));

  assert(addons_definir_lista(list, 2));
  unsigned versionAB = addons_versao();
  addons_identidade_recebida(baseA, versionAB, sessionA, NULL);
  expect(ADD_IDENTIDADE_PENDENTE, "fixture.missing"); /* B still pending wins. */
  addons_identidade_recebida(baseA, versionAB, sessionA, bodyA);
  expect(ADD_IDENTIDADE_PENDENTE, "fixture.b");
  addons_identidade_recebida(baseB, versionAB, sessionA, NULL);
  expect(ADD_IDENTIDADE_FALHOU, "fixture.b");
  expect(ADD_IDENTIDADE_PRONTA, "fixture.a");
  addons_identidade_recebida(baseB, versionAB, sessionA,
                            "{\"catalogs\":[{\"id\":\"nested-only\"}]}");
  expect(ADD_IDENTIDADE_FALHOU, "fixture.b");
  assert(!addons_base_por_id("nested-only")[0]);
  addons_identidade_recebida(baseB, versionAB, sessionA, bodyB);
  expect(ADD_IDENTIDADE_PRONTA, "fixture.b");
  expect(ADD_IDENTIDADE_AUSENTE, "fixture.missing");

  /* Replacement clears the old identity; all resolved lists prove absence. */
  assert(addons_definir_lista(&list[1], 1));
  unsigned versionB = addons_versao();
  assert(!addons_base_por_id("fixture.a")[0]);
  assert(!strcmp(copiedBase, baseA)); /* owned copy survives replacement */
  assert(!addons_manifesto_recebido(baseA, versionAB, sessionA, bodyA));
  assert(!addons_sondado(0));
  addons_identidade_recebida(baseA, versionAB, sessionA, bodyA);
  assert(!addons_id_manifesto(0)[0]);
  addons_identidade_recebida(baseB, versionB, sessionA, bodyB);
  expect(ADD_IDENTIDADE_AUSENTE, "fixture.a");
  expect(ADD_IDENTIDADE_PRONTA, "fixture.b");
  addons_esquecer();
  expect(ADD_IDENTIDADE_AUSENTE, "fixture.b");

  /* A disabled unresolved addon has no pending promise; a known ID still wins. */
  list[0].ativo = 0;
  assert(addons_definir_lista(list, 1));
  expect(ADD_IDENTIDADE_FALHOU, "fixture.a");
  addons_identidade_recebida(baseA, addons_versao(), sessionA, bodyA);
  expect(ADD_IDENTIDADE_PRONTA, "fixture.a");
  expect(ADD_IDENTIDADE_AUSENTE, "fixture.missing");
  addons_esquecer();
  list[0].ativo = 1;

  /* A -> B -> A uses the real monotonically advancing profile session. */
  perfis_invalidar_sessao();
  uint64_t sessionB = perfis_sessao_geracao();
  assert(sessionB != sessionA);
  assert(addons_definir_lista(&list[1], 1));
  unsigned versionProfileB = addons_versao();
  assert(!addons_manifesto_recebido(baseB, versionProfileB, sessionA, bodyA));
  assert(!addons_sondado(0));
  addons_identidade_recebida(baseB, versionProfileB, sessionA, bodyA);
  assert(!addons_id_manifesto(0)[0]);
  expect(ADD_IDENTIDADE_PENDENTE, "fixture.b");
  addons_identidade_recebida(baseB, versionProfileB, sessionB, bodyB);
  expect(ADD_IDENTIDADE_PRONTA, "fixture.b");

  perfis_invalidar_sessao();
  addons_esquecer();
  assert(addons_definir_lista(list, 1));
  unsigned versionReturnedA = addons_versao();
  uint64_t sessionReturnedA = perfis_sessao_geracao();
  assert(sessionReturnedA != sessionA && sessionReturnedA != sessionB);
  /* Same current URL and version cannot rescue the old A session. */
  addons_identidade_recebida(baseA, versionReturnedA, sessionA, bodyA);
  assert(!addons_id_manifesto(0)[0]);
  expect(ADD_IDENTIDADE_PENDENTE, "fixture.a");
  addons_identidade_recebida(baseA, versionReturnedA, sessionReturnedA, bodyA);
  expect(ADD_IDENTIDADE_PRONTA, "fixture.a");

  puts("PASS: identity absence, pending, ready-before-capabilities, failure, replacement, removal, stale URL/version/session, and A-B-A lifecycle");
  return 0;
}
