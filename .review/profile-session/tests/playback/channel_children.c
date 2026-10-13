// The real stream child probes, with deliberately late network replies.
#define main legacy_failed_fixture_main
#define dados_gravar legacy_dados_gravar
#define dados_ler legacy_dados_ler
#define rede_baixar_st legacy_rede_baixar_st
#include "tests/fontesmortas.c"
#undef main
#undef dados_gravar
#undef dados_ler
#undef rede_baixar_st
#define main legacy_channel_ranking_main
#include "tests/canalfila.c"
#undef main
#include "perfis.h"
#include <unistd.h>

static pthread_mutex_t net_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t net_condition = PTHREAD_COND_INITIALIZER;
static int calls, started, released, block_requests, ignore_cancel;
static char observed_url[16][4096], observed_header[16][512];
static const volatile int *observed_cancel;
static _Atomic int done;
static int selected;
static uint64_t captured;
void fil_definir_perfil(int p) {(void)p;}
void fontepref_definir_perfil(int p) {(void)p;}
void legmem_definir_perfil(int p) {(void)p;}
void arteesc_definir_perfil(int p) {(void)p;}
void plugins_perfil_mudou(void) {}
void servidores_perfil_trocou(void) {}
int dados_gravar(const char *n,const char *s) {(void)n;(void)s;return 1;}
char *dados_ler(const char *n) {(void)n;return NULL;}
const char *ajustes_qualidade(void) {return "Automatica";}
int ajustes_tamanho_max_gb(void) {return 0;}
int ajustes_tamanho_min_gb(void) {return 0;}
int ajustes_livetv_resolucao(void) {return 0;}
int ajustes_limite_dinamico(void) {return 0;}
int ajustes_tela_max_resolucao(void) {return 0;}
void marco(const char *s) {(void)s;}
char *rede_baixar_medido_controle(const char *url,int seconds,const char *const *headers,const RedeControle *control,RedeMedida *measure) {
  assert(seconds == CANAL_PRAZO_S && control && control->cancelado);
  pthread_mutex_lock(&net_mutex);
  int i=calls++;assert(i<16);
  snprintf(observed_url[i],sizeof observed_url[i],"%s",url);
  snprintf(observed_header[i],sizeof observed_header[i],"%s",headers&&headers[0]?headers[0]:"");
  observed_cancel=control->cancelado; started++;
  pthread_cond_broadcast(&net_condition);
  while (block_requests && !released && (ignore_cancel || !__atomic_load_n(control->cancelado,__ATOMIC_ACQUIRE))) {
    pthread_mutex_unlock(&net_mutex);usleep(1000);pthread_mutex_lock(&net_mutex);
  }
  int canceled=__atomic_load_n(control->cancelado,__ATOMIC_ACQUIRE);
  pthread_mutex_unlock(&net_mutex);
  *measure=(RedeMedida){.status=200,.cancelado=ignore_cancel?0:canceled};
  if(canceled&&!ignore_cancel)return NULL;
  if (strstr(url,"/muda")) return NULL;
  if (strstr(url,"/morta")) return strdup("#EXTM3U\n");
  if (strstr(url,"/grande")) { char *b=malloc(5001);assert(b);memset(b,'x',5000);b[5000]=0;return b; }
  return strdup("#EXTM3U\n#EXTINF:1,live\nhttps://segments.fixture.invalid/a.ts\n");
}
static void *probe(void *unused) {(void)unused;selected=stream_canal_primeira_viva_sessao(8,captured);atomic_store(&done,1);return NULL;}
static void before_profile(void) {stream_invalidar("profile opens");}
static void profile(int number) {perfis_definir_ativo(number);}
static void sources(const char *owner,int count) {
  Stream *streams=calloc((size_t)count,sizeof *streams);assert(streams);
  for(int i=0;i<count;i++) {
    snprintf(streams[i].url,sizeof streams[i].url,"https://%s.fixture.invalid/%d.m3u8",owner,i);
    snprintf(streams[i].cabecalhos,sizeof streams[i].cabecalhos,"Authorization: Bearer PROFILE_%s",owner);
    snprintf(streams[i].provedor,sizeof streams[i].provedor,"Fixture");
    streams[i].altura=1080;
  }
  stream_definir_lista(streams,count);free(streams);
}
static void wait_started(int count) {
  for(int i=0;i<3000;i++){pthread_mutex_lock(&net_mutex);int yes=started>=count;pthread_mutex_unlock(&net_mutex);if(yes)return;usleep(1000);}assert(!"children did not enter transport");
}
static void wait_done(void) {for(int i=0;i<3000;i++){if(atomic_load(&done))return;usleep(1000);}assert(!"child probes did not cancel/finish");}
int main(void) {
  perfis_ao_abrir_sessao(before_profile);profile(1);sources("A",2);captured=perfis_sessao_geracao();
  pthread_t worker;block_requests=1;ignore_cancel=1;pthread_create(&worker,NULL,probe,NULL);wait_started(2);
  profile(2);sources("B",1);assert(observed_cancel&&__atomic_load_n(observed_cancel,__ATOMIC_ACQUIRE));
  pthread_mutex_lock(&net_mutex);released=1;pthread_mutex_unlock(&net_mutex);wait_done();pthread_join(worker,NULL);
  assert(selected==-1);assert(canalOrdemN==0&&classeEscolhida==0);
  for(int i=0;i<2;i++){assert(strstr(observed_url[i],"A.fixture.invalid"));assert(strstr(observed_header[i],"PROFILE_A"));}
  assert(stream_canal_primeira_viva_sessao(8,perfis_sessao_geracao())==0);
  assert(stream_canal_proxima(0)==-1&&stream_canal_classe_escolhida()==1);
  puts("PASS: immutable A URL/headers, late A replies cannot classify or reorder B's shorter list");

  profile(1);sources("A",2);captured=perfis_sessao_geracao();
  pthread_mutex_lock(&net_mutex);started=0;released=0;block_requests=1;ignore_cancel=0;pthread_mutex_unlock(&net_mutex);atomic_store(&done,0);
  pthread_create(&worker,NULL,probe,NULL);wait_started(2);profile(2);sources("B",1);wait_done();pthread_join(worker,NULL);
  assert(selected==-1&&!canalOrdemN);
  puts("PASS: profile activation cancels blocked channel requests without waiting for their timeout");

  profile(1);sources("A",1);uint64_t old=perfis_sessao_geracao();profile(2);profile(1);sources("A",1);int previous_calls=calls;
  assert(stream_canal_primeira_viva_sessao(8,old)==-1);
  assert(calls==previous_calls);
  puts("PASS: dispatch-time epochs reject delayed channel entry after A to B to A");
  block_requests=0;legacy_channel_ranking_main(0,NULL);
  puts("PASS: existing uncertain/dead/mute/live ranking and watchdog timeout behavior preserved");
  stream_definir_lista(NULL,0);puts("channel child isolation: PASS");
}
