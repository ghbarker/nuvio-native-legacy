// Reuse established Home CPU doubles, but call real Home/history/settings/focus.
#define main existing_homepos_main
#include "homepos.c"
#undef main
#include <stdio.h>
void desc_ocultar_vistos_atualizar(void) {}
const char *ling_legenda(void) { return ""; }
const char *ling_audio(void) { return ""; }
const char *ling_conta_legenda_valor(void) { return ""; }
const char *ling_conta_legenda2_valor(void) { return ""; }
const char *ling_conta_audio_valor(void) { return ""; }
void ling_conta_legenda(const char *v) { (void)v; }
void ling_conta_legenda2(const char *v) { (void)v; }
void ling_conta_audio(const char *v) { (void)v; }
int fonteregra_do_blob(const char *json) { (void)json; return 0; }

static void row(CatFileira *f, const char *key, const char *type, int start, int count, int discovery) {
  memset(f, 0, sizeof *f);
  snprintf(f->chave, sizeof f->chave, "%s", key);
  snprintf(f->titulo, sizeof f->titulo, "%s", key);
  snprintf(f->tipo, sizeof f->tipo, "%s", type);
  if (discovery) {
    snprintf(f->base, sizeof f->base, "https://provider.invalid/%s", key);
    snprintf(f->catId, sizeof f->catId, "%s", key);
  }
  f->ini = start; f->n = count;
}
static void expected(const char *key, const int *indices, int n) {
  int r = idx(key);
  assert(r >= 0 && fileiras[r].n == n);
  for (int c = 0; c < n; c++) {
    int got = fileiraItemIndice(&fileiras[r], c);
    if (got != indices[c]) fprintf(stderr, "row %s col %d expected raw %d got %d\n", key, c, indices[c], got);
    assert(got == indices[c]);
  }
}
static void heroVisible(void) {
  heroSetGarantir();
  for (int i = 0; i < heroSetN; i++) {
    const CatItem *item = cat_item(heroSet[i]);
    assert(item && (strcmp(item->tipo, "movie") || !cat_visto(item)));
  }
}
int main(void) {
  assert(!ajustes_ocultar_filmes_vistos());
  fil_definir_limite(FIL_LIMITE_MAX);
  ajustes_aplicar_blob("{\"continueWatchingEnabled\":true}");
  CatItem *it = calloc(48, sizeof *it);
  CatFileira rows[5]; assert(it);
  for (int i = 0; i < 48; i++) {
    snprintf(it[i].imdb, sizeof it[i].imdb, "ttHome%d", i);
    snprintf(it[i].titulo, sizeof it[i].titulo, "Home %d", i);
    snprintf(it[i].tipo, sizeof it[i].tipo, "movie");
  }
  for (int i = 24; i < 27; i++) strcpy(it[i].tipo, "series");
  row(&rows[0], "providerA", "movie", 0, 10, 1);
  row(&rows[1], "providerB", "movie", 10, 10, 1);
  row(&rows[2], "continue_watching", "movie", 20, 2, 0);
  row(&rows[3], "watchlist", "movie", 22, 2, 0);
  row(&rows[4], "seriesDiscovery", "series", 24, 3, 1);
  for (int i = 0; i < 4; i++) cat_historico_definir_id(it[i].imdb, "movie", 1);
  cat_historico_definir_id(it[11].imdb, "movie", 1);
  for (int i = 20; i < 24; i++) cat_historico_definir_id(it[i].imdb, "movie", 1);
  for (int i = 24; i < 27; i++) cat_historico_definir_id(it[i].imdb, "series", 1);
  montar(it, rows, 5);
  const int originalA[] = {0,1,2,3,4,5,6,7,8,9};
  expected("providerA", originalA, 10);
  int ra = idx("providerA"), rb = idx("providerB");
  assert(ra < rb);
  float height = alturaTotalFil(ra);
  // Focus identity survives hidden cards before it. Watch history does not
  // mutate metadata or provider priority; every underlying item remains.
  focoHero = 0; foco.fileira = ra; foco.coluna = 7;
  foco.colunaLembrada[ra] = 7; posCapturar();
  ajustes_definir_ocultar_filmes_vistos(1);
  sincronizarFileiras();
  const int filteredA[] = {4,5,6,7,8,9};
  const int filteredB[] = {10,12,13,14,15,16,17,18,19};
  const int cw[] = {20,21}, library[] = {22,23}, series[] = {24,25,26};
  expected("providerA", filteredA, 6); expected("providerB", filteredB, 9);
  expected("continue_watching", cw, 2); expected("watchlist", library, 2);
  expected("seriesDiscovery", series, 3);
  assert(cat_n() == 48 && cat_n_fileiras() == 5 && cat_fileira(0)->n == 10);
  assert(idx("providerA") < idx("providerB"));
  assert(foco.fileira == idx("providerA") && foco.coluna == 3);
  assert(fileiraItemIndice(&fileiras[foco.fileira], foco.coluna) == 7);
  assert(alturaTotalFil(idx("providerA")) == height);
  heroVisible();
  puts("PASS Home: default off, movie catalogs only, raw metadata retained, provider order, focus identity and row height");

  // A status-only mutation refreshes projection despite unchanged rows/counts.
  posCapturar(); cat_historico_definir_id(it[4].imdb, "movie", 1);
  sincronizarFileiras();
  const int markedA[] = {5,6,7,8,9}; expected("providerA", markedA, 5);
  assert(foco.coluna == 2 && fileiraItemIndice(&fileiras[foco.fileira], foco.coluna) == 7);
  heroVisible();
  cat_historico_definir_id(it[4].imdb, "movie", 0);
  sincronizarFileiras(); expected("providerA", filteredA, 6);
  // A removed focused title clamps to a valid visible neighbor.
  foco.fileira = idx("providerB"); foco.coluna = 8; posCapturar();
  cat_historico_definir_id(it[19].imdb, "movie", 1);
  sincronizarFileiras();
  assert(foco.fileira == idx("providerB") && foco.coluna < fileiras[foco.fileira].n + fileiras[foco.fileira].verTudo);
  if (foco.coluna < fileiras[foco.fileira].n)
    assert(fileiraItemIndice(&fileiras[foco.fileira], foco.coluna) >= 0);
  puts("PASS Home: live mark/unmark refresh and removal clamps focus without catalog reload");

  // Disabling is reversible and restores provider order and IDs in memory.
  ajustes_definir_ocultar_filmes_vistos(0); sincronizarFileiras();
  expected("providerA", originalA, 10);
  const int originalB[] = {10,11,12,13,14,15,16,17,18,19};
  expected("providerB", originalB, 10);
  assert(cat_n() == 48);
  puts("PASS Home: disabling restores all titles immediately");

  // Under Dinamica, short first row borrows a visible third option from the
  // next catalog. The original and borrowed projections must both be honored.
  rows[0].n = 4; rows[1].n = 4;
  cat_historico_definir_id(it[2].imdb, "movie", 0);
  cat_historico_definir_id(it[3].imdb, "movie", 0);
  cat_historico_definir_id(it[10].imdb, "movie", 1);
  assert(ajustes_shot_valor("homeLayoutLocal", HOME_LAYOUT_DINAMICA));
  ajustes_definir_ocultar_filmes_vistos(1);
  montar(it, rows, 5);
  // If the configured layout exposes the destaque, the third option must be
  // raw12, never hidden raw0/raw1/raw10/raw11.
  Fileira *first = &fileiras[idx("providerA")];
  assert(first->tipo == FILEIRA_DESTAQUE);
  const int destaque[] = {2,3,12}; expected("providerA", destaque, 3);
  heroVisible();
  puts("PASS Home: short-row destaque augmentation honors visible indices");
  // Keep the all-hidden catalog's View All affordance, preserving row height
  // and allowing its later pages to expose unwatched movies.
  for (int i = 0; i < 4; i++) cat_historico_definir_id(it[i].imdb, "movie", 1);
  for (int i = 10; i < 14; i++) cat_historico_definir_id(it[i].imdb, "movie", 1);
  // No third discovery catalog is available to supply a visible series.
  rows[4].base[0] = 0; rows[4].catId[0] = 0;
  montar(it, rows, 5);
  first = &fileiras[idx("providerA")];
  assert(first->n == 0 && first->verTudo == 1);
  assert(alturaTotalFil(idx("providerA")) > 0);
  assert(foco.fileira >= 0 && foco.fileira < nFileiras);
  assert(foco.coluna >= 0 && foco.coluna < foco.nColunas[foco.fileira]);
  heroVisible();
  ajustes_definir_ocultar_filmes_vistos(0); sincronizarFileiras();
  const int restoredShort[] = {0,1,2,3}; expected("providerA", restoredShort, 4);
  puts("PASS Home: all-hidden row retains View All and focus safety; disabling restores raw first cap");
  free(it);
  puts("watched_home: all scenarios passed");
  return 0;
}
