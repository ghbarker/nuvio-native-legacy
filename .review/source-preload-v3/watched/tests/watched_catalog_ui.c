// Issue #366: real ViewAll/Vitrine layout and focus logic with a synthetic
// already-loaded catalog. The shared movie-only predicate and history map
// are the actual descoberta.c/catalogo.c code; no network or GL context.
#include <assert.h>
#define desc_vertudo_n fixture_n
#define desc_vertudo_item fixture_item
#define desc_vertudo_n_bruto fixture_raw_n
#define desc_vertudo_item_bruto fixture_raw_item
#define desc_vertudo_revisao fixture_revision
#define desc_vertudo_mais fixture_more
#define desc_vertudo_carregando fixture_loading
#define desc_vertudo_fim fixture_end
#define desc_vertudo_erro fixture_error
#define desc_vertudo_filtro fixture_filter
#define desc_vertudo_fonte fixture_source
#define desc_chave_tmdb fixture_tmdb_key
#define desc_nome_catalogo fixture_catalog_name
#define desc_pedir_titulo_tmdb fixture_request_tmdb
#include "vertudo.c"
#undef desc_vertudo_n
#undef desc_vertudo_item
#undef desc_vertudo_n_bruto
#undef desc_vertudo_item_bruto
#undef desc_vertudo_revisao
#undef desc_vertudo_mais
#undef desc_vertudo_carregando
#undef desc_vertudo_fim
#undef desc_vertudo_erro
#undef desc_vertudo_filtro
#undef desc_vertudo_fonte
#undef desc_chave_tmdb
#undef desc_nome_catalogo
#undef desc_pedir_titulo_tmdb

static CatItem fixture[VT_MAX];
static int fixtureCount, hideWatched, projection[VT_MAX], projectionCount;
static unsigned fixtureGeneration, fixtureProjectionRevision;
static unsigned cachedHistory, cachedGeneration;
static int cachedHide = -1, rawReads, invalidReads, holdCancelled, moreRequests;
static int focusPhone;
static int loading, atEnd, catalogError, filtersRequested;
int ajustes_ocultar_filmes_vistos(void) { return hideWatched; }
int prog_tocou(const char *id) { return strcmp(id, "ttSeekOnly") != 0; }
Uint32 SDL_GetTicks(void) { return 1000; }
float ajustes_conteudo_x(void) { return 104.0f; }
float ajustes_rail_largura_fixa(void) { return 0; }
float gfx_escala_ui(void) { return 1.0f; }
#ifdef NV_TOUCH_UI
float nv_layout_w = 1920, nv_layout_h = 1080;
int layout_modo_mobile(void) { return focusPhone; }
#endif
void ctxhold_cancelar(CtxHold *h) { memset(h, 0, sizeof *h); holdCancelled++; }
const char *i18n(const char *s) { return s; }

static void projectFixture(void) {
  unsigned history = cat_revisao_itens();
  if (cachedHistory == history && cachedGeneration == fixtureGeneration && cachedHide == hideWatched) return;
  projectionCount = 0;
  for (int i = 0; i < fixtureCount; i++)
    if (!hideWatched || !desc_filme_visto(&fixture[i])) projection[projectionCount++] = i;
  cachedHistory = history; cachedGeneration = fixtureGeneration; cachedHide = hideWatched;
  fixtureProjectionRevision++;
}
int fixture_n(void) { projectFixture(); return projectionCount; }
unsigned fixture_revision(void) { projectFixture(); return fixtureProjectionRevision; }
int fixture_item(int i, CatItem *out) {
  projectFixture();
  if (i < 0 || i >= projectionCount || !out) return 0;
  *out = fixture[projection[i]]; return 1;
}
int fixture_raw_n(void) { return fixtureCount; }
void fixture_more(void) { moreRequests++; }
int fixture_loading(void) { return loading; }
int fixture_end(void) { return atEnd; }
int fixture_error(void) { return catalogError; }
const char *fixture_tmdb_key(void) { return ""; }
const char *fixture_catalog_name(const char *base, const char *type, const char *id) {
  (void)base; (void)type; (void)id; return "Synthetic catalog";
}
void fixture_filter(const char *base, const char *type, const char *id, const char *genre) {
  (void)base; (void)type; (void)id; (void)genre; filtersRequested++;
}
void fixture_source(const ColSource *source) { (void)source; filtersRequested++; }
void fixture_request_tmdb(long id, const char *type) { (void)id; (void)type; assert(!"Unexpected detail request"); }
int ctxhold_evento(CtxHold *h, const SDL_Event *event, int eligible) {
  (void)h; (void)event; (void)eligible; return CTXH_NADA;
}
int ctxhold_passo(CtxHold *h, Uint32 now, int feedback) {
  (void)h; (void)now; (void)feedback; return 0;
}
const char *addons_base_por_id(const char *id) { (void)id; return ""; }
const char *nuvem_trakt_cliente(void) { return ""; }
int salvos_perfil_atual(void) { return 1; }
char *dados_caminho(char *out, unsigned n, const char *file) { (void)out; (void)n; (void)file; return NULL; }
int dados_gravar(const char *file, const char *contents) {
  (void)file; (void)contents; return 0;
}
void col_nome_fonte(const ColSource *source, const char *manifest, char *out, unsigned n) {
  (void)source; snprintf(out, n, "%s", manifest);
}
void tex_orcamento_info(int *mb, long *memory, int *fixed, int *slots) {
  *mb = 64; *memory = 2048; *fixed = 0; *slots = 128;
}
int ptv_gpu_fraca_atual(void) { return 0; }
void home_vitrine_medida(float *width, float *height, float *step, float *label) {
  *width = 200; *height = 300; *step = 220; *label = 36;
}
int home_vitrine_arte_pronta(const CatItem *item) { (void)item; return 0; }
int ctxlista_indice(const char *id, long tmdb) { (void)id; (void)tmdb; return -1; }
int ctxlista_abrir(int index, GfxRect rect, const char *art) { (void)index; (void)rect; (void)art; return 0; }
const char *posterprov_card_addon(const char *origin, const char *id, long tmdb, const char *type, const char *original) {
  (void)origin; (void)id; (void)tmdb; (void)type; return original;
}
int fixture_raw_item(int i, CatItem *out) {
  rawReads++;
  if (i < 0 || i >= fixtureCount || !out) { invalidReads++; return 0; }
  *out = fixture[i]; return 1;
}
static void seed(int count, const char *prefix, const char *type, int year) {
  assert(count <= VT_MAX); fixtureCount = count;
  memset(fixture, 0, sizeof fixture);
  for (int i = 0; i < count; i++) {
    snprintf(fixture[i].imdb, sizeof fixture[i].imdb, "%s%d", prefix, i);
    snprintf(fixture[i].tipo, sizeof fixture[i].tipo, "%s", type);
    snprintf(fixture[i].meta, sizeof fixture[i].meta, "%d", year);
    snprintf(fixture[i].titulo, sizeof fixture[i].titulo, "Title %d", i);
  }
  fixtureGeneration++;
}
static void mark(int i, int watched) {
  cat_historico_definir_id(fixture[i].imdb, fixture[i].tipo, watched);
}
static void freshHistory(int profile) { cat_historico_contexto("synthetic-catalog-ui", profile); }
static void resetGrid(int layout) {
  collection = NULL; lay = layout; timeline = layout == VT_LAY_LINHA;
  orderN = -1; celN = celLay = -1; foco = 0;
  limparFocoCatalogo(); memset(&hold, 0, sizeof hold);
}
static void expectGridFocus(const char *id, int expectedIndex) {
  CatItem item;
  assert(foco == expectedIndex);
  assert(viewItem(foco, &item)); assert(!strcmp(item.imdb, id));
}
static void resetVitrine(int cap, int rows) {
  free(vitBuf);
  for (int r = 0; r < VIT_LIN_MAX; r++) { free(vitExtra[r]); vitExtra[r] = NULL; }
  assert(cap == VIT_ITENS || cap == VIT_ITENS_FRACA);
  vitCap = cap; vitBuf = calloc((size_t)VIT_LIN_MAX * cap, sizeof(CatItem)); assert(vitBuf);
  memset(vitLin, 0, sizeof vitLin); memset(vitVis, 0, sizeof vitVis);
  memset(vitColLemb, 0, sizeof vitColLemb); memset(&vitFocoChave, 0, sizeof vitFocoChave);
  vitOcultar = -1;
  vitProjetoSujo = 1; vitN = rows; vitFR = vitFC = 0;
  semFonte = 0; rawReads = invalidReads = 0;
}
static void expectRow(int row, const char *prefix, int start, int count) {
  assert(vitLin[row].n == count);
  for (int i = 0; i < count; i++) {
    char id[64]; snprintf(id, sizeof id, "%s%d", prefix, start + i);
    const CatItem *item = vitItem(row, i); assert(item); assert(!strcmp(item->imdb, id));
  }
  assert(!vitItem(row, -1)); assert(!vitItem(row, count));
}
static size_t allocatedVitrineSlots(void) {
  size_t slots = (size_t)VIT_LIN_MAX * vitCap;
  for (int r = 0; r < VIT_LIN_MAX; r++) if (vitExtra[r]) slots += vitCap;
  return slots;
}
static void runScenarios(void) {
  loading = atEnd = catalogError = 0;
  freshHistory(10); seed(2, "ttHiddenBatch", "movie", 2020); mark(0, 1); mark(1, 1);
  hideWatched = 1; semFonte = 0; resetGrid(VT_LAY_GRADE); aberta = 1; tabFocus = 0;
  assert(atualizarGrade() == 0 && temMaisVistos());
  int beforeMore = moreRequests;
  for (int frame = 0; frame < 8; frame++) vertudo_atualizar(0.016f, 1000 + frame);
  assert(moreRequests == beforeMore && foco == 0);
  SDL_Event emptyEvent = {0}; emptyEvent.type = SDL_KEYDOWN; emptyEvent.key.keysym.sym = SDLK_RIGHT;
  vertudo_evento(&emptyEvent); emptyEvent.key.keysym.sym = SDLK_DOWN; vertudo_evento(&emptyEvent);
  emptyEvent.type = SDL_KEYUP; emptyEvent.key.keysym.sym = SDLK_RETURN; vertudo_evento(&emptyEvent);
  assert(moreRequests == beforeMore);
  emptyEvent.type = SDL_KEYDOWN; vertudo_evento(&emptyEvent);
  assert(moreRequests == beforeMore + 1 && foco == 0);
  emptyEvent.key.repeat = 1; vertudo_evento(&emptyEvent); assert(moreRequests == beforeMore + 1);
  emptyEvent.key.repeat = 0; loading = 1; vertudo_evento(&emptyEvent);
  carregarMaisVistos(0, 0); assert(!temMaisVistos() && moreRequests == beforeMore + 1);
  loading = 0; ponteiroMaisVistos(0, 0); assert(moreRequests == beforeMore + 1);
  carregarMaisVistos(0, 0); assert(moreRequests == beforeMore + 2);
  static ColFolder emptyFolder; emptyFolder.nSources = 0; collection = &emptyFolder; tabFocus = 1;
  emptyEvent.key.keysym.sym = SDLK_DOWN; vertudo_evento(&emptyEvent);
  assert(!tabFocus && moreRequests == beforeMore + 2);
  emptyEvent.key.keysym.sym = SDLK_UP; vertudo_evento(&emptyEvent);
  assert(tabFocus && moreRequests == beforeMore + 2);
  emptyEvent.key.keysym.sym = SDLK_DOWN; vertudo_evento(&emptyEvent);
  assert(!tabFocus && moreRequests == beforeMore + 2);
  emptyEvent.key.keysym.sym = SDLK_LEFT; pedMenu = 0; vertudo_evento(&emptyEvent);
  assert(vertudo_pediu_menu() && moreRequests == beforeMore + 2);
  emptyEvent.key.keysym.sym = SDLK_ESCAPE; vertudo_evento(&emptyEvent);
  carregarMaisVistos(0, 0); assert(!aberta && moreRequests == beforeMore + 2);
  aberta = 1; collection = NULL; tabFocus = 0; atEnd = 1;
  emptyEvent.key.keysym.sym = SDLK_RETURN; vertudo_evento(&emptyEvent); carregarMaisVistos(0, 0);
  assert(!temMaisVistos() && moreRequests == beforeMore + 2);
  atEnd = 0; semFonte = 1; carregarMaisVistos(0, 0); assert(!temMaisVistos() && moreRequests == beforeMore + 2);
  semFonte = 0; catalogError = 1; assert(!temMaisVistos()); carregarMaisVistos(0, 0);
  assert(moreRequests == beforeMore + 2);
  vertudo_evento(&emptyEvent); assert(moreRequests == beforeMore + 3); // Existing error retry retained.
  catalogError = 0; seed(0, "ttGenuinelyEmpty", "movie", 2020);
  assert(!temMaisVistos()); vertudo_evento(&emptyEvent); carregarMaisVistos(0, 0);
  assert(moreRequests == beforeMore + 3);
  seed(2, "ttHiddenBatch", "movie", 2020); hideWatched = 0;
  assert(fixture_n() == 2 && !temMaisVistos()); carregarMaisVistos(0, 0);
  assert(moreRequests == beforeMore + 3);
  lay = VT_LAY_VITRINE; hideWatched = 1;
  assert(!fixture_n() && fixture_raw_n() == 2 && !temMaisVistos()); carregarMaisVistos(0, 0);
  assert(moreRequests == beforeMore + 3);
  puts("PASS hidden-page continuation: idle frames/navigation never fetch, one OK/tap requests one batch, repeats/loading blocked, tabs/back/end/error/genuine empty preserved");
  freshHistory(1); seed(4, "ttGrid", "movie", 2020); hideWatched = 1;
  resetGrid(VT_LAY_GRADE); assert(atualizarGrade() == 4); foco = 2; atualizarGrade();
  expectGridFocus("ttGrid2", 2);
  mark(0, 1); hold.armado = 1; int cancelled = holdCancelled;
  assert(atualizarGrade() == 3); expectGridFocus("ttGrid2", 1);
  assert(!hold.armado && holdCancelled == cancelled + 1);
  // Three remain, but the replacement shifts the surviving focused title.
  mark(0, 0); mark(3, 1); assert(atualizarGrade() == 3); expectGridFocus("ttGrid2", 2);
  mark(2, 1); assert(atualizarGrade() == 2); expectGridFocus("ttGrid1", 1);
  mark(0, 1); mark(1, 1); assert(atualizarGrade() == 0);
  assert(foco == 0 && !focoChave.id[0] && focoChave.tmdb == 0);
  CatItem empty; assert(!viewItem(foco, &empty));
  puts("PASS grid: same-count replacement, stable title focus, held OK cancelled, missing/empty focus clamped");

  freshHistory(7); seed(4, "ttPointer", "movie", 2020);
  resetGrid(VT_LAY_GRADE); assert(atualizarGrade() == 4); foco = 2; atualizarGrade();
  gradePonteiroValido = 1; gradePonteiroRevisao = gradeRevisao;
  mark(0, 1); int requestsBefore = moreRequests;
  ponteiroCartaz(0, 0); expectGridFocus("ttPointer2", 1);
  assert(moreRequests == requestsBefore);
  gradePonteiroRevisao = gradeRevisao; ponteiroCartaz(0, 0);
  expectGridFocus("ttPointer1", 0); assert(moreRequests == requestsBefore + 1);
  puts("PASS pointer: stale drawn index rejected after projection change, fresh target accepts click without changing title identity");

  freshHistory(9); seed(4, "ttIdentity", "movie", 2020);
  fixture[1].imdb[0] = 0; fixture[1].tmdb = 742; fixtureGeneration++;
  resetGrid(VT_LAY_GRADE); assert(atualizarGrade() == 4); foco = 1; atualizarGrade();
  assert(!focoChave.id[0] && focoChave.tmdb == 742);
  mark(0, 1); assert(atualizarGrade() == 3); expectGridFocus("", 0);
  assert(viewItem(foco, &empty) && empty.tmdb == 742);
  puts("PASS title identity: TMDB-only title preserves focused identity when earlier movie disappears");

  // Sorting invalidates even if the new projection has the old item count.
  freshHistory(2); seed(4, "ttYear", "movie", 2020);
  snprintf(fixture[0].meta, sizeof fixture[0].meta, "2030");
  snprintf(fixture[1].meta, sizeof fixture[1].meta, "1990");
  snprintf(fixture[2].meta, sizeof fixture[2].meta, "2000");
  snprintf(fixture[3].meta, sizeof fixture[3].meta, "2010"); fixtureGeneration++;
  resetGrid(VT_LAY_LINHA); assert(atualizarGrade() == 4); foco = 1; atualizarGrade();
  expectGridFocus("ttYear2", 1);
  mark(1, 1); assert(atualizarGrade() == 3); expectGridFocus("ttYear2", 0);
  mark(1, 0); mark(0, 1); assert(atualizarGrade() == 3); expectGridFocus("ttYear2", 1);
  assert(orderN == 3 && order[0] == 0 && order[1] == 1 && order[2] == 2);
  puts("PASS chronological layouts: sorted projection rebuilt on revision, surviving focus preserved");

  freshHistory(3); seed(5, "ttMixed", "movie", 2020);
  strcpy(fixture[1].tipo, "series"); strcpy(fixture[2].tipo, "anime");
  strcpy(fixture[3].tipo, ""); fixtureGeneration++;
  for (int i = 0; i < 5; i++) mark(i, 1);
  resetGrid(VT_LAY_GRADE); assert(atualizarGrade() == 3);
  for (int i = 0; i < 3; i++) { CatItem item; assert(viewItem(i, &item)); assert(strcmp(item.tipo, "movie")); }
  hideWatched = 0; assert(atualizarGrade() == 5);
  puts("PASS media scope: exact movies hidden; watched series, unknown types and empty type retained; toggle restores raw order");

  freshHistory(4); seed(60, "ttCache", "movie", 2020); hideWatched = 0;
  resetVitrine(VIT_ITENS, 1); vitGuardar(0); vitProjetar();
  expectRow(0, "ttCache", 0, VIT_ITENS);
  assert(!vitExtra[0] && vitLin[0].brutoN == VIT_ITENS && vitLin[0].extraN == 0);
  assert(allocatedVitrineSlots() == (size_t)VIT_LIN_MAX * VIT_ITENS);
  for (int i = 0; i < 5; i++) mark(i, 1);
  vitGuardar(0); vitProjetar(); assert(vitExtra[0]);
  assert(vitLin[0].extraN == VIT_ITENS && allocatedVitrineSlots() == (size_t)(VIT_LIN_MAX + 1) * VIT_ITENS);
  CatItem *extraAllocation = vitExtra[0];
  CatItem baseline[VIT_ITENS]; memcpy(baseline, vitBuf, sizeof baseline);
  hideWatched = 1; vitProjetar(); expectRow(0, "ttCache", 5, VIT_ITENS);
  vitFC = 5; vitLembrarFoco(); assert(!strcmp(vitItem(0, vitFC)->imdb, "ttCache10"));
  vitHeroAtual = 10; vitHeroAnt = 9; vitHeroDes = 11;
  mark(5, 1); vitProjetar(); assert(vitFC == 4 && !strcmp(vitItem(0, vitFC)->imdb, "ttCache10"));
  assert(vitHeroAtual == -1 && vitHeroAnt == -1 && vitHeroDes == -1);
  mark(1, 0); vitProjetar(); assert(vitFC == 5 && !strcmp(vitItem(0, vitFC)->imdb, "ttCache10"));
  hideWatched = 0; vitProjetar(); expectRow(0, "ttCache", 0, VIT_ITENS);
  assert(!memcmp(baseline, vitBuf, sizeof baseline));
  for (int i = 0; i < 100; i++) { hideWatched = !hideWatched; vitProjetar(); }
  assert(vitExtra[0] == extraAllocation && !invalidReads);
  puts("PASS Vitrine: raw first-cap cache preserved, received deeper links fill holes, disabled restores original first-cap, live marks keep focus and reset heroes");

  // Same source/title in two rows must retain its own row, not jump rows.
  resetVitrine(VIT_ITENS_FRACA, 2); hideWatched = 1; seed(30, "ttRows", "movie", 2020);
  for (int i = 0; i < 3; i++) mark(i, 1);
  vitGuardar(0); vitGuardar(1); vitProjetar();
  vitFR = 1; vitFC = 4; vitLembrarFoco(); mark(3, 1); vitProjetar();
  assert(vitFR == 1 && !strcmp(vitItem(1, vitFC)->imdb, "ttRows7"));
  assert(allocatedVitrineSlots() == (size_t)(VIT_LIN_MAX + 2) * VIT_ITENS_FRACA);
  for (int i = 0; i < 30; i++) mark(i, 1);
  vitProjetar(); assert(!vitTemItens() && vitFC == 0 && !vitItem(vitFR, vitFC));
  assert(vitLinhaVizinha(vitFR, 1) == -1 && vitLinhaVizinha(vitFR, -1) == -1);
  assert(!vitItem(-1, 0) && !vitItem(vitN, 0));
  hideWatched = 0; vitProjetar(); expectRow(0, "ttRows", 0, VIT_ITENS_FRACA); expectRow(1, "ttRows", 0, VIT_ITENS_FRACA);
  puts("PASS weak-device Vitrine: 12-card cap, duplicate title focus stays in same row, all-hidden rows safe, toggle restores both sources");

  freshHistory(5); seed(80, "ttGroup", "movie", 2020);
  for (int i = 40; i < 80; i++) strcpy(fixture[i].tipo, "series");
  fixtureGeneration++;
  for (int i = 0; i < 5; i++) mark(i, 1);
  for (int i = 40; i < 80; i++) mark(i, 1);
  hideWatched = 1; resetVitrine(VIT_ITENS, 0); vitAgrupar(fixtureCount); vitProjetar();
  assert(vitN == 2); expectRow(0, "ttGroup", 5, VIT_ITENS); expectRow(1, "ttGroup", 40, VIT_ITENS);
  assert(vitExtra[0] && !vitExtra[1]);
  hideWatched = 0; vitProjetar(); expectRow(0, "ttGroup", 0, VIT_ITENS); expectRow(1, "ttGroup", 40, VIT_ITENS);
  puts("PASS grouped Vitrine: original raw movie/series rows preserved, movie row filled before cap, watched series remain");

  freshHistory(6); seed(50, "ttDecade", "movie", 2020);
  for (int i = 30; i < 50; i++) strcpy(fixture[i].meta, "1990");
  fixtureGeneration++;
  for (int i = 0; i < 4; i++) mark(i, 1);
  for (int i = 30; i < 50; i++) mark(i, 1);
  hideWatched = 1; resetVitrine(VIT_ITENS, 0); vitAgrupar(fixtureCount); vitProjetar();
  assert(vitN == 2); expectRow(0, "ttDecade", 4, VIT_ITENS); assert(vitLin[1].n == 0);
  vitFR = 1; vitFC = 0; vitLembrarFoco(); vitProjetoSujo = 1; vitProjetar();
  assert(vitFR == 0 && vitItem(vitFR, vitFC));
  hideWatched = 0; vitProjetar(); expectRow(0, "ttDecade", 0, VIT_ITENS); expectRow(1, "ttDecade", 30, VIT_ITENS);
  assert(!invalidReads);
  puts("PASS decade Vitrine: empty watched decade skipped, off restores first-cap of original groups");

  freshHistory(8); seed(80, "ttBound", "movie", 2020);
  for (int i = 0; i < VIT_ITENS; i++) mark(i, 1);
  hideWatched = 1; resetVitrine(VIT_ITENS, VIT_LIN_MAX);
  for (int r = 0; r < VIT_LIN_MAX; r++) vitGuardar(r);
  vitProjetar();
  assert(allocatedVitrineSlots() == (size_t)2 * VIT_LIN_MAX * VIT_ITENS);
  for (int r = 0; r < VIT_LIN_MAX; r++) {
    assert(vitLin[r].brutoN == VIT_ITENS && vitLin[r].extraN == VIT_ITENS);
    expectRow(r, "ttBound", VIT_ITENS, VIT_ITENS);
    CatItem *extra = vitExtra[r]; vitGuardar(r); assert(vitExtra[r] == extra);
  }
  assert(allocatedVitrineSlots() == (size_t)2 * VIT_LIN_MAX * VIT_ITENS && !invalidReads);
  puts("PASS allocation bound: all 12 rows fill, each extra capped at 20, repeated storage reuses buffers");

  for (int r = 0; r < VIT_LIN_MAX; r++) { free(vitExtra[r]); vitExtra[r] = NULL; }
  free(vitBuf); vitBuf = NULL;
  printf("watched_catalog_ui: all scenarios passed (CatItem=%zu bytes, max bounded baseline+extras=%zu bytes)\n", sizeof(CatItem), (size_t)2 * VIT_LIN_MAX * VIT_ITENS * sizeof(CatItem));
}
int main(void) {
  puts("Catalog UI runtime mode: TV");
  runScenarios();
#ifdef NV_TOUCH_UI
  focusPhone = 1; nv_layout_w = 1080; nv_layout_h = 1920;
  puts("Catalog UI runtime mode: Mobile portrait");
  runScenarios();
#endif
  return 0;
}
