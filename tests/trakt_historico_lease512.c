// #512: the production profile/credential/history gate must cover publication.
// Only the external profile and history epochs are controlled here. The whole
// credential-state block, mutexes, validation and public callback API are real.
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static pthread_mutex_t perfilTrava = PTHREAD_MUTEX_INITIALIZER;
static _Atomic uint64_t perfil = 1;
static _Atomic unsigned long long mapaTeste = 1;
static uint64_t nv_perfil_sessao_ler(void) { return perfil; }
static int nv_perfil_sessao_travar(uint64_t g) {
  pthread_mutex_lock(&perfilTrava);
  if (g != perfil) { pthread_mutex_unlock(&perfilTrava); return 0; }
  return 1;
}
static void nv_perfil_sessao_destravar(void) { pthread_mutex_unlock(&perfilTrava); }
unsigned long long cat_historico_geracao(void) { return mapaTeste; }
int cat_historico_definir_se_geracao(const char *id, const char *tipo, int v, unsigned long long g) {
  (void)id; (void)tipo; (void)v; return g == mapaTeste;
}
#include "lease-actual.inc"

static int publicados;
static int publicar(void *p) {
  assert(pthread_mutex_trylock(&perfilTrava) == EBUSY);
  assert(pthread_mutex_trylock(&travaCred) == EBUSY);
  publicados++;
  return *(int *)p;
}
static void livres(void) {
  assert(!pthread_mutex_trylock(&perfilTrava)); pthread_mutex_unlock(&perfilTrava);
  assert(!pthread_mutex_trylock(&travaCred)); pthread_mutex_unlock(&travaCred);
}
static struct {
  pthread_mutex_t trava;
  pthread_cond_t cv;
  int capturou, mudou, entrou, tentou, resultado;
  unsigned long long cred;
} corrida = { .trava = PTHREAD_MUTEX_INITIALIZER, .cv = PTHREAD_COND_INITIALIZER };
static void esperar(int *valor) { // caller holds corrida.trava
  struct timespec fim;
  assert(!clock_gettime(CLOCK_REALTIME, &fim)); fim.tv_sec += 5;
  while (!*valor) assert(!pthread_cond_timedwait(&corrida.cv, &corrida.trava, &fim));
}
static void *respostaAntiga(void *u) {
  (void)u;
  unsigned long long antes = trakt_credencial_geracao();
  pthread_mutex_lock(&corrida.trava);
  corrida.cred = antes; corrida.capturou = 1; pthread_cond_broadcast(&corrida.cv);
  esperar(&corrida.mudou); pthread_mutex_unlock(&corrida.trava);
  int valor = 7;
  corrida.resultado = trakt_historico_aplicar_se_atual(1, 1, antes, publicar, &valor);
  return NULL;
}
static void *trocarDurantePublicacao(void *u) {
  (void)u;
  pthread_mutex_lock(&corrida.trava); esperar(&corrida.entrou); pthread_mutex_unlock(&corrida.trava);
  // The callback is entered, so a credential change must wait at the REAL lock.
  assert(pthread_mutex_trylock(&travaCred) == EBUSY);
  pthread_mutex_lock(&corrida.trava); corrida.tentou = 1;
  pthread_cond_broadcast(&corrida.cv); pthread_mutex_unlock(&corrida.trava);
  pthread_mutex_lock(&travaCred); credGeracao++; pthread_mutex_unlock(&travaCred);
  pthread_mutex_lock(&corrida.trava); corrida.mudou = 1;
  pthread_cond_broadcast(&corrida.cv); pthread_mutex_unlock(&corrida.trava);
  return NULL;
}
static int publicarComConcorrencia(void *p) {
  pthread_mutex_lock(&corrida.trava); corrida.entrou = 1;
  pthread_cond_broadcast(&corrida.cv); esperar(&corrida.tentou);
  assert(!corrida.mudou && credGeracao == corrida.cred);
  pthread_mutex_unlock(&corrida.trava);
  return publicar(p);
}
int main(void) {
  int valor = 7; pthread_t fio;
  unsigned long long cred = trakt_credencial_geracao();
  assert(trakt_historico_aplicar_se_atual(1, 1, cred, publicar, &valor) == 7);
  assert(publicados == 1); livres();
  puts("ok  callback runs under both original ownership locks and returns its result");

  publicados = 0;
  assert(!pthread_create(&fio, NULL, respostaAntiga, NULL));
  pthread_mutex_lock(&corrida.trava); esperar(&corrida.capturou); pthread_mutex_unlock(&corrida.trava);
  pthread_mutex_lock(&travaCred); credGeracao++; pthread_mutex_unlock(&travaCred);
  pthread_mutex_lock(&corrida.trava); corrida.mudou = 1;
  pthread_cond_broadcast(&corrida.cv); pthread_mutex_unlock(&corrida.trava);
  assert(!pthread_join(fio, NULL));
  assert(!corrida.resultado && !publicados); livres();
  puts("ok  credential rotation between response check and publication rejects stale data");

  cred = trakt_credencial_geracao();
  assert(!trakt_historico_aplicar_se_atual(2, 1, cred, publicar, &valor));
  assert(!trakt_historico_aplicar_se_atual(1, 2, cred, publicar, &valor));
  assert(!trakt_historico_aplicar_se_atual(1, 1, cred, NULL, &valor));
  assert(!publicados); livres();
  puts("ok  stale profile, stale history and null callback cannot publish");

  valor = 0;
  assert(!trakt_historico_aplicar_se_atual(1, 1, cred, publicar, &valor));
  assert(publicados == 1); livres();
  puts("ok  a refused local publication keeps its false result and releases locks");

  publicados = 0; valor = 7;
  pthread_mutex_lock(&corrida.trava);
  corrida.mudou = corrida.entrou = corrida.tentou = 0; corrida.cred = cred;
  pthread_mutex_unlock(&corrida.trava);
  assert(!pthread_create(&fio, NULL, trocarDurantePublicacao, NULL));
  assert(trakt_historico_aplicar_se_atual(1, 1, cred, publicarComConcorrencia, &valor) == 7);
  assert(!pthread_join(fio, NULL));
  assert(publicados == 1 && corrida.mudou && trakt_credencial_geracao() == cred + 1); livres();
  puts("ok  credential writer cannot cross a publication already holding its lease");

  publicados = 0; cred = trakt_credencial_geracao();
  pthread_mutex_lock(&perfilTrava); perfil = 2; perfil = 3; pthread_mutex_unlock(&perfilTrava);
  assert(!trakt_historico_aplicar_se_atual(1, 1, cred, publicar, &valor));
  assert(!publicados); livres();
  puts("ok  returning to the same profile label does not revive an old opening epoch");
  puts("trakt_historico_lease512: 6 scenarios passed");
  return 0;
}
