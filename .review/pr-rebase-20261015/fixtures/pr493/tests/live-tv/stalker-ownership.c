#include "support.h"
#include "stalker.h"
#include "dados.h"
static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cv = PTHREAD_COND_INITIALIZER;
static int block, block_handshake, entered, release_request, calls, result;
char *rede_baixar_com(const char *url, int sec, const char *const *headers) {
  (void)sec; (void)headers;
  pthread_mutex_lock(&m); calls++;
  if (block && strstr(url, block_handshake ? "action=handshake" : "get_ordered_list")) {
    entered = 1; pthread_cond_broadcast(&cv); while (!release_request) pthread_cond_wait(&cv, &m);
  }
  pthread_mutex_unlock(&m);
  if (strstr(url, "action=handshake")) return strdup("{\"js\":1,\"token\":\"TOKEN_A\"}");
  if (strstr(url, "get_genres")) return strdup("{\"js\":[{\"id\":\"1\",\"title\":\"News\"}]}");
  if (strstr(url, "get_ordered_list")) return strdup("{\"js\":{\"data\":[{\"id\":\"42\",\"name\":\"A channel\",\"cmd\":\"ffmpeg http://a.example/private_A\",\"tv_genre_id\":\"1\"}]}}");
  if (strstr(url, "create_link")) return strdup("{\"js\":1,\"cmd\":\"ffmpeg http://a.example/private_A\"}");
  return strdup("{\"js\":1}");
}
static void *worker(void *u) { uint64_t s = *(uint64_t *)u; StalkerCanal out[1]; result = stalker_canais_sessao(out, 1, s); return NULL; }
static void begin(pthread_t *t, uint64_t *s) {
  pthread_mutex_lock(&m); block = 1; entered = release_request = 0; pthread_mutex_unlock(&m);
  *s = perfis_sessao_geracao(); assert(!pthread_create(t, NULL, worker, s));
  pthread_mutex_lock(&m); while (!entered) pthread_cond_wait(&cv, &m); pthread_mutex_unlock(&m);
}
static void finish(pthread_t t) {
  pthread_mutex_lock(&m); release_request = 1; pthread_cond_broadcast(&cv); pthread_mutex_unlock(&m);
  assert(!pthread_join(t, NULL)); block = 0;
}
int main(void) {
  pthread_t t; uint64_t s; char url[1100]; StalkerCanal out[1]; fixture_init();
  stalker_definir_portal("a.example"); stalker_definir_mac("00:1a:79:00:00:01");
  begin(&t, &s); perfis_definir_ativo(2); assert(!stalker_configurado());
  finish(t); assert(result == 0); assert(!stalker_resolver("stalker:42", url, sizeof url));
  int before_old_resolve = calls; assert(!stalker_resolver_sessao("stalker:42", url, sizeof url, s)); assert(calls == before_old_resolve);
  puts("PASS: delayed A channel response cannot populate B command table");
  perfis_definir_ativo(1); begin(&t, &s); perfis_definir_ativo(2); perfis_definir_ativo(1);
  assert(stalker_configurado()); finish(t); assert(result == 0);
  int before = calls; assert(!stalker_resolver("stalker:42", url, sizeof url)); assert(calls == before);
  assert(stalker_canais(out, 1) == 1); assert(stalker_resolver("stalker:42", url, sizeof url));
  assert(!strcmp(url, "http://a.example/private_A"));
  puts("PASS: A -> B -> A rejects stale commands; fresh A still resolves its own channel");
  perfis_definir_ativo(1); block_handshake = 1; begin(&t, &s);
  struct timespec start, end; clock_gettime(CLOCK_MONOTONIC, &start);
  perfis_definir_ativo(2); assert(!stalker_configurado());
  assert(!strcmp(stalker_portal_curto(), "-") && !strcmp(stalker_mac_mascarado(), "-"));
  clock_gettime(CLOCK_MONOTONIC, &end);
  double seconds = end.tv_sec - start.tv_sec + (end.tv_nsec - start.tv_nsec) / 1e9;
  assert(seconds < 0.1); finish(t); assert(result == 0);
  printf("PASS: delayed A handshake does not block profile opening/settings; B ready in %.3f seconds\n", seconds);
  fixture_cleanup();
}
