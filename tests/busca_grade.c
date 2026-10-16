// Search See All uses the real grid, while pages/menu/network are typed
// boundaries. Reuse the existing collection fixture's seeded reader and layout.
#define main colecao_fixture_main
#define desc_vertudo_busca grade_busca_abrir
#define ctxlista_abrir grade_menu_abrir
#define ctxlista_indice grade_menu_indice
#define desc_pedir_titulo_tmdb grade_tmdb_pedir
#include "colecao_exibicao.c"
#undef main
#undef desc_vertudo_busca
#undef ctxlista_abrir
#undef ctxlista_indice
#undef desc_pedir_titulo_tmdb
#include "segurar.h"

static int gradeAlvo, gradeGeracao, gradePedidos, gradeMenu = -1;
static int gradeConversoes, gradeMenuNumerico;
static char gradeTermo[96];
int grade_busca_abrir(int alvo, int geracao, const char *termo) {
  gradeAlvo = alvo; gradeGeracao = geracao; gradePedidos++;
  snprintf(gradeTermo, sizeof gradeTermo, "%s", termo);
  return 1;
}
int grade_menu_abrir(int idx, GfxRect rect, const char *arte) {
  assert(rect.w > 0 && rect.h > 0 && arte);
  assert(idx >= 0 && idx < cat_n()); gradeMenu = idx; return 1;
}
int grade_menu_indice(const char *id, long tmdb) {
  assert(id && tmdb > 0); gradeMenuNumerico++; return 0;
}
void grade_tmdb_pedir(long id, const char *tipo) {
  assert(id > 0 && tipo); gradeConversoes++;
}
static void gradeTecla(Uint32 tipo, SDL_Keycode tecla) {
  SDL_Event e = {0}; e.type = tipo; e.key.keysym.sym = tecla;
  vertudo_evento(&e);
}
static void gradeMesmoItem(int idx, int pos) {
  const CatItem *it = cat_item(idx);
  assert(it && !strcmp(it->imdb, semente[pos].imdb));
  assert(!strcmp(it->tipo, "series"));
  assert(!strcmp(it->origem, "https://search.invalid/exact-addon"));
}

void testarGradeBusca(void) {
  for (int mobile = 0; mobile <= 1; mobile++) {
    layout_tela_definir(mobile ? 1080 : 1920, mobile ? 2340 : 1080);
    layout_modo_definir(mobile);
    CatItem outros[3] = {{0}};
    CatItem *outro = &outros[0];
    // A wrong-type item with the same numeric TMDB must not own the result's
    // menu. Search uses the raw ID/type/origin tuple, like its first24 items.
    strcpy(outro->imdb, "tt0000563"); strcpy(outro->titulo, "Unrelated movie");
    strcpy(outro->tipo, "movie"); outro->tmdb = 9024;
    outros[1] = *outro; strcpy(outros[1].imdb, "tmdb:9024");
    strcpy(outros[1].tipo, "series"); strcpy(outros[1].origem, "Other addon");
    outros[2] = outros[1]; strcpy(outros[2].tipo, "movie");
    strcpy(outros[2].origem, "https://search.invalid/exact-addon");
    cat_definir_tudo(outros, 3, NULL, 0); cat_quadro();
    memset(semente, 0, sizeof semente); nSemente = 40;
    for (int i = 0; i < nSemente; i++) {
      snprintf(semente[i].imdb, sizeof semente[i].imdb, "tmdb:%d", 9000+i);
      snprintf(semente[i].titulo, sizeof semente[i].titulo, "Search series %d", i);
      strcpy(semente[i].tipo, "series"); semente[i].tmdb = 9000+i;
      strcpy(semente[i].origem, "https://search.invalid/exact-addon");
    }
    gradePedidos = gradeConversoes = gradeMenuNumerico = 0;
    gradeMenu = -1; loading = 0; exhausted = 1; hideWatched = allWatched = 1;
    assert(vertudo_busca(1, 37, "bigg boss", "Same label · Addon"));
    assert(gradePedidos == 1 && gradeAlvo == 1 && gradeGeracao == 37);
    assert(!strcmp(gradeTermo, "bigg boss"));
    assert(buscaModo && !collection && lay == VT_LAY_GRADE);
    assert(!strcmp(titulo, "Same label · Addon"));
    assert(atualizarGrade() == 40);  // collection-only watched filtering stays out
    for (int pos = 23; pos <= 24; pos++) {
      foco = pos; lembrarFoco();
      gradeTecla(SDL_KEYDOWN, SDLK_RETURN);
      assert(vertudo_pediu_abrir() == -1);
      gradeTecla(SDL_KEYUP, SDLK_RETURN);
      int idx = vertudo_pediu_abrir();
      gradeMesmoItem(idx, pos);
      assert(vertudo_pediu_abrir() == -1);
      assert(gradeConversoes == 0 && gradeMenuNumerico == 0);
      // Detail Back returns to this still-open grid, retaining its selection.
      assert(vertudo_aberta() && vertudo_foco() == pos);
    }
    foco = 24; lembrarFoco();
    gradeTecla(SDL_KEYDOWN, SDLK_RETURN);
    assert(hold.armado && gradeMenu == -1);
    if (!hold.desde) hold.desde = 1;
    Uint32 agora = hold.desde + NV_HOLD_MS + 1u;
    segurar_quadro(agora);
    vertudo_atualizar(0.0f, agora);
    gradeMesmoItem(gradeMenu, 24);
    assert(gradeMenu >= 3 && gradeMenuNumerico == 0 && gradeConversoes == 0);
    gradeTecla(SDL_KEYUP, SDLK_RETURN);
    assert(vertudo_pediu_abrir() == -1);
    gradeTecla(SDL_KEYDOWN, SDLK_ESCAPE);
    assert(!vertudo_aberta());
    // Existing collection entries retain their dedicated TMDB title resolver.
    buscaModo = 0; abrirItem(&semente[24]);
    assert(gradeConversoes == 1);
  }
  hideWatched = allWatched = 0;
  layout_tela_definir(1920, 1080); layout_modo_definir(0);
  puts("PASS: Search See All grid opens and menus raw 24th/25th IDs in TV and Mobile");
}
