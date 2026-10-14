/* Real localized metadata -> real posplay release gate -> real asynchronous
 * Next controller -> one-shot automatic request and prepared source transfer.
 * Source/resolver/native playback boundaries are the original fixture doubles. */
#include "next_ready_posplay_joined.c"
#include "idiomacod.h"
int fixture_episodes(int, const char *, CatEp *);

static int releaseThenAdvance(int language) {
  resetJoined(); autoNext = 1;
  assert(fixture_episodes(language, "2023-06-25T00:00:00Z", episodeList) == 2);
  frameAt(credits - 60);
  /* Return a distinct failure instead of asserting, so the exact same
   * English scenario proves the immutable Logs1 baseline rejects preload. */
  for (int i=0; i<20 && !atomic_load(&consultas); i++) usleep(1000);
  if (!atomic_load(&consultas)) {
    fprintf(stderr, "RELEASE-GATE FAILURE locale=%d display='%s': no metadata query or native preload\n",
            language, episodeList[1].data);
    return 42;
  }
  prontoFonte();
  unsigned token = tokenAtual;
  assert(token && iniciaBuffer == 1 && resolucoes == 1 && reservas == 4 && http == 4);
  posplay_marcador_real(1);
  frameAt(credits);
  assert(posplay_visivel());
  int t=0, e=0, advanced=0;
  for (int i=0; i<20 && !advanced; i++) {
    frameAt(credits + i);
    advanced = posplay_pediu_episodio(&t, &e);
  }
  assert(advanced && t == 1 && e == 2);
  assert(!posplay_pediu_episodio(&t, &e));
  ProximoPreparoResultado result;
  assert(proximopreparo_consumir("tt42:1:2", &result));
  assert(result.token == token && result.indiceFonte >= 0);
  assert(fontefalha_tentativas(result.politica) == 4);
  assert(result.sessaoPerfil == perfis_sessao_geracao());
  assert(!proximopreparo_consumir("tt42:1:2", &(ProximoPreparoResultado){0}));
  assert(iniciaBuffer == 1 && !cancelaBuffer && !renova && reservas == 4 && http == 4);
  proximopreparo_liberar_resultado(&result);
  printf("PASS joined locale %d: localized release starts buffer before credits, auto Next transfers original token once\n",language);
  return 0;
}

int main(int argc, char **argv) {
  ui = pthread_self();
  int result = releaseThenAdvance(IDIOMA_EN);
  if (result) return result;
  if (argc > 1 && !strcmp(argv[1], "--baseline-control")) return 0;
  for (int language=0; language<IDIOMA_N; language++)
    if (language != IDIOMA_EN && (result = releaseThenAdvance(language))) return result;

  const char *unreleased[] = {"2099-01-01T00:00:00Z", NULL, "", "2023", "not-a-release"};
  for (unsigned i=0; i<sizeof unreleased/sizeof *unreleased; i++) {
    resetJoined(); autoNext = 1;
    assert(fixture_episodes(IDIOMA_EN, unreleased[i], episodeList) == 2);
#ifdef NV_DATE_RAW_FIELD
    if (unreleased[i] && unreleased[i][0])
      strcpy(episodeList[1].data,"2020-01-01"); /* raw unknown/future must win */
#endif
    for (int j=0; j<20; j++) frameAt(credits - 60 + j);
    assert(!consultas && !resolucoes && !iniciaBuffer && !reservas && !http);
  }
  puts("PASS joined negative gates: future/missing/year-only/malformed raw date never spends speculative source or native work");
  posplay_fechar(); proximopreparo_encerrar();
  return 0;
}
