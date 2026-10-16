// #524: keep a known series identity when opening a title outside the catalog.
// The two reduced Cinemeta responses reproduce the actual service collision:
// series/tt9054364 is Slime; movie/tt9054364 is Chasing Mavericks. Both echo
// the requested IMDb id and have moviedb_id 82684, so that number alone does
// not prove the media type. No live service, account, or test-only app hook.
#define main detalheanime_existing_main
#include "detalheanime.c"
#undef main

static const char *SLIME_SERIES =
  "{\"meta\":{\"id\":\"tt9054364\",\"imdb_id\":\"tt9054364\",\"type\":\"series\","
  "\"name\":\"That Time I Got Reincarnated as a Slime\",\"moviedb_id\":82684,"
  "\"poster\":\"https://fixture.invalid/slime.jpg\",\"description\":\"Series synopsis\","
  "\"videos\":[{\"id\":\"tt9054364:1:1\",\"season\":1,\"episode\":1,\"name\":\"Storm Dragon\"},"
  "{\"id\":\"tt9054364:1:2\",\"season\":1,\"episode\":2,\"name\":\"Meeting the Goblins\"}]}}";
static const char *CHASING_WRONG_ID =
  "{\"meta\":{\"id\":\"tt9054364\",\"imdb_id\":\"tt9054364\",\"type\":\"movie\","
  "\"name\":\"Chasing Mavericks\",\"moviedb_id\":82684,\"releaseInfo\":\"2012\","
  "\"poster\":\"https://fixture.invalid/chasing.jpg\",\"description\":\"Movie synopsis\",\"videos\":[]}}";
static const char *CHASING_MOVIE =
  "{\"meta\":{\"id\":\"tt1629757\",\"imdb_id\":\"tt1629757\",\"type\":\"movie\","
  "\"name\":\"Chasing Mavericks\",\"moviedb_id\":82684,\"releaseInfo\":\"2012\","
  "\"poster\":\"https://fixture.invalid/chasing.jpg\",\"description\":\"Movie synopsis\",\"videos\":[]}}";

static void setup(void) {
  limparCacheMeta();
  metaprov_zerar_pausa();
  cat_definir_tudo(NULL, 0, NULL, 0);
  addonMeta = nFake = nPedidos = nRotas = 0;
  fakeMetaExterno = fakeTmdbBasico = fakeTmdbArte = fakeSoCinemeta = 0;
  fakeIdioma = "en-US";
  desc_tmdb_definir("00000000000000000000000000000524");
  sobIndice = -1; sobTmdb = 0; sobFioVivo = sobTemSemente = 0;
  sobId[0] = sobTipo[0] = 0;
  rota("/tv/82684/external_ids?", "{\"imdb_id\":\"tt9054364\"}");
  rota("/movie/82684/external_ids?", "{\"imdb_id\":\"tt1629757\"}");
  rota("catalog.nuvio.tv", "{\"meta\":{}}");
  rota("/meta/series/tt9054364.json", SLIME_SERIES);
  rota("/meta/movie/tt9054364.json", CHASING_WRONG_ID);
  rota("/meta/movie/tt1629757.json", CHASING_MOVIE);
}

static int awaitTitle(const char *id, const char *type, const char *title) {
  for (int i = 0; i < 5000 && desc_titulo_buscando(); i++) SDL_Delay(1);
  int ix = desc_titulo_pronto();
  const CatItem *ci = ix >= 0 ? cat_item(ix) : NULL;
  printf("result id=%s type=%s title=%s index=%d requests=%d\n",
         ci ? ci->imdb : "(missing)", ci ? ci->tipo : "(missing)",
         ci ? ci->titulo : "(missing)", ix, nPedidos);
  for (int i = 0; i < nPedidos && i < 64; i++) printf("request %s\n", pedidos[i]);
  return ci && !strcmp(ci->imdb, id) && !strcmp(ci->tipo, type) && !strcmp(ci->titulo, title);
}

int main(int argc, char **argv) {
  const char *mode = argc > 1 ? argv[1] : "";
  setup();
  if (!strcmp(mode, "tmdb-tv")) {
    // A TV credit already tells us the media type. Resolving its external IMDb
    // id must not switch to the numerically colliding TMDB movie.
    desc_pedir_titulo_tmdb(82684, "tv");
    return !awaitTitle("tt9054364", "series", "That Time I Got Reincarnated as a Slime");
  }
  if (!strcmp(mode, "episode-id")) {
    // Activity/Continue Watching keys explicitly carry episode coordinates.
    // Looking up the title must retain that evidence after removing the suffix.
    desc_pedir_titulo("tt9054364:1:2");
    return !awaitTitle("tt9054364", "series", "That Time I Got Reincarnated as a Slime");
  }
  if (!strcmp(mode, "movie-control")) {
    // Movie 82684 and TV 82684 are different titles. A correct fix must keep
    // the movie route, not globally coerce anime-shaped/numeric ids to series.
    desc_pedir_titulo_tmdb(82684, "movie");
    return !awaitTitle("tt1629757", "movie", "Chasing Mavericks");
  }
  if (!strcmp(mode, "known-series-control")) {
    catalogoDe("tt9054364", "series", "That Time I Got Reincarnated as a Slime", "");
    abrir();
    const CatItem *ci = cat_item(0);
    printf("series detail title=%s type=%s episodes=%d movie-requested=%d\n",
           ci->titulo, ci->tipo, cat_n_episodios(0), pediu("/meta/movie/tt9054364.json"));
    return strcmp(ci->tipo, "series") || strcmp(ci->titulo, "That Time I Got Reincarnated as a Slime") ||
           cat_n_episodios(0) != 2 || pediu("/meta/movie/tt9054364.json");
  }
  if (!strcmp(mode, "uncertain-detail-control")) {
    catalogoDe("tt9054364", "anime", "That Time I Got Reincarnated as a Slime", "");
    abrir();
    const CatItem *ci = cat_item(0);
    printf("uncertain detail title=%s type=%s episodes=%d movie-requested=%d\n",
           ci->titulo, ci->tipo, cat_n_episodios(0), pediu("/meta/movie/tt9054364.json"));
    return strcmp(ci->tipo, "series") || strcmp(ci->titulo, "That Time I Got Reincarnated as a Slime") ||
           cat_n_episodios(0) != 2 || pediu("/meta/movie/tt9054364.json");
  }
  fprintf(stderr, "unknown scenario: %s\n", mode);
  return 2;
}
