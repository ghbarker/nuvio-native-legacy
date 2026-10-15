#include "server-network.h"
#include "plex.c"
static void changeProfile(void) {
  pthread_mutex_lock(&trava); avancarLocked();
  snprintf(conta.base, sizeof conta.base, "http://server-B"); snprintf(conta.token, sizeof conta.token, "token-B");
  estado = PX_EST_CONECTADO; pthread_mutex_unlock(&trava);
}
static void testReports(int delayed, int authFail) {
  grupo = rede_grupo_criar(); grupoRel = rede_grupo_criar(); geracao = 1; fiosVivos = 2; parar = 0;
  snprintf(conta.base, sizeof conta.base, "http://server-A"); snprintf(conta.token, sizeof conta.token, "token-A");
  Sessao s = {.geracao=1}; s.s.metodo = PX_METODO_TRANSCODE;
  snprintf(s.s.ratingKey, sizeof s.s.ratingKey, "123"); snprintf(s.s.sessaoId, sizeof s.s.sessaoId, "session-A");
  networkReset(delayed, authFail);
  pthread_mutex_lock(&trava); relatarLocked(PX_REL_FIM, &s, 300); pthread_mutex_unlock(&trava);
  if (!delayed) changeProfile();
  pthread_t worker; pthread_create(&worker, NULL, trabalharRel, NULL);
  if (delayed) { waitEntered(); changeProfile(); releaseNetwork(); }
  for (int k = 0; k < 300 && plex_relatorios_pendentes(); k++) usleep(10000);
  assert(!plex_relatorios_pendentes());
  pthread_mutex_lock(&trava); assert(estado == PX_EST_CONECTADO && !strcmp(conta.token, "token-B"));
  parar = 1; pthread_cond_broadcast(&sinalRel); pthread_mutex_unlock(&trava); pthread_join(worker, NULL);
  assert(requests == (authFail ? 1 : 2));
  rede_grupo_soltar(grupo); rede_grupo_soltar(grupoRel); grupo = grupoRel = NULL;
  printf("PASS Plex frozen final stop %s, auth=%s; original server/token retained, new account state intact\n", delayed ? "in flight" : "queued", authFail ? "401" : "200");
}
int main(void) { testReports(0, 0); testReports(1, 0); testReports(1, 1); return 0; }
