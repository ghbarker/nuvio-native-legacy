/* Review-only executable. The unchanged upstream selector regression supplies
 * its existing integration stand-ins; all model/row/event code is production. */
#define main upstream_selector_main
#include SOURCE_SELECTOR_TEST
#undef main

static void title_at(const char *key, const char *raw) {
  char name[96], detail[512]; const char *language, *icon; int active, dimmed;
  int row = linhaDe(key);
  assert(row >= 0);
  textoLinha(&linhas[row], name, sizeof name, detail, sizeof detail,
             &language, &icon, &active, &dimmed);
  assert(!strcmp(name, i18n(ling_nome("en"))));
  assert(language && !strcmp(language, "en"));
  assert(!strncmp(detail, raw, strlen(raw)));
  assert(strstr(detail, "Embutida"));
  assert(!strstr(name, "Full") && !strstr(name, "Signs"));
}

static void full_key(int index, char out[96]) {
  char id[24]; legendasui_id_embutida(&emb[index], id);
  snprintf(out, 96, "m|%s", id);
}

int main(void) {
  assert(upstream_selector_main() == 0);
  legendasui_reiniciar();
  nEmb = 3; nAdd = 0; todas = 0; ativaEmb = 0; extId[0] = 0;
  ling_local_legenda("en"); ling_local_legenda2("");
  faixa(0, 21, "en", "Ingles generated", "S_TEXT/ASS", 0);
  faixa(1, 22, "en", "Ingles generated", "S_TEXT/ASS", 0);
  faixa(2, 23, "en", "Ingles generated", "S_TEXT/ASS", 1);
  snprintf(emb[0].nome, sizeof emb[0].nome, "Full");
  snprintf(emb[1].nome, sizeof emb[1].nome, "Full honorifics");
  snprintf(emb[2].nome, sizeof emb[2].nome, "Signs & Songs");
  emb[0].tipoLeg = emb[1].tipoLeg = LING_LEG_COMPLETA;
  emb[2].tipoLeg = LING_LEG_LETREIROS;
  legendasui_abrir();
  assert(!mais && !ver);
  title_at("rec|p", "Full");

  /* All and per-language versions keep distinct, exact container names. */
  focarChave("mais"); tecla(SDLK_RETURN);
  assert(mais);
  char k0[96], k1[96], k2[96];
  full_key(0, k0); full_key(1, k1); full_key(2, k2);
  title_at(k0, "Full"); title_at(k1, "Full honorifics");
  title_at(k2, "Signs & Songs");
  assert(strcmp(k0, k1) && strcmp(k1, k2));
  ver = 1; snprintf(verIdioma, sizeof verIdioma, "en"); montarLinhas();
  title_at(k0, "Full"); title_at(k1, "Full honorifics");

  /* Updating only a display name preserves identity, focus and selection. */
  focarChave(k1);
  snprintf(emb[1].nome, sizeof emb[1].nome, "Full honorifics — 日本語 🎵");
  montarLinhas();
  assert(!strcmp(linhas[foco].chave, k1));
  title_at(k1, "Full honorifics — 日本語 🎵");
  tecla(SDLK_RETURN); assert(escolhidaEmb == 1);
  assert(emb[escolhidaEmb].numero == 22);

  /* Backend reordering resolves the same number to its new list index. */
  VideoFaixa swap = emb[0]; emb[0] = emb[1]; emb[1] = swap;
  ativaEmb = 0; montarLinhas(); focarChave(k1);
  assert(cand[linhas[foco].cand].indice == 0 && !strcmp(linhas[foco].chave, k1));
  ativaEmb = -1; escolhidaEmb = -99; agoraMs += 1000;
  tecla(SDLK_RETURN); assert(escolhidaEmb == 0);
  assert(emb[escolhidaEmb].numero == 22);
  title_at(k1, "Full honorifics — 日本語 🎵");

  /* A real title equal to the language is retained; it is not inferred. */
  snprintf(emb[0].nome, sizeof emb[0].nome, "English");
  montarLinhas(); title_at(k1, "English");

  /* Missing titles preserve the language and metadata, not generated labels. */
  emb[0].nome[0] = 0; montarLinhas();
  char name[96], detail[512]; const char *language, *icon; int active, dimmed;
  textoLinha(&linhas[linhaDe(k1)], name, sizeof name, detail, sizeof detail,
             &language, &icon, &active, &dimmed);
  assert(!strcmp(name, i18n(ling_nome("en"))));
  assert(strstr(detail, "Embutida") && !strstr(detail, "generated"));

  /* An unlabeled language still has an accessible translated track fallback. */
  emb[0].idioma[0] = 0; ver = 0; montarLinhas();
  textoLinha(&linhas[linhaDe(k1)], name, sizeof name, detail, sizeof detail,
             &language, &icon, &active, &dimmed);
  char fallback[96]; snprintf(fallback, sizeof fallback, i18n("Legenda %d"), 1);
  assert(!strcmp(name, fallback));
  assert(!strstr(detail, "generated"));

  puts("title rows: upstream selector plus language/title/versions/fallback/identity checks passed");
  return 0;
}
