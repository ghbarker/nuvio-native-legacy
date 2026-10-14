#include "support.h"
#include "dados.h"
#include "guia.c"
#include "guialembrete.c"
static int epg_open_profile, short_clears;
static int test_addon, network_calls, network_entered, network_release;
static pthread_mutex_t network_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t network_cv = PTHREAD_COND_INITIALIZER;
void epg_abrir_sessao(int p) { epg_open_profile = p; }
void xtepg_limpar(void) { short_clears++; }
void marco(const char *s) { (void)s; }
int addons_n(void) { return test_addon; }
const char *addons_base(int i) { (void)i; return perfis_ativo() == 1 ? "https://a.example/private_A" : "https://b.example/private_B"; }
const char *addons_nome(int i) { (void)i; return "Fixture channel addon"; }
int addons_ativo(int i) { (void)i; return 1; }
int addons_catalogos_canal(int i, AddCatCanal *out, int max) {
  (void)i; assert(max > 0); memset(out, 0, sizeof *out);
  strcpy(out->tipo, "tv"); strcpy(out->id, "channels"); strcpy(out->nome, "News"); return 1;
}
int stalker_configurado(void) { return 0; }
int xtream_configurado(void) { return 0; }
int stalker_canais_sessao(StalkerCanal *out, int max, uint64_t g) { (void)out; (void)max; (void)g; return 0; }
int xtream_canais_sessao(XtreamCanal *out, int max, uint64_t g) { (void)out; (void)max; (void)g; return 0; }
int xtream_ultima_falha(void) { return 0; }
int xtream_ultimo_http(void) { return 0; }
int xtream_ultimo_total(void) { return 0; }
char *rede_baixar_medido_controle(const char *url, int sec, const char *const *h, const RedeControle *ctl, RedeMedida *med) {
  (void)sec; (void)h; (void)med;
  assert(strstr(url, "https://a.example/private_A/catalog/tv/channels.json"));
  pthread_mutex_lock(&network_mutex); network_calls++; network_entered = 1; pthread_cond_broadcast(&network_cv);
  while (!network_release) pthread_cond_wait(&network_cv, &network_mutex);
  assert(ctl && ctl->cancelado && *ctl->cancelado);
  pthread_mutex_unlock(&network_mutex);
  return strdup("{\"metas\":[{\"id\":\"channel_A\",\"name\":\"Late A channel\",\"type\":\"tv\"}]}");
}
const char *ajustes_epg_pais(void) { return ""; }
const char *ajustes_tmdb_idioma(void) { return "en-US"; }
int ajustes_idioma(void) { return 1; }
int epg_pais_existe(const char *p) { (void)p; return 0; }
const char *epg_sem_prefixo(const char *s, char p[4]) { if (p) p[0] = 0; return s; }
int epg_paises_definir(const char *p) { (void)p; return 0; }
void epg_fonte_extra_ids(const char *const *ids, int n) { (void)ids; (void)n; }
int xtream_e_id(const char *s) { return s && !strncmp(s, "xtream:", 7); }
void fontecache_engatilhar(const char *a, const char *b, const char *c, const char *d) {
  (void)a; (void)b; (void)c; (void)d;
}
static void channel(const char *id, const char *name) {
  nCanais = nCats = 1; memset(&canais[0], 0, sizeof canais[0]);
  snprintf(canais[0].id, sizeof canais[0].id, "%s", id);
  snprintf(canais[0].nome, sizeof canais[0].nome, "%s", name);
  canais[0].cat = 0; strcpy(cats[0], "News");
}
int main(void) {
  fixture_init(); dados_gravar("guia-fav.txt", "channel_A\n"); dados_gravar("guia-modo.txt", "lista\n");
  dados_gravar("guia-preview.txt", "1\n"); favLer(); modoLer(); previewLer();
  assert(nFav == 1 && favIndice("channel_A") == 0 && modoLista && previewLigado);
  char *legacy = dados_ler("guia-fav.txt"); assert(legacy && !strcmp(legacy, "channel_A\n")); free(legacy);
  channel("channel_A", "A-only channel"); empacotar(); cacheGravar();
  unsigned long old_signature = assinaturaDe(canais, nCanais, cats);
  strcpy(canais[0].base, "https://a.example/new_credential");
  assert(assinaturaDe(canais, nCanais, cats) != old_signature); canais[0].base[0] = 0;
  puts("PASS: unchanged channel IDs cannot preserve an outdated source credential");
  lembrete_carregar(1); assert(lembrete_alternar("channel_A", "A channel", "A programme", "https://a.example/private", time(NULL) + 120, time(NULL) + 1800) == 1);
  uint64_t old = perfis_sessao_geracao(); cargaSessao = old; cargaCancelada = 0; fioVivo = 1;
  sNCanais = 1; strcpy(sCanais[0].id, "staging_A");
  aberta = overlay = painel = catAberto = buscaEstado = 1;
  pedidoBase[0] = previewId[0] = focoPend[0] = heroId[0] = 'A';
  pediuCanal = pedPreview = pedRestaurar = pedGuiaCheio = pedAssistir = 1;
  modo = ultimoModo = 1; strcpy(aCanal, "channel_A"); strcpy(aBase, "https://a.example/private");
  perfis_definir_ativo(2); guia_abrir_sessao();
  assert(!nCanais && !nCats && !nFontes && !nSabe && !nFav && !nFavOrd);
  assert(!guia_visivel() && !painel && !catAberto && !buscaEstado);
  assert(!pediuCanal && !pedPreview && !pedRestaurar && !pedGuiaCheio && !pedAssistir);
  assert(!pedidoBase[0] && !previewId[0] && !focoPend[0] && !heroId[0] && !aCanal[0] && !aBase[0]);
  assert(!glem_cartao_aberto() && lembrete_perfil() == 2 && lembrete_n() == 0);
  assert(epg_open_profile == 2 && short_clears == 1);
  assert(fioVivo && sNCanais == 1 && !strcmp(sCanais[0].id, "staging_A") && !cargaAtual());
  fioVivo = 0;
  favLer(); modoLer(); previewLer(); assert(!nFav && !modoLista && !previewLigado); assert(!cacheLer());
  puts("PASS: B starts empty; no A UI, playback request, reminder, favorite or channel is visible");
  channel("channel_B", "B-only channel"); favAlternar(&canais[0]); cacheGravar();
  assert(nFav == 1 && favIndice("channel_B") == 0);
  // A finished just before the switch. Draining its real result path must
  // leave B's published list and B's disk cache untouched.
  fioVivo = 1; atomic_store_explicit(&pendPronto, 1, memory_order_release);
  drenarCarga(); assert(!fioVivo && nCanais == 1 && !strcmp(canais[0].id, "channel_B"));
  assert(estado == G_PARADO);
  puts("PASS: draining a late A guide result preserves B's current list and cache");
  perfis_definir_ativo(1); guia_abrir_sessao(); favLer(); modoLer(); previewLer();
  assert(favIndice("channel_A") == 0 && favIndice("channel_B") < 0 && modoLista && previewLigado);
  assert(cacheLer() && nCanais == 1 && !strcmp(canais[0].id, "channel_A") && canais[0].fav);
  assert(lembrete_n() == 1 && !strcmp(lembrete_item(0)->canal, "channel_A"));
  puts("PASS: A preferences, reminders and cached channels survive switching without reconfiguration");
  perfis_definir_ativo(2); guia_abrir_sessao(); favLer(); assert(cacheLer());
  assert(nCanais == 1 && !strcmp(canais[0].id, "channel_B") && canais[0].fav);
  assert(favIndice("channel_A") < 0 && favIndice("channel_B") == 0);
  puts("PASS: B reloads only its own saved favorite and channel cache");
  perfis_definir_ativo(1); guia_abrir_sessao(); test_addon = 1;
  iniciarCarga();
  pthread_mutex_lock(&network_mutex); while (!network_entered) pthread_cond_wait(&network_cv, &network_mutex); pthread_mutex_unlock(&network_mutex);
  perfis_definir_ativo(2); guia_abrir_sessao(); favLer(); assert(cacheLer());
  pthread_mutex_lock(&network_mutex); network_release = 1; pthread_cond_broadcast(&network_cv); pthread_mutex_unlock(&network_mutex);
  for (int k = 0; k < 2000 && !atomic_load_explicit(&pendPronto, memory_order_acquire); k++) usleep(1000);
  assert(atomic_load_explicit(&pendPronto, memory_order_acquire)); drenarCarga();
  assert(network_calls == 1 && !sNCanais && nCanais == 1 && !strcmp(canais[0].id, "channel_B"));
  puts("PASS: canceled A guide worker uses only captured A URL; late response cannot replace cached B channels");
  fixture_cleanup();
}
