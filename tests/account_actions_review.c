/* The Guide's real metadata and action handlers, with inert external UI. */
#define TESTE_AJUSTES 1
#define main account_actions_existing_main
#include "menus_toque.c"
#undef main

static int miniFreed, guideSearches;
void gfx_mini_liberar(GfxMini *m) { (void)m; miniFreed++; }
void spot_abrir_guia(void) { guideSearches++; }

static void back(void) {
  SDL_Event e = {0}; e.type = SDL_KEYDOWN; e.key.keysym.sym = SDLK_AC_BACK;
  guiaEvento(&e);
}
static void openResource(int e) {
  guiaAbrir(0);
  guiaPontIndiceAbrir(GUIA_ENT[e].cap + GI_CAP0, 0);
  assert(gv.col == GC_LISTA);
  guiaPontEntAbrir(e, 0);
  assert(gv.ent == e && gv.col == GC_BOTOES && guiaAberto);
}
int main(void) {
  montarTela();
  const float telas[][2] = {{1080,2340},{1080,1920},{2340,1080},{2520,1080}};
  int resources = 0, actions = 0, examples = 0, settings = 0, screens = 0;
  int keyPages = 0;
  for (int t = 0; t < 4; t++) {
    nv_layout_w = telas[t][0]; nv_layout_h = telas[t][1];
    for (int e = 0; e < GUIA_NENT; e++) {
      int bs[3], nb = guiaBotoes(e, bs);
      openResource(e); resources++;
      for (int key = 0; key < 2; key++) {
        gv.col = GC_LISTA;
        SDL_Event ev = {0}; ev.type = SDL_KEYDOWN;
        ev.key.keysym.sym = key ? SDLK_RIGHT : SDLK_RETURN;
        guiaEvento(&ev);
        assert(guiaAberto && gv.ent == e && gv.col == GC_BOTOES && !gv.botao);
        keyPages++;
      }
      /* A wrong chapter cannot replace the resource selected on this page. */
      int other = (GUIA_ENT[e].cap + 1) % GUIA_NCAPS;
      int invalid = guiaPrimeira(GI_CAP0 + other);
      guiaPontEnt(invalid, 0); assert(gv.ent == e && gv.col == GC_BOTOES);
      guiaPontEntAbrir(invalid, 0); assert(gv.ent == e && gv.col == GC_BOTOES);
      guiaPontBotao(0, invalid); assert(gv.ent == e && gv.col == GC_BOTOES && guiaAberto);
      /* Both native Back and the drawn Back callback traverse the pages. */
      back(); assert(guiaAberto && gv.col == GC_LISTA && gv.ent == e);
      guiaPontVoltar(1, 0); assert(guiaAberto && gv.col == GC_INDICE);
      back(); assert(!guiaAberto && focoOp == AJ_GUIA);
      for (int b = 0; b < nb; b++) {
        openResource(e); pediuTela = 0; actions++;
        guiaPontBotao(b, e);
        if (bs[b] == GB_EXEMPLO) {
          assert(guiaAberto && guiaExemplo == GUIA_ENT[e].exemplo); examples++;
          back(); assert(guiaAberto && !guiaExemplo && gv.ent == e && gv.col == GC_BOTOES);
        } else if (bs[b] == GB_AJUSTES) {
          assert(!guiaAberto && focoOp == GUIA_ENT[e].alvo && !focoIndice); settings++;
        } else {
          assert(!guiaAberto && ajustes_pediu_tela() == GUIA_ENT[e].tela); screens++;
          assert(!ajustes_pediu_tela());
        }
      }
    }
    guiaAbrir(1); pediuNovidades = 0;
    back(); assert(guiaAberto && gv.col == GC_INDICE);
    back(); assert(!guiaAberto && pediuNovidades);
    guiaAbrir(0); guiaDoIndice = 1; focoIndice = 0;
    guiaPontVoltar(0, 0); assert(!guiaAberto && focoIndice);
    guiaAbrir(0); guiaPontIndiceAbrir(GI_BUSCA, 0); assert(guideSearches == t + 1);
    /* Rotating the canvas changes geometry, not the selected resource. */
    openResource(0); int idx = gv.idx;
    nv_layout_w = telas[(t + 1) % 4][0]; nv_layout_h = telas[(t + 1) % 4][1];
    assert(guiaTelefone() && gv.idx == idx && gv.ent == 0 && gv.col == GC_BOTOES);
    guiaFechar();
  }
  /* A tablet retains the side-inspector/remote behavior for a no-action entry. */
  testMobile = 0;
  nv_layout_w = 1728; nv_layout_h = 1080; assert(!guiaTelefone());
  guiaAbrir(0); gv.ent = 0; gv.col = GC_LISTA;
  SDL_Event ev = {0}; ev.type = SDL_KEYDOWN; ev.key.keysym.sym = SDLK_RETURN;
  guiaEvento(&ev); assert(gv.col == GC_LISTA && guiaAberto);
  assert(resources == GUIA_NENT * 4 && GUIA_NENT == 104 && miniFreed > 0);
  printf("account_actions_review: %d resource pages, %d keyboard detail transitions, %d actions (%d examples, %d settings, %d screens), Back/origin/rotation/tablet OK\n",
         resources, keyPages, actions, examples, settings, screens);
}
