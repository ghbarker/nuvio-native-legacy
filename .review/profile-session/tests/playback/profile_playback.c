// Uses the real profile session gate, source orchestration, caches and subtitle workers.
#define main legacy_vod_fixture_main
#define perfis_ativo legacy_perfis_ativo
#define rede_baixar legacy_rede_baixar
#define stream_extrair legacy_stream_extrair
#include "tests/fontecache_vod.c"
#undef main
#undef perfis_ativo
#undef rede_baixar
#undef stream_extrair
#include "src/perfis.h"
#include "src/fontesmortas.h"
#include "src/addonstats.h"
#include "src/js.h"
#include <sys/time.h>
#include "linguas.h"

static pthread_mutex_t network_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t network_condition = PTHREAD_COND_INITIALIZER;
static int blocked, release_network, block_once, blocked_done;
static _Atomic int subtitle_calls;
static char requested_base[2048];
static char files_name[24][100], *files_body[24];
static int files_count;
void fil_definir_perfil(int p) { (void)p; }
void fontepref_definir_perfil(int p) { (void)p; }
void legmem_definir_perfil(int p) { (void)p; }
void arteesc_definir_perfil(int p) { (void)p; }
void plugins_perfil_mudou(void) {}
void servidores_perfil_trocou(void) {}
int dados_gravar(const char *name, const char *body) {
  int i;
  for (i = 0; i < files_count; i++) if (!strcmp(name, files_name[i])) break;
  if (i == files_count) { assert(files_count < 24); snprintf(files_name[i], 100, "%s", name); files_count++; }
  free(files_body[i]); files_body[i] = strdup(body); return 1;
}
char *dados_ler(const char *name) {
  for (int i = 0; i < files_count; i++) if (!strcmp(name, files_name[i])) return strdup(files_body[i]);
  return NULL;
}
static char *transport(const char *url) {
  int sequence = atomic_fetch_add(&pedidos, 1) + 1;
  int sub = strstr(url, "/subtitles/") != NULL;
  if (sub) atomic_fetch_add(&subtitle_calls, 1);
  char owner[16] = "none";
  const char *start = strstr(url, "fixture.invalid/");
  assert(start); start += strlen("fixture.invalid/");
  size_t length = strcspn(start, "/"); assert(length < sizeof owner);
  memcpy(owner, start, length); owner[length] = 0;
  pthread_mutex_lock(&network_lock);
  snprintf(requested_base, sizeof requested_base, "%s", url);
  if (block_once) {
    block_once = 0; blocked = 1; pthread_cond_broadcast(&network_condition);
    while (!release_network) pthread_cond_wait(&network_condition, &network_lock);
    blocked_done = 1; pthread_cond_broadcast(&network_condition);
  }
  pthread_mutex_unlock(&network_lock);
  if (strstr(url, "/fail/")) return NULL;
  char body[1024];
  if (sub) snprintf(body, sizeof body, "{\"subtitles\":[{\"lang\":\"eng\",\"url\":\"https://media.fixture.invalid/%s/sub-%d.srt\"}]}", owner, sequence);
  else snprintf(body, sizeof body, "{\"streams\":[{\"url\":\"https://media.fixture.invalid/%s/video-%d.mp4\"}]}", owner, sequence);
  return strdup(body);
}
char *rede_baixar(const char *url, int seconds) { (void)seconds; return transport(url); }
char *rede_baixar_medido_controle(const char *url, int seconds, const char *const *headers, const RedeControle *control, RedeMedida *measure) {
  (void)seconds; (void)headers; (void)control;
  char *body = transport(url); *measure = (RedeMedida){ .status=body ? 200 : 503, .bytes=body ? (long)strlen(body) : 0, .ms=1 }; return body;
}
char *rede_baixar_trecho_st(const char *u, int s, long long a, long long b, long *n, int *st, int *e, char *f, unsigned t) {
  (void)u;(void)s;(void)a;(void)b;(void)f;(void)t; if(n)*n=0;if(st)*st=0;if(e)*e=28;return NULL;
}
int stream_extrair(const char *json, const char *provider, Stream **out) {
  const char *object = js_array(json, NULL, "streams");
  *out = NULL; if (!object) return 0;
  Stream *s = calloc(1, sizeof *s); assert(s);
  js_texto(object, js_fim(object), "url", s->url, sizeof s->url);
  snprintf(s->provedor, sizeof s->provedor, "%s", provider); *out=s; return 1;
}
static void configure(const char *name) {
  AddonRemoto a = {.ativo=1};
  snprintf(a.nome, sizeof a.nome, "AIOStreams");
  snprintf(a.url, sizeof a.url, "https://fixture.invalid/%s/manifest.json", name);
  assert(addons_definir_lista(&a, 1));
}
static void activate(int number) { perfis_definir_ativo(number); addons_abrir_sessao(); }
static void start_block(void) { pthread_mutex_lock(&network_lock); block_once=1;blocked=release_network=blocked_done=0;pthread_mutex_unlock(&network_lock); }
static void await_block(void) {
  for (int i=0;i<4000;i++) { pthread_mutex_lock(&network_lock); int yes=blocked;pthread_mutex_unlock(&network_lock);if(yes)return;usleep(1000); }
  assert(!"network did not start");
}
static void unblock(void) { pthread_mutex_lock(&network_lock);release_network=1;pthread_cond_broadcast(&network_condition);pthread_mutex_unlock(&network_lock); }
static void finish_source(void) {
  for(int i=0;i<4000;i++){if(addons_estado()!=ADD_BUSCANDO)return;usleep(1000);}assert(!"source worker did not finish");
}
static void finish_subtitles(void) {
  for(int i=0;i<4000;i++){if(addons_legendas_prontas())return;usleep(1000);}assert(!"subtitle worker did not finish");
}
static void await_network_done(void) {
  for(int i=0;i<4000;i++){pthread_mutex_lock(&network_lock);int yes=blocked_done;pthread_mutex_unlock(&network_lock);if(yes)return;usleep(1000);}assert(!"network did not finish");
}
int main(void) {
  activate(1); configure("A"); start_block(); stream_definir_alvo("tt42"); addons_buscar("tt42","movie"); await_block();
  activate(2); assert(!addons_n() && !stream_n() && !addons_n_legendas()); configure("B"); unblock(); finish_source(); assert(!stream_n());
  buscarTitulo("tt42","movie"); assert(nLista==1 && strstr(listaAtiva[0].url,"/B/"));
  puts("PASS: profile B rejects late A source and loads its own same-title URL");

  activate(1); configure("A"); start_block(); stream_definir_alvo("tt42"); addons_buscar("tt42","movie"); await_block();
  uint64_t old=perfis_sessao_geracao(); activate(2);activate(1);configure("A");assert(!perfis_sessao_atual(old));unblock();finish_source();assert(!stream_n());
  buscarTitulo("tt42","movie");assert(nLista==1 && strstr(listaAtiva[0].url,"/A/"));
  puts("PASS: A to B to A rejects the original A request despite same account/profile/config");

  Stream saved={0};snprintf(saved.url,sizeof saved.url,"https://media.fixture.invalid/private-live.m3u8");
  fontecache_guardar("channel:same","tv",&saved,1);assert(fontecache_n()==1);
  FontecacheEscopo scope;addons_capturar_escopo(&scope);activate(2);configure("B");
  Stream *out=NULL;int count=0;assert(fontecache_pegar("channel:same","tv",&out,&count)==FC_NADA);
  fontecache_guardar_sessao("channel:same","tv",&saved,1,scope.sessao);assert(!fontecache_n());
  puts("PASS: signed live source cache and late inserts cannot cross sessions");

  ling_local_legenda("*"); addons_buscar_legendas("tt42","movie");finish_subtitles();assert(addons_n_legendas()==1);assert(strstr(addons_legenda(0)->url,"/B/"));
  int sub_before=subtitle_calls;activate(1);configure("A");assert(!addons_n_legendas());addons_buscar_legendas("tt42","movie");finish_subtitles();assert(subtitle_calls>sub_before);assert(strstr(addons_legenda(0)->url,"/A/"));
  puts("PASS: same-title subtitle result is refetched after profile switch");
  activate(1);configure("A");start_block();addons_buscar_legendas("tt42","movie");await_block();activate(2);configure("B");unblock();await_network_done();
  for(int i=0;i<100;i++)usleep(1000);assert(!addons_n_legendas());addons_buscar_legendas("tt42","movie");finish_subtitles();assert(strstr(addons_legenda(0)->url,"/B/"));
  puts("PASS: delayed subtitle publication is discarded without restoring A's cached URLs");

  Stream failed={0};snprintf(failed.provedor,sizeof failed.provedor,"AIOStreams");snprintf(failed.infoHash,sizeof failed.infoHash,"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");failed.fileIdx=0;
  activate(1);fontesmortas_marcar(&failed);assert(fontesmortas_idade(&failed)>=0);activate(2);assert(fontesmortas_idade(&failed)<0);activate(1);assert(fontesmortas_idade(&failed)>=0);
  conta="conta-b";activate(1);assert(fontesmortas_idade(&failed)<0);conta="conta-a";activate(1);assert(fontesmortas_idade(&failed)>=0);
  puts("PASS: failed-link memory persists per profile/account without deleting other profiles");

  configure("fail");for(int i=0;i<3;i++)buscarTitulo("tt99","movie");int calls=pedidos;
  AddonRemoto different={.ativo=1};snprintf(different.nome,sizeof different.nome,"AIOStreams");snprintf(different.url,sizeof different.url,"https://fixture.invalid/fail/other-config/manifest.json");assert(addons_definir_lista(&different,1));buscarTitulo("tt99","movie");assert(pedidos-calls==2);
  puts("PASS: same addon name with different configuration gets its own retry history");
  addons_encerrar();stream_definir_lista(NULL,0);for(int i=0;i<files_count;i++)free(files_body[i]);puts("profile playback isolation: PASS");
}
