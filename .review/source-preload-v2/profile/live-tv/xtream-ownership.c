#include "support.h"
#include "xtream.h"
#include "rede.h"
#include "dados.h"
static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cv = PTHREAD_COND_INITIALIZER;
static int block, entered, release_request, calls, result;
static char requested[1200];
int ajustes_livetv_formato(void) { return 0; }
char *rede_baixar_st_retry(const char *url, int sec, const char *const *headers, int *status, int *ra) {
  (void)sec; (void)headers; if (ra) *ra = 0; if (status) *status = 200;
  pthread_mutex_lock(&m); calls++; snprintf(requested, sizeof requested, "%s", url);
  if (block) { entered = 1; pthread_cond_broadcast(&cv); while (!release_request) pthread_cond_wait(&cv, &m); }
  pthread_mutex_unlock(&m);
  if (strstr(url, "get_live_categories")) return strdup("[]");
  if (strstr(url, "get_live_streams")) return strdup("[{\"stream_id\":42,\"name\":\"A channel\"}]");
  return strdup("{\"user_info\":{\"auth\":1,\"status\":\"Active\",\"max_connections\":4,\"allowed_output_formats\":[\"ts\"]}}");
}
static void *worker(void *u) { (void)u; XtreamConta c; result = xtream_conta_ler(&c); return NULL; }
static void begin(pthread_t *t) {
  pthread_mutex_lock(&m); block = 1; entered = release_request = 0; pthread_mutex_unlock(&m);
  assert(!pthread_create(t, NULL, worker, NULL));
  pthread_mutex_lock(&m); while (!entered) pthread_cond_wait(&cv, &m); pthread_mutex_unlock(&m);
}
static void finish(pthread_t t) {
  pthread_mutex_lock(&m); release_request = 1; pthread_cond_broadcast(&cv); pthread_mutex_unlock(&m);
  assert(!pthread_join(t, NULL)); block = 0;
}
int main(void) {
  pthread_t t; XtreamConta c; XtreamCanal ch[2]; const char *ext[2]; char url[1100];
  fixture_init();
  xtream_definir_servidor("https://a.example"); xtream_definir_usuario("userA"); xtream_definir_senha("secretA");
  uint64_t a = perfis_sessao_geracao(); begin(&t);
  assert(strstr(requested, "userA") && strstr(requested, "secretA"));
  perfis_definir_ativo(2);
  assert(!xtream_configurado()); assert(!xtream_conta(&c));
  assert(!xtream_url("xtream:42", url, sizeof url));
  finish(t); assert(!result); assert(!xtream_conta(&c));
  int before = calls; assert(!xtream_canais_sessao(ch, 2, a)); assert(calls == before);
  puts("PASS: delayed A account cannot populate empty B or use B credentials");
  perfis_definir_ativo(1); assert(xtream_configurado()); begin(&t);
  uint64_t old_a = perfis_sessao_geracao();
  perfis_definir_ativo(2); perfis_definir_ativo(1);
  assert(perfis_sessao_geracao() != old_a); assert(xtream_configurado()); assert(!xtream_conta(&c));
  finish(t); assert(!result); assert(!xtream_conta(&c));
  assert(xtream_conta_ler(&c) && c.maxConexoes == 4);
  assert(xtream_formatos(ext) == 1 && !strcmp(ext[0], "ts"));
  assert(xtream_canais(ch, 2) == 1 && !strcmp(ch[0].nome, "A channel"));
  perfis_definir_ativo(2); assert(xtream_formatos(ext) == 2 && !strcmp(ext[0], "m3u8"));
  puts("PASS: A -> B -> A rejects the old opening and preserves saved A credentials");
  fixture_cleanup();
}
