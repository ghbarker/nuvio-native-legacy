// Real addon request capture, provenance stamping and worker publication.
// Only transport/parser and unrelated persistence neighbours are fixtures.
#include "perfis.h"
#include "rede.h"
#define main provenance_legacy_main
#define rede_baixar provenance_legacy_download
#define perfis_ativo provenance_legacy_active
#define sessao_usuario provenance_legacy_account
#define stream_definir_lista provenance_legacy_define
#define stream_definir_lista_idade provenance_legacy_define_age
#define stream_lista_acrescentar provenance_legacy_append
#include "addonslista.c"
#undef main
#undef rede_baixar
#undef perfis_ativo
#undef sessao_usuario
#undef stream_definir_lista
#undef stream_definir_lista_idade
#undef stream_lista_acrescentar
#include <assert.h>
#include <pthread.h>
#include <time.h>

static Stream published[8];
static int published_count, cache_writes;
void stream_definir_lista(const Stream *list, int count) {
  assert(count >= 0 && count <= 8);
  published_count = count;
  if (count) memcpy(published, list, sizeof *list * (size_t)count);
}
void stream_definir_lista_idade(const Stream *list, int count, Uint32 age) {
  (void)age; stream_definir_lista(list, count);
}
void stream_lista_acrescentar(const Stream *list, int count, int order) {
  assert(published_count + count <= 8);
  for (int i = 0; i < count; i++) assert(list[i].addonOrdem == order);
  memcpy(published + published_count, list, sizeof *list * (size_t)count);
  published_count += count;
}
void fontecache_guardar_sessao(const char *id, const char *type,
                              const Stream *list, int count, uint64_t visit) {
  (void)id; (void)type; (void)list; (void)count;
  assert(perfis_sessao_atual(visit));
  cache_writes++;
}
const char *sessao_usuario(void) { return "provenance-account"; }
int dados_gravar(const char *name, const char *text) {
  (void)name; (void)text; return 1;
}
void fil_definir_perfil(int p) { (void)p; }
void fontepref_definir_perfil(int p) { (void)p; }
void legmem_definir_perfil(int p) { (void)p; }
void arteesc_definir_perfil(int p) { (void)p; }
void plugins_perfil_mudou(void) {}
void servidores_perfil_trocou(void) {}
int sessao_identidade_local_copiar(char *account, size_t na, char *jwt, size_t nj) {
  snprintf(account, na, "%s", sessao_usuario());
  snprintf(jwt, nj, "%s", "fixture-jwt"); return 1;
}
void sessao_token_recusado(const char *jwt) { (void)jwt; }

#include "addons.c"

static pthread_mutex_t network_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t network_changed = PTHREAD_COND_INITIALIZER;
static int hold_network, network_entered, network_released;
static int replace_live_bases, requests;
static char requested[2][NV_ADDON_PEDIDO_MAX];
static char original_base[2][NV_ADDON_URL_MAX];

static void replace_live_addons(void) {
  pthread_mutex_lock(&addonTrava);
  for (int i = 0; i < nAddon; i++)
    snprintf(addon[i].base, sizeof addon[i].base,
             "https://same-host.test/profile-b/instance-%d", i);
  pthread_mutex_unlock(&addonTrava);
}

char *rede_baixar(const char *url, int timeout) {
  assert(timeout > 0 && strstr(url, "/stream/"));
  pthread_mutex_lock(&network_lock);
  if (requests < 2) snprintf(requested[requests], sizeof requested[0], "%s", url);
  requests++;
  if (hold_network) {
    network_entered = 1;
    pthread_cond_broadcast(&network_changed);
    while (!network_released) pthread_cond_wait(&network_changed, &network_lock);
  }
  pthread_mutex_unlock(&network_lock);
  if (replace_live_bases) replace_live_addons();
  return strdup("{\"streams\":[{\"url\":\"https://video.test/stream.mp4\"}]}");
}

static void configure_original(int count) {
  pthread_mutex_lock(&addonTrava);
  memset(addon, 0, sizeof addon); nAddon = count;
  for (int i = 0; i < count; i++) {
    strcpy(addon[i].nome, "AIOStreams");
    strcpy(addon[i].base, "https://same-host.test/config/");
    size_t length = strlen(addon[i].base);
    // Same host, same displayed name, and a long identical config prefix.
    memset(addon[i].base + length, 'x', 800); length += 800;
    addon[i].base[length++] = (char)('a' + i); addon[i].base[length] = 0;
    addon[i].ativo = addon[i].fonte = 1;
    snprintf(original_base[i], sizeof original_base[i], "%s", addon[i].base);
  }
  versaoLista++;
  pthread_mutex_unlock(&addonTrava);
  requests = published_count = cache_writes = 0;
  memset(requested, 0, sizeof requested);
  progLimpar(); progPublicou = progExtraPublicou = 0;
  progLigado = 1; pendId[0] = 0;
  addons_capturar_escopo(&fioEscopo);
  assert(fioEscopo.sessao == perfis_sessao_geracao() && fioEscopo.sessao > 0);
}

static void original_provenance_survives_live_replacement(void) {
  perfis_definir_ativo(1); configure_original(2);
  char expected[2][FONTEFALHA_CHAVE], replaced[FONTEFALHA_CHAVE];
  for (int i = 0; i < 2; i++)
    fontefalha_addon_chave(original_base[i], expected[i], sizeof expected[i]);
  assert(strcmp(expected[0], expected[1]));
  BaldeFonte buckets[2] = {{.idx = 0}, {.idx = 1}};
  Consulta query = {.id = "tt100", .tipo = "movie", .tipoAlt = "", .timeout = 1,
                    .baldes = buckets, .nBaldes = 2, .progresso = 1, .rodada = 1,
                    .sessao = perfis_sessao_geracao()};
  query.origens = capturarPedidos(query.sessao, &query.nOrigens, &query.versao);
  assert(query.origens && query.nOrigens == 2);
  pthread_mutex_init(&query.trava, NULL);
  replace_live_bases = 1;
  fioFontes(&query);
  replace_live_bases = 0;
  assert(requests == 2);
  for (int i = 0; i < 2; i++) {
    assert(strstr(requested[i], original_base[i]) == requested[i]);
    assert(strcmp(addon[i].base, original_base[i]));
    fontefalha_addon_chave(addon[i].base, replaced, sizeof replaced);
    assert(strcmp(expected[i], replaced));
    assert(buckets[i].n == 1);
    assert(!strcmp(buckets[i].achados[0].addonChave, expected[i]));
    assert(buckets[i].achados[0].addonOrdem == i);
    free(buckets[i].achados);
  }
  free(query.origens); pthread_mutex_destroy(&query.trava);
  progDrenar(); assert(published_count == 2);
  for (int i = 0; i < 2; i++) assert(!strcmp(published[i].addonChave, expected[i]));
  progLimpar();
}

static void wait_for_network(void) {
  struct timespec deadline;
  assert(!clock_gettime(CLOCK_REALTIME, &deadline)); deadline.tv_sec += 5;
  pthread_mutex_lock(&network_lock);
  while (!network_entered)
    assert(!pthread_cond_timedwait(&network_changed, &network_lock, &deadline));
  pthread_mutex_unlock(&network_lock);
}

static void stale_visit_cannot_publish(int return_to_a, int same_profile) {
  perfis_definir_ativo(1); configure_original(1);
  free(resultado); resultado = NULL; nResultado = 0;
  atomic_store(&estado, ADD_BUSCANDO); atomic_store(&fioTerminou, 0);
  PedidoBusca *request = calloc(1, sizeof *request); assert(request);
  strcpy(request->id, "tt200"); strcpy(request->tipo, "movie");
  request->escopo = fioEscopo; request->progresso = 1;
  uint64_t original_visit = request->escopo.sessao;
  pthread_mutex_lock(&network_lock);
  hold_network = 1; network_entered = network_released = 0;
  pthread_mutex_unlock(&network_lock);
  pthread_t worker; assert(!pthread_create(&worker, NULL, buscar, request));
  wait_for_network();
  perfis_definir_ativo(same_profile ? 1 : 2);
  if (return_to_a) perfis_definir_ativo(1);
  assert(!perfis_sessao_atual(original_visit));
  if (return_to_a || same_profile) assert(perfis_ativo() == 1);
  pthread_mutex_lock(&network_lock);
  network_released = 1; pthread_cond_broadcast(&network_changed);
  pthread_mutex_unlock(&network_lock);
  assert(!pthread_join(worker, NULL)); hold_network = 0;
  progDrenar();
  assert(atomic_load(&fioTerminou));
  assert(atomic_load(&estado) == ADD_PARADO);
  assert(!resultado && nResultado == 0 && cache_writes == 0);
  assert(published_count == 0 && progN == 0);
  progLimpar();
}

static void current_visit_can_publish(void) {
  perfis_definir_ativo(1); configure_original(1);
  free(resultado); resultado = NULL; nResultado = 0;
  PedidoBusca *request = calloc(1, sizeof *request); assert(request);
  strcpy(request->id, "tt300"); strcpy(request->tipo, "movie");
  request->escopo = fioEscopo; request->progresso = 1;
  char expected[FONTEFALHA_CHAVE];
  fontefalha_addon_chave(original_base[0], expected, sizeof expected);
  atomic_store(&estado, ADD_BUSCANDO); atomic_store(&fioTerminou, 0);
  buscar(request); progDrenar();
  assert(requests == 1 && atomic_load(&estado) == ADD_PRONTO);
  assert(resultado && nResultado == 1 && cache_writes == 1);
  assert(!strcmp(resultado[0].addonChave, expected));
  assert(published_count == 1 && !strcmp(published[0].addonChave, expected));
  free(resultado); resultado = NULL; nResultado = 0; progLimpar();
}

int main(void) {
  original_provenance_survives_live_replacement();
  stale_visit_cannot_publish(0, 0);
  stale_visit_cannot_publish(1, 0);
  stale_visit_cannot_publish(0, 1);
  current_visit_can_publish();
  puts("PASS: real addon snapshots preserve full instance identity after live replacement; A-B, A-B-A and same-profile stale workers cannot publish streams or refill cache");
  return 0;
}
