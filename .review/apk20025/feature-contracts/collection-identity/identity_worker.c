/* Include the full production discovery unit, as tests/manifesto_wait.c does. */
/* The retry boundary is a trap in a separate unit; no full Home mount here. */
__attribute__((weak)) void desc_repetir_silencioso(void);
int ajustes_social(void) { return 1; }
int ajustes_hist_conta(void) { return 1; }
int ajustes_busca_cinemeta(void) { return 1; }
int ajustes_busca_nuvio(void) { return 0; }
#include DISCOVERY_SOURCE
#include <assert.h>
#include <stdatomic.h>
#include <unistd.h>

static _Atomic int requests;
static int networkMode;
static const char *networkBody;
static const char *fixtureBase = "https://eager.invalid/private-fixture";

void fontecache_vod_limpar(void) {}
void SDL_Delay(Uint32 ms) { usleep(ms * 1000u); }

char *rede_baixar(const char *url, int seconds) {
  (void)seconds;
  assert(strstr(url, "/manifest.json"));
  requests++;
  /* The identity is still unresolved when the actual worker enters network. */
  assert(addons_identidade_estado("fixture.eager") == ADD_IDENTIDADE_PENDENTE);
  assert(!addons_sondado(0));
  if (networkMode == 1) {
    assert(addons_alternar(0) == 0);
    assert(addons_alternar(0) == 1); /* New version, same URL and final state. */
  } else if (networkMode == 2) {
    perfis_invalidar_sessao();
  }
  return networkBody ? strdup(networkBody) : NULL;
}

static void install(void) {
  AddonRemoto a = {0};
  addons_esquecer();
  maniCacheLimpar();
  for (int i = 0; i < maniN; i++) free(mani[i].corpo);
  memset(mani, 0, sizeof mani);
  maniN = 0;
  snprintf(a.nome, sizeof a.nome, "%s", "Fixture current list");
  snprintf(a.url, sizeof a.url, "%s", fixtureBase);
  a.ativo = 1;
  assert(addons_definir_lista(&a, 1));
  networkMode = 0;
  networkBody = "{\"catalogs\":[{\"id\":\"wrong-nested-id\"}],\"id\":\"fixture.eager\"}";
}

static void run_actual_worker(void) {
  maniN = networkMode ? 2 : 1;
  maniProx = 0;
  maniGeracao++;
  maniVersao = addons_versao();
  maniSessao = nv_perfil_sessao_ler();
  snprintf(mani[0].base, sizeof mani[0].base, "%s", fixtureBase);
  nv_addon_url(mani[0].url, sizeof mani[0].url, fixtureBase, "/manifest.json");
  /* Synchronous invocation gives exact completion without any CW/catalog work. */
  fioManifesto((void *)(uintptr_t)maniGeracao);
}

int main(void) {
  install();
  run_actual_worker();
  assert(requests == 1);
  assert(mani[0].pronto && !mani[0].erro);
  assert(addons_identidade_estado("fixture.eager") == ADD_IDENTIDADE_PRONTA);
  assert(!strcmp(addons_base_por_id("fixture.eager"), fixtureBase));
  assert(!addons_sondado(0)); /* CW and full manifest capabilities never ran. */

  install();
  networkBody = NULL;
  run_actual_worker();
  assert(mani[0].pronto && mani[0].erro);
  assert(addons_identidade_estado("fixture.eager") == ADD_IDENTIDADE_FALHOU);

  install();
  networkMode = 1;
  unsigned oldVersion = addons_versao();
  run_actual_worker();
  assert(addons_versao() != oldVersion);
  assert(!addons_id_manifesto(0)[0]);
  assert(mani[0].pronto && mani[0].erro && mani[1].pronto && mani[1].erro);
  assert(addons_identidade_estado("fixture.eager") == ADD_IDENTIDADE_PENDENTE);
  char *cached = desc_manifesto_cache_obter(mani[0].url, addons_versao());
  assert(!cached); /* Old response cannot be keyed under the new version. */

  install();
  networkMode = 2;
  uint64_t oldSession = nv_perfil_sessao_ler();
  oldVersion = addons_versao();
  run_actual_worker();
  assert(nv_perfil_sessao_ler() != oldSession);
  assert(addons_versao() == oldVersion); /* Isolates the session gate. */
  assert(!addons_id_manifesto(0)[0]);
  assert(mani[0].pronto && mani[0].erro && mani[1].pronto && mani[1].erro);

  install();
  char manifestUrl[NV_ADDON_PEDIDO_MAX];
  nv_addon_url(manifestUrl, sizeof manifestUrl, fixtureBase, "/manifest.json");
  desc_manifesto_cache_guardar(manifestUrl, addons_versao(), networkBody);
  int requestsBeforeCache = requests;
  maniLargar();
  assert(requests == requestsBeforeCache);
  assert(addons_identidade_estado("fixture.eager") == ADD_IDENTIDADE_PRONTA);
  assert(!addons_sondado(0));
  int attempted = 0;
  char *body = maniPegar(manifestUrl, 1, &attempted);
  assert(attempted && body && strstr(body, "fixture.eager"));
  free(body);
  assert(requests == requestsBeforeCache);

  maniCacheLimpar();
  puts("PASS: actual full-include manifest worker and cache hits publish identity before CW/capabilities; failure and stale version/session are guarded");
  return 0;
}
