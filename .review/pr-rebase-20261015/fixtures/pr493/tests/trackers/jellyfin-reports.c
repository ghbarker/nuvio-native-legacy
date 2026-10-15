#include "server-network.h"
#include "jellyfin.c"
static void changeProfile(JfInst *i) {
  pthread_mutex_lock(&i->trava); avancarLocked(i);
  snprintf(i->conta.base, sizeof i->conta.base, "http://server-B"); snprintf(i->conta.token, sizeof i->conta.token, "token-B");
  i->estado = JF_EST_CONECTADO; pthread_mutex_unlock(&i->trava);
}
static void testReports(int delayed, int authFail) {
  JfInst i = {.tipo=JF_TIPO_JELLYFIN, .log="jellyfin", .arqFmt="jellyfin-p%d.txt", .geracao=1, .fiosVivos=2,
    .trava=PTHREAD_MUTEX_INITIALIZER, .sinalCtl=PTHREAD_COND_INITIALIZER, .sinalRel=PTHREAD_COND_INITIALIZER};
  i.grupo = rede_grupo_criar(); i.grupoRel = rede_grupo_criar();
  snprintf(i.conta.base, sizeof i.conta.base, "http://server-A"); snprintf(i.conta.token, sizeof i.conta.token, "token-A");
  Sessao s = {.geracao=1}; s.s.metodo = JF_METODO_TRANSCODE;
  snprintf(s.s.itemId, sizeof s.s.itemId, "item-A"); snprintf(s.s.sessaoId, sizeof s.s.sessaoId, "session-A");
  networkReset(delayed, authFail);
  pthread_mutex_lock(&i.trava); relatarLocked(&i, JF_REL_FIM, &s, 300); pthread_mutex_unlock(&i.trava);
  if (!delayed) changeProfile(&i);
  pthread_t worker; pthread_create(&worker, NULL, trabalharRel, &i);
  if (delayed) { waitEntered(); changeProfile(&i); releaseNetwork(); }
  for (int k = 0; k < 300 && jfi_relatorios_pendentes(&i); k++) usleep(10000);
  assert(!jfi_relatorios_pendentes(&i));
  pthread_mutex_lock(&i.trava); assert(i.estado == JF_EST_CONECTADO && !strcmp(i.conta.token, "token-B"));
  i.parar = 1; pthread_cond_broadcast(&i.sinalRel); pthread_mutex_unlock(&i.trava); pthread_join(worker, NULL);
  assert(requests == (authFail ? 1 : 2)); // stop + explicit transcode release
  rede_grupo_soltar(i.grupo); rede_grupo_soltar(i.grupoRel);
  pthread_mutex_destroy(&i.trava); pthread_cond_destroy(&i.sinalCtl); pthread_cond_destroy(&i.sinalRel);
  printf("PASS Jellyfin frozen final stop %s, auth=%s; original server/token retained, new account state intact\n", delayed ? "in flight" : "queued", authFail ? "401" : "200");
}
int main(void) { testReports(0, 0); testReports(1, 0); testReports(1, 1); return 0; }
