// Test the real discovery state machine without HTTP or SDL initialization.
// Include the module to drive request start/final completion independently;
// consumers see only its stable public readiness token.
#include "../src/descoberta.c"
#include <assert.h>

const char *i18n(const char *s) { return s; }
// Standalone text configuration; date localization is outside episode readiness.
int ajustes_idioma(void) { return 0; }
const char *idioma_mes_data(int mes, const char *nomePt) { (void)mes; return nomePt; }

static void sourceIds(void) {
  CatEp eps[3];
  const char *boa = "{\"meta\":{\"videos\":["
    "{\"id\":\"opaque-source-episode\",\"season\":2,\"episode\":1,\"tvdb_id\":10567809}]}}";
  assert(parsearEpisodios(boa, eps, 3) == 1);
  assert(eps[0].temporada == 2 && eps[0].episodio == 1 && eps[0].tvdbEp == 10567809);
  assert(!strcmp(eps[0].vid, "opaque-source-episode"));
  const char *pares = "{\"meta\":{\"videos\":["
    "{\"id\":\"first\",\"season\":2,\"episode\":1,\"tvdb_id\":10567809},"
    "{\"id\":\"different-number\",\"season\":1,\"episode\":13,\"tvdb_id\":10567809}]}}";
  assert(parsearEpisodios(pares, eps, 3) == 2);
  assert(eps[0].tvdbEp == 0 && eps[1].tvdbEp == 0);
  // A conflict beyond the output cap still disqualifies the retained ID.
  assert(parsearEpisodios(pares, eps, 1) == 1 && eps[0].tvdbEp == 0);
  assert(!strcmp(eps[0].vid, "first"));
  const char *ids = "{\"meta\":{\"videos\":["
    "{\"id\":\"first\",\"season\":2,\"episode\":1,\"tvdb_id\":10567809},"
    "{\"id\":\"second\",\"season\":2,\"episode\":1,\"tvdb_id\":11209906},"
    "{\"id\":\"third\",\"season\":2,\"episode\":2,\"tvdb_id\":10567809}]}}";
  assert(parsearEpisodios(ids, eps, 3) == 2);
  assert(eps[0].tvdbEp == 0 && eps[1].tvdbEp == 0);
  assert(!strcmp(eps[0].vid, "first"));
}

int main(void) {
  sourceIds();
  const char *id = "tt21975436", *origem = "org.aiometadata.fixture";
  uint64_t a = perfis_sessao_geracao();
  unsigned primeiro, segundo, novo;
  assert(desc_episodios_escolha_pronta(id, origem, a) == 0);
  primeiro = epEscolhaIniciar("tt21975436:1:14", origem, a);
  assert(primeiro && !desc_episodios_escolha_pronta(id, origem, a));
  // An intermediate list is not final merely because episodes now exist.
  assert(!desc_episodios_escolha_pronta("tt21975436:2:2", origem, a));
  epEscolhaConcluir(id, origem, a, primeiro);
  assert(desc_episodios_escolha_pronta("tt21975436:2:2", origem, a) == primeiro);
  assert(!desc_episodios_escolha_pronta(id, "org.other.fixture", a));
  assert(!desc_episodios_escolha_pronta("tt219754360", origem, a));
  assert(!desc_episodios_escolha_pronta("kitsu:21975436", origem, a));

  // A new request makes the old result pending before its first publication.
  segundo = epEscolhaIniciar(id, origem, a);
  assert(segundo && segundo != primeiro);
  assert(!desc_episodios_escolha_pronta(id, origem, a));
  epEscolhaConcluir(id, origem, a, primeiro); // Late tail from the old request.
  assert(!desc_episodios_escolha_pronta(id, origem, a));
  epEscolhaConcluir(id, origem, a, segundo);
  assert(desc_episodios_escolha_pronta(id, origem, a) == segundo);

  // A consumer must compare reads around its owned catalog snapshot.
  unsigned antes = desc_episodios_escolha_pronta(id, origem, a);
  novo = epEscolhaIniciar(id, origem, a);
  epEscolhaConcluir(id, origem, a, novo);
  assert(antes != desc_episodios_escolha_pronta(id, origem, a));

  // Reopening the same profile is a different visit, not permission for an
  // old completion or old source table to become current again.
  perfis_invalidar_sessao();
  uint64_t b = perfis_sessao_geracao();
  assert(a != b && !desc_episodios_escolha_pronta(id, origem, a));
  assert(!desc_episodios_escolha_pronta(id, origem, b));
  unsigned atual = epEscolhaIniciar(id, origem, b);
  epEscolhaConcluir(id, origem, a, novo);
  assert(!desc_episodios_escolha_pronta(id, origem, b));
  epEscolhaConcluir(id, origem, b, atual);
  assert(desc_episodios_escolha_pronta(id, origem, b) == atual);

  // The bounded table may forget an old result; forgetting never fabricates
  // readiness for a different title or accepts its outstanding old token.
  for (int i = 0; i < EP_ESCOLHAS_MAX + 1; i++) {
    char outro[64]; snprintf(outro, sizeof outro, "tt9%06d", i);
    unsigned token = epEscolhaIniciar(outro, origem, b);
    epEscolhaConcluir(outro, origem, b, token);
    assert(desc_episodios_escolha_pronta(outro, origem, b) == token);
  }
  assert(!desc_episodios_escolha_pronta(id, origem, b));
  epEscolhaConcluir(id, origem, b, atual);
  assert(!desc_episodios_escolha_pronta(id, origem, b));
  puts("episodio_escolha512: PASS");
  return 0;
}
