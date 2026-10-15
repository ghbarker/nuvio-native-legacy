// Additional cross-product behavior; existing catalog assertions stay unchanged.
#define main original_watched_catalog_main
#include "watched_catalog_ui.c"
#undef main
static void checkMode(int choice) {
  freshHistory(40 + choice); seed(10, "ttEight", "movie", 2020);
  for (int i = 0; i < 10; i++) snprintf(fixture[i].meta, sizeof fixture[i].meta, "%d", 1980 + ((i * 7) % 30));
  strcpy(fixture[6].tipo, "series"); strcpy(fixture[8].tipo, "anime"); fixture[9].tipo[0] = 0;
  fixtureGeneration++;
  mark(1, 1); mark(3, 1); mark(6, 1); mark(8, 1); mark(9, 1);
  hideWatched = 0;
  if (choice == VT_LAY_VITRINE) {
    resetVitrine(VIT_ITENS, 1); vitGuardar(0); vitProjetar();
    assert(vitLin[0].n == 10); vitFC = 5; vitLembrarFoco();
    hideWatched = 1; vitProjetar(); assert(vitLin[0].n == 8);
    assert(!strcmp(vitItem(0, vitFC)->imdb, "ttEight5"));
    for (int i = 0; i < vitLin[0].n; i++) {
      const CatItem *item = vitItem(0, i); assert(item);
      assert(strcmp(item->imdb, "ttEight1") && strcmp(item->imdb, "ttEight3"));
    }
    hideWatched = 0; vitProjetar(); expectRow(0, "ttEight", 0, 10);
  } else {
    resetGrid(choice == VT_LAY_AUTO ? layAuto() : choice);
    assert(atualizarGrade() == 10);
    CatItem item;
    for (int i = 0; i < 10; i++) { assert(viewItem(i, &item)); if (!strcmp(item.imdb, "ttEight5")) foco = i; }
    atualizarGrade(); hideWatched = 1; assert(atualizarGrade() == 8);
    assert(viewItem(foco, &item) && !strcmp(item.imdb, "ttEight5"));
    for (int i = 0; i < 8; i++) {
      assert(viewItem(i, &item));
      assert(strcmp(item.imdb, "ttEight1") && strcmp(item.imdb, "ttEight3"));
    }
    hideWatched = 0; assert(atualizarGrade() == 10);
    assert(viewItem(foco, &item) && !strcmp(item.imdb, "ttEight5"));
  }
  printf("PASS display %d: movie-only hiding, raw restoration and title focus\n", choice);
}
int main(void) {
  for (int mode = 0; mode <
#ifdef NV_TOUCH_UI
      2
#else
      1
#endif
      ; mode++) {
#ifdef NV_TOUCH_UI
    focusPhone = mode; nv_layout_w = mode ? 1080 : 1920; nv_layout_h = mode ? 1920 : 1080;
#endif
    for (int choice = VT_LAY_AUTO; choice < VT_LAY_N; choice++) checkMode(choice);
  }
  for (int r = 0; r < VIT_LIN_MAX; r++) { free(vitExtra[r]); vitExtra[r] = NULL; }
  free(vitBuf); vitBuf = NULL;
  return 0;
}
