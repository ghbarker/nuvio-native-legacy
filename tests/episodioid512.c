// Stable episode identity is independent from each source's season labels.
#include "episodioid.h"
#include "traktscrobble.h"
#include "traktult.h"
#include <assert.h>
#include <stdio.h>

static void lerId(void) {
  assert(episodioid_tvdb("{\"tvdb_id\":10567809}", NULL) == 10567809);
  assert(episodioid_tvdb("{\"ids\":{\"trakt\":1,\"tvdb\":10567809}}", NULL) == 10567809);
  assert(episodioid_tvdb("{\"tvdb_id\":10567809,\"ids\":{\"tvdb\":10567809}}", NULL) == 10567809);
  assert(episodioid_tvdb("{\"name\":\"tvdb_id\",\"tvdb_id\":10567809}", NULL) == 10567809);
  assert(episodioid_tvdb("{\"show\":{\"ids\":{\"tvdb\":383397}}}", NULL) == 0);
  const char *invalidos[] = {
    "{\"tvdb_id\":0}", "{\"tvdb_id\":-1}", "{\"tvdb_id\":1.5}",
    "{\"tvdb_id\":1e3}", "{\"tvdb_id\":2147483648}", "{\"tvdb_id\":\"10567809\"}",
    "{\"tvdb_id\":null}", "{\"tvdb_id\":1,\"ids\":{\"tvdb\":2}}",
    "{\"tvdb_id\":\"bad\",\"ids\":{\"tvdb\":10567809}}", "{}"
  };
  for (unsigned i = 0; i < sizeof invalidos / sizeof *invalidos; i++)
    assert(episodioid_tvdb(invalidos[i], NULL) == 0);
}

static void fonte(void) {
  CatEp eps[3] = {{0}};
  EpisodioId r = {0};
  eps[0].temporada = 2; eps[0].episodio = 1; eps[0].tvdbEp = 10567809;
  eps[1].temporada = 2; eps[1].episodio = 2; eps[1].tvdbEp = 11209906;
  eps[2] = eps[1];
  assert(episodioid_procurar(eps, 3, 11209906, &r));
  assert(r.temporada == 2 && r.episodio == 2);
  eps[2].temporada = 1; eps[2].episodio = 14;
  assert(!episodioid_procurar(eps, 3, 11209906, &r));
  assert(!episodioid_procurar(eps, 3, 99999999, &r));
  eps[1].temporada = 0;
  assert(!episodioid_procurar(eps, 2, 11209906, &r));
}

static void metadata(void) {
  EpisodioId r;
  const char *boa = "{\"meta\":{\"id\":\"tt21975436\",\"type\":\"series\",\"videos\":["
    "{\"id\":\"source:kaiju:2\",\"season\":2,\"episode\":2,\"tvdb_id\":11209906}]}}";
  assert(episodioid_meta(boa, "tt21975436", 11209906, &r));
  assert(r.temporada == 2 && r.episodio == 2);
  assert(!episodioid_meta(boa, "tt00000000", 11209906, &r));
  assert(!episodioid_meta("{\"meta\":{\"type\":\"movie\",\"videos\":["
    "{\"season\":2,\"episode\":2,\"tvdb_id\":11209906}]}}", "tt21975436", 11209906, &r));
  assert(!episodioid_meta("{\"meta\":{\"nested\":{\"videos\":["
    "{\"season\":2,\"episode\":2,\"tvdb_id\":11209906}]}}}", "tt21975436", 11209906, &r));
  assert(!episodioid_meta("{\"meta\":{\"videos\":["
    "{\"season\":2,\"episode\":2,\"tvdb_id\":11209906},"
    "{\"season\":1,\"episode\":14,\"tvdb_id\":11209906}]}}", "tt21975436", 11209906, &r));
}

static void inverseAmbiguity(void) {
  CatEp eps[3] = {{0}};
  EpisodioId r = {77, 88, 99};
  eps[0].temporada = eps[1].temporada = eps[2].temporada = 3;
  eps[0].episodio = eps[1].episodio = 1; eps[2].episodio = 2;
  eps[0].tvdbEp = 201; eps[1].tvdbEp = 202; eps[2].tvdbEp = 203;
  assert(!episodioid_procurar(eps, 3, 201, &r));
  assert(r.temporada == 77 && r.episodio == 88 && r.tvdbEp == 99);
  assert(!episodioid_procurar(eps, 3, 202, &r));
  assert(episodioid_procurar(eps, 3, 203, &r) && r.temporada == 3 && r.episodio == 2);
  // Repeating the same proven identity still describes one source episode.
  eps[1].tvdbEp = 201;
  assert(episodioid_procurar(eps, 3, 201, &r) && r.temporada == 3 && r.episodio == 1);

  const char *conflito = "{\"meta\":{\"videos\":["
    "{\"season\":3,\"episode\":1,\"tvdb_id\":201},"
    "{\"season\":3,\"episode\":1,\"tvdb_id\":202},"
    "{\"season\":3,\"episode\":2,\"tvdb_id\":203}]}}";
  assert(!episodioid_meta(conflito, "tt21975436", 201, &r));
  assert(!episodioid_meta(conflito, "tt21975436", 202, &r));
  assert(episodioid_meta(conflito, "tt21975436", 203, &r) && r.temporada == 3 && r.episodio == 2);
  assert(episodioid_meta("{\"meta\":{\"videos\":["
    "{\"season\":3,\"episode\":1,\"tvdb_id\":201},"
    "{\"season\":3,\"episode\":1,\"tvdb_id\":201}]}}", "tt21975436", 201, &r));
}

static void corpo(void) {
  char b[400], legado[400];
  TraktScrobbleAlvo a = {"tt21975436:2:1", 10567809};
  scrobble_corpo_alvo(b, sizeof b, &a, 40);
  assert(!strcmp(b, "{\"episode\":{\"ids\":{\"tvdb\":10567809}},\"progress\":40.00}"));
  assert(!strstr(b, "\"show\"") && !strstr(b, "\"season\""));
  a.tvdbEp = 0;
  scrobble_corpo_alvo(b, sizeof b, &a, 40);
  scrobble_corpo(legado, sizeof legado, a.id, 40);
  assert(!strcmp(b, legado));
  snprintf(a.id, sizeof a.id, "tt0371746");
  scrobble_corpo_alvo(b, sizeof b, &a, 40);
  scrobble_corpo(legado, sizeof legado, a.id, 40);
  assert(!strcmp(b, legado) && strstr(b, "\"movie\""));
  a.tvdbEp = 10567809;
  scrobble_corpo_alvo(b, sizeof b, &a, 150);
  assert(strstr(b, "\"progress\":100.00"));
}

static void historico(void) {
  TkUltimo v[2] = {0};
  int n = 0;
  assert(tk_ult_anotar_tvdb(v, &n, 2, "tt21975436", 1, 13, 1000, 10567809));
  assert(v[0].tvdbEp == 10567809);
  assert(!tk_ult_anotar_tvdb(v, &n, 2, "tt21975436", 1, 12, 500, 10364437));
  assert(v[0].tvdbEp == 10567809);
  assert(tk_ult_anotar(v, &n, 2, "tt21975436", 1, 14, 2000));
  assert(v[0].tvdbEp == 0); // Unknown newer identity never inherits the old ID.
}

int main(void) {
  lerId(); fonte(); metadata(); inverseAmbiguity(); corpo(); historico();
  puts("episodioid512: PASS");
  return 0;
}
