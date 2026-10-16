// #391 reported-title verification, without changing metadata-source priority.
// Includes the established fake-service fixture, but executes the real worker
// and linked catalog store. No network or account credentials are used.
#define main detalheanime_existing_main
#include "detalheanime.c"
#undef main

static char *fixture391(const char *name) {
  char path[256], *body;
  FILE *file;
  long size;
  snprintf(path, sizeof path, "tests/fixtures/issue391/%s", name);
  file = fopen(path, "rb");
  assert(file);
  assert(fseek(file, 0, SEEK_END) == 0);
  size = ftell(file);
  assert(size > 0 && size <= 2 * 1024 * 1024);
  rewind(file);
  body = malloc((size_t)size + 1);
  assert(body);
  assert(fread(body, 1, (size_t)size, file) == (size_t)size);
  body[size] = 0;
  assert(fclose(file) == 0);
  return body;
}

static int moved391;
static void moveTitle391(const char *url) {
  CatItem items[2];
  if (moved391 || (!strstr(url, "catalog.nuvio.tv") && !strstr(url, "cinemeta"))) return;
  moved391 = 1;
  memset(items, 0, sizeof items);
  snprintf(items[0].imdb, sizeof items[0].imdb, "tt0111161");
  snprintf(items[0].tipo, sizeof items[0].tipo, "movie");
  snprintf(items[0].titulo, sizeof items[0].titulo, "Unrelated title");
  assert(cat_copiar_item(0, &items[1]));
  cat_definir_tudo(items, 2, NULL, 0);
}

static void reportedTitle391(int fallback, int rebuild) {
  const int countsPrimary[] = {25, 22, 26, 26, 26, 26, 25};
  const int countsFallback[] = {26, 22, 26, 26, 26, 26, 26};
  const int *expected = fallback ? countsFallback : countsPrimary;
  const char *plotPrimary = "Follow the intergalactic adventures of Capt. Jean-Luc Picard";
  const char *plotFallback = "Set almost 100 years after Captain Kirk's five-year mission";
  char *primary = fixture391("nuvio_tt0092455.json");
  char *backup = fixture391("cinemeta_tt0092455.json");
  char stream[64];
  int season, episode, counts[7] = {0}, index = rebuild ? 1 : 0;
  int total = fallback ? 178 : 176;
  const CatItem *item;
  limparCacheMeta(); metaprov_zerar_pausa();
  nFake = addonMeta = nRotas = 0; addonResp = cineSerie = NULL;
  fakeMetaExterno = fakeTmdbBasico = fakeTmdbArte = fakeSoCinemeta = 0;
  fakeIdioma = "en-US"; tmdbChave[0] = 0;
  tmdbFind = tmdbTemp1 = tmdbFilme = NULL; caudaB = NULL;
  rota("catalog.nuvio.tv", fallback ? NULL : primary);
  rota("v3-cinemeta.strem.io", backup);
  catalogoDe("tt0092455", "series", "Star Trek: The Next Generation", "fixture.report391");
  moved391 = 0; pedidoMetaHook = rebuild ? moveTitle391 : NULL;
  abrir();
  pedidoMetaHook = NULL;
  assert(pediu("catalog.nuvio.tv"));
  assert(pediu("v3-cinemeta.strem.io") == fallback);
  assert(cat_n_episodios(index) == total);
  item = cat_item(index);
  assert(item && !strcmp(item->imdb, "tt0092455"));
  assert(!strcmp(item->tipo, "series"));
  assert(!strcmp(item->origem, "fixture.report391"));
  assert(!strcmp(item->titulo, "Star Trek: The Next Generation"));
  assert(strstr(item->sinopse, fallback ? plotFallback : plotPrimary));
  assert(item->nTemporadas == 7);
  for (season = 1; season <= 7; season++) {
    assert(item->temporadas[season - 1] == season);
  }
  for (episode = 0; episode < total; episode++) {
    const CatEp *ep = cat_episodio(index, episode);
    assert(ep && ep->temporada >= 1 && ep->temporada <= 7);
    counts[ep->temporada - 1]++;
  }
  for (season = 0; season < 7; season++) assert(counts[season] == expected[season]);
  // The attached photograph has season 2 selected with a blank episode row.
  // Verify every reported-season episode, not just its season tab or a count.
  for (episode = 1; episode <= 22; episode++) {
    int i, found = 0;
    for (i = 0; i < total; i++) {
      const CatEp *ep = cat_episodio(index, i);
      if (ep->temporada == 2 && ep->episodio == episode) {
        char expectedId[64];
        snprintf(expectedId, sizeof expectedId, "tt0092455:2:%d", episode);
        assert(!strcmp(ep->vid, expectedId));
        assert(ep->nome[0]);
        assert(cat_id_stream(index, 2, episode, stream, sizeof stream));
        assert(!strcmp(stream, expectedId));
        found++;
      }
    }
    assert(found == 1);
  }
  if (rebuild) {
    assert(moved391);
    assert(cat_n_episodios(0) == 0 && cat_item(0)->nTemporadas == 0);
    assert(!strcmp(cat_item(0)->titulo, "Unrelated title"));
  }
  printf("episode_report391: %s%s: %d episodes, 7 season tabs, all 22 season-2 ids preserved\n",
         fallback ? "live-cinemeta" : "live-nuvio", rebuild ? "-rebuild" : "", total);
  limparCacheMeta();
  free(primary); free(backup);
}

int main(int argc, char **argv) {
  const char *scenario = argc > 1 ? argv[1] : "";
  if (!strcmp(scenario, "live-nuvio")) reportedTitle391(0, 0);
  else if (!strcmp(scenario, "live-cinemeta")) reportedTitle391(1, 0);
  else if (!strcmp(scenario, "live-nuvio-rebuild")) reportedTitle391(0, 1);
  else if (!strcmp(scenario, "live-cinemeta-rebuild")) reportedTitle391(1, 1);
  else { fprintf(stderr, "Unknown scenario: %s\n", scenario); return 2; }
  return 0;
}
