// Real native worker, native providers and monotonic profile session gate.
// Only transport/UI neighbours are doubles; real API credentials never used.
#include <native-worker-support.h>
#include "perfis.h"
#include <unistd.h>

static pthread_mutex_t barrier = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t condition = PTHREAD_COND_INITIALIZER;
static int entered, released, calls, posts, delay, success;
static int callOwner[32];
static const char *ownerKey = "A";
static uint64_t requestEpoch;
static DebridContexto credentials;
static FonteFalhaSessao *policy;
static unsigned listEpoch;
static int result, chosen, service;
static char resultUrl[4096];

const char *sessao_usuario(void) { return "native-request-fixture"; }
int dados_gravar(const char *name, const char *body) { (void)name; (void)body; return 1; }
void fil_definir_perfil(int p) { (void)p; }
void fontepref_definir_perfil(int p) { (void)p; }
void legmem_definir_perfil(int p) { (void)p; }
void arteesc_definir_perfil(int p) { (void)p; }
void plugins_perfil_mudou(void) {}
void servidores_perfil_trocou(void) {}

char *rede_baixar_st(const char *url, int seconds, const char *const *headers, int *status) {
  (void)seconds;
  int owner = headers && headers[0] && strstr(headers[0], "TEST-A") ? 1 :
              headers && headers[0] && strstr(headers[0], "TEST-B") ? 2 : 0;
  if (strstr(url, "requestdl")) owner = strstr(url, "TEST-A") ? 1 : strstr(url, "TEST-B") ? 2 : 0;
  assert(owner && calls < 32);
  callOwner[calls++] = owner;
  assert(strstr(url, "api.torbox.app") && "stale worker must never fall through to another native provider");
  pthread_mutex_lock(&barrier);
  if (delay && strstr(url, "checkcached")) {
    entered = 1; pthread_cond_broadcast(&condition);
    while (!released) pthread_cond_wait(&condition, &barrier);
  }
  pthread_mutex_unlock(&barrier);
  *status = 200;
  if (strstr(url, "checkcached"))
    return strdup(success ? "{\"success\":true,\"data\":[{\"hash\":\"abc\"}]}" : "{\"success\":true,\"data\":[]}");
  if (strstr(url, "mylist"))
    return strdup("{\"success\":true,\"data\":[{\"id\":7,\"files\":[{\"id\":0,\"name\":\"Movie.mp4\",\"size\":9999}]}]}");
  assert(strstr(url,"requestdl"));
  return strdup(owner == 1 ? "{\"success\":true,\"data\":\"https://native.fixture.invalid/A/video.mp4\"}" : "{\"success\":true,\"data\":\"https://native.fixture.invalid/B/video.mp4\"}");
}
char *rede_postar_st(const char *url, int seconds, const char *const *headers, const char *body, int *status) {
  (void)seconds; (void)body;
  assert(strstr(url,"createtorrent") && headers && headers[0] && strstr(headers[0],ownerKey));
  posts++; *status = 200; return strdup("{\"success\":true,\"data\":{\"torrent_id\":7}}");
}
static void activate(int profile, const char *key) {
  perfis_definir_ativo(profile);
  debrid_abrir_sessao(perfis_sessao_geracao());
  DebridConta c; debrid_conta_iniciar(&c);
  debrid_conta_definir(&c,"torbox",key);
  debrid_conta_definir(&c,"premiumize",key);
  debrid_aplicar_chaves_locais(&c);
}
static FonteFalhaConfig config(void) {
  FonteFalhaConfig c = {.provedores=1};
  c.ordem[0]=DEBRID_TB; c.ordem[1]=DEBRID_PM; c.ordem[2]=DEBRID_RD; c.ordem[3]=DEBRID_AD;
  for(int i=0;i<4;i++) c.limites[i]=3;
  return c;
}
static void dispatch(void) {
  FonteFalhaConfig c=config();
  policy=fontefalha_sessao_nova(&c,(1u<<DEBRID_TB)|(1u<<DEBRID_PM),-1); assert(policy);
  Stream s={.fileIdx=0,.altura=1080}; strcpy(s.infoHash,"ABC"); strcpy(s.rotulo,"1080p");
  strcpy(s.addonChave,"native-fixture-addon");
  stream_definir_lista(&s,1); listEpoch=stream_lista_geracao();
  requestEpoch=perfis_sessao_geracao(); debrid_contexto_capturar(&credentials);
}
static int cancelled(void *u) { (void)u; return !perfis_sessao_atual(requestEpoch); }
static void *worker(void *u) {
  (void)u; debrid_contexto_entrar(&credentials);
  result=stream_falha_verificar(policy,listEpoch,-1,cancelled,NULL,&chosen,&service,resultUrl,sizeof resultUrl);
  debrid_contexto_sair(); return NULL;
}
static void waitEntered(void) {
  for(int i=0;i<4000;i++) {
    pthread_mutex_lock(&barrier); int yes=entered; pthread_mutex_unlock(&barrier);
    if(yes) return; usleep(1000);
  }
  assert(!"native TorBox transport failed to enter barrier");
}
static void release(void) {
  pthread_mutex_lock(&barrier); released=1; pthread_cond_broadcast(&condition); pthread_mutex_unlock(&barrier);
}
int main(void) {
  activate(1,"TEST-A"); dispatch(); delay=1;
  pthread_t thread; assert(!pthread_create(&thread,NULL,worker,NULL)); waitEntered();
  activate(2,"TEST-B"); activate(1,"TEST-A"); assert(!perfis_sessao_atual(requestEpoch));
  release(); assert(!pthread_join(thread,NULL));
  assert(result==FONTEFALHA_FIM && chosen==-1 && !resultUrl[0]);
  assert(calls==1 && callOwner[0]==1 && posts==0 && debrid_fora_de_cache()==0);
  assert(debrid_servico_disponivel(DEBRID_TB) && debrid_servico_disponivel(DEBRID_PM));
  fontefalha_sessao_liberar(policy);
  puts("PASS native failover: delayed A→B→A stops after A's captured HTTP; no B credential query, secondary provider or stale cache counter");

  activate(1,"TEST-A"); dispatch(); activate(2,"TEST-B");
  int priorCalls=calls; worker(NULL); assert(calls==priorCalls && result==FONTEFALHA_FIM);
  fontefalha_sessao_liberar(policy);
  puts("PASS native failover: worker dispatched before a switch but scheduled after it performs no request");

  delay=0; success=1; ownerKey="TEST-B"; dispatch(); worker(NULL);
  assert(result==FONTEFALHA_PROXIMA && chosen==0 && service==DEBRID_TB && strstr(resultUrl,"/B/"));
  assert(calls==priorCalls+3 && posts==1);
  assert(stream_falha_aplicar_sessao(chosen,listEpoch,service,resultUrl,requestEpoch));
  assert(strstr(stream_item(0)->url,"/B/"));
  char saved[4096]; strcpy(saved,stream_item(0)->url);
  perfis_definir_ativo(1); perfis_definir_ativo(2);
  assert(stream_lista_geracao()==listEpoch && !perfis_sessao_atual(requestEpoch));
  assert(!stream_falha_aplicar_sessao(0,listEpoch,DEBRID_PM,"https://native.fixture.invalid/A/stale.mp4",requestEpoch));
  assert(!strcmp(stream_item(0)->url,saved) && stream_item(0)->debridServico==DEBRID_TB+1);
  fontefalha_sessao_liberar(policy); stream_definir_lista(NULL,0);
  puts("PASS native failover: current TorBox positive control succeeds; old epoch publication is rejected even with unchanged list generation");
  return 0;
}
