#include "support.h"
#include "xtepg.c"
static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cv = PTHREAD_COND_INITIALIZER;
static int block, entered, release_request, long_backoff, got429;
int xtream_e_id(const char *id) { return id && !strncmp(id, "xtream:", 7); }
int xtream_ultimo_retry_after(void) { return 60; }
int xtream_epg_curto(const char *id, XtreamProg *p, int cap, int *status) {
  (void)id; (void)cap;
  int owner = perfis_ativo();
  pthread_mutex_lock(&m);
  if (block) { entered = 1; pthread_cond_broadcast(&cv); while (!release_request) pthread_cond_wait(&cv, &m); }
  if (long_backoff && owner == 1) { got429 = 1; pthread_cond_broadcast(&cv); pthread_mutex_unlock(&m); *status = 429; return -1; }
  pthread_mutex_unlock(&m);
  *status = 200; p->ini = time(NULL) - 60; p->fim = time(NULL) + 3600;
  snprintf(p->titulo, sizeof p->titulo, "Private programme profile %d", owner);
  return 1;
}
static void wait_cached(const char *id) {
  for (int k = 0; k < 2000; k++) { xtepg_passo(); if (xtepg_tem(id)) return; usleep(1000); }
  assert(!"short EPG did not complete");
}
int main(void) {
  fixture_init(); block = 1; xtepg_querer("xtream:42");
  pthread_mutex_lock(&m); while (!entered) pthread_cond_wait(&cv, &m); pthread_mutex_unlock(&m);
  perfis_definir_ativo(2); xtepg_limpar(); assert(!xtepg_tem("xtream:42")); xtepg_querer("xtream:42");
  pthread_mutex_lock(&m); release_request = 1; block = 0; pthread_cond_broadcast(&cv); pthread_mutex_unlock(&m);
  wait_cached("xtream:42"); EpgProg p; assert(xtepg_agora("xtream:42", time(NULL), &p));
  assert(!strcmp(p.titulo, "Private programme profile 2"));
  puts("PASS: identical channel IDs cannot receive an old profile’s delayed short EPG");
  perfis_definir_ativo(1); xtepg_limpar(); long_backoff = 1; xtepg_querer("xtream:43");
  pthread_mutex_lock(&m); while (!got429) pthread_cond_wait(&cv, &m); pthread_mutex_unlock(&m);
  perfis_definir_ativo(2); xtepg_limpar();
  struct timespec start, end; clock_gettime(CLOCK_MONOTONIC, &start); xtepg_querer("xtream:43");
  wait_cached("xtream:43"); clock_gettime(CLOCK_MONOTONIC, &end);
  double seconds = end.tv_sec - start.tv_sec + (end.tv_nsec - start.tv_nsec) / 1e9;
  assert(seconds < 2.0); assert(xtepg_agora("xtream:43", time(NULL), &p)); assert(strstr(p.titulo, "profile 2"));
  printf("PASS: switching interrupts A’s 60-second backoff; B completes in %.3f seconds\n", seconds);
  xtepg_limpar(); fixture_cleanup();
}
