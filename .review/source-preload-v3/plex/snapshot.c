// Review-only: actual Plex snapshots/protocol/parsers with deterministic HTTP.
// No real server/account and no native player; timeline/publication invariants
// are checked against the actual Plex integration state in this translation unit.
#ifndef SOURCE_PLEX
#define SOURCE_PLEX "plex.c"
#endif
#include SOURCE_PLEX
#include <assert.h>
#include <stdatomic.h>

struct RedeGrupo { _Atomic unsigned refs, geracao; _Atomic int cancelado; };
struct RedeJob { RedeGrupo *grupo; unsigned geracao; _Atomic unsigned refs; _Atomic int cancelado; };
RedeGrupo *rede_grupo_criar(void) { RedeGrupo *g=calloc(1,sizeof *g); assert(g);g->refs=1;g->geracao=1;return g; }
void rede_grupo_soltar(RedeGrupo *g) { if(g&&atomic_fetch_sub(&g->refs,1)==1)free(g); }
uint64_t rede_grupo_avancar(RedeGrupo *g) { return atomic_fetch_add(&g->geracao,1)+1; }
void rede_grupo_cancelar(RedeGrupo *g) { atomic_store(&g->cancelado,1); }
RedeJob *rede_job_criar(RedeGrupo *g) { RedeJob *j=calloc(1,sizeof *j);assert(j);j->grupo=g;j->geracao=atomic_load(&g->geracao);j->refs=1;atomic_fetch_add(&g->refs,1);return j; }
void rede_job_reter(RedeJob *j) { atomic_fetch_add(&j->refs,1); }
void rede_job_soltar(RedeJob *j) { if(j&&atomic_fetch_sub(&j->refs,1)==1){rede_grupo_soltar(j->grupo);free(j);} }
void rede_job_cancelar(RedeJob *j) { if(j)atomic_store(&j->cancelado,1); }
RedeErro rede_job_estado(RedeJob *j) { if(!j)return REDE_OK;if(j->cancelado||j->grupo->cancelado)return REDE_CANCELADO;return j->geracao==j->grupo->geracao?REDE_OK:REDE_GERACAO; }
unsigned rede_pedido_capacidades(void) { return REDE_CAP_JOB; }
void rede_resposta_limpar(RedeResposta *r) { free(r->corpo);free(r->cabecalhos);memset(r,0,sizeof *r); }
JfBackend jellyfin_backend(void) { return JF_BACKEND_HOST; }
const char *jellyfin_backend_nome(JfBackend b) { (void)b; return "fixture"; }
char *dados_ler(const char *n) { (void)n;return NULL; }
char *dados_caminho(char *out,unsigned n,const char *f) { snprintf(out,n,"/tmp/%s",f);return out; }
int dados_apagar(const char *n) { (void)n;return 1; }
void dados_fs_travar(void) {}
void dados_fs_liberar(void) {}
static pthread_mutex_t perfilTrava;
static _Atomic int perfilAtual=1;
static _Atomic uint64_t perfilEpoch=1;
int perfis_ativo(void) { return atomic_load(&perfilAtual); }
uint64_t perfis_sessao_geracao(void) { return atomic_load(&perfilEpoch); }
int perfis_sessao_atual(uint64_t g) { return g==atomic_load(&perfilEpoch); }
int perfis_sessao_travar(uint64_t g) { pthread_mutex_lock(&perfilTrava);if(perfis_sessao_atual(g))return 1;pthread_mutex_unlock(&perfilTrava);return 0; }
void perfis_sessao_destravar(void) { pthread_mutex_unlock(&perfilTrava); }
static _Atomic int requests, blocked, entered, released, callerCancel, expectPreparo=1;
enum { ANSWER_OK, ANSWER_NO_EPISODE, ANSWER_EMPTY, ANSWER_FORMAT, ANSWER_AUTH, ANSWER_NETWORK };
static _Atomic int answer;
static const char *episodes="{\"MediaContainer\":{\"Metadata\":[{\"ratingKey\":\"200\",\"parentIndex\":1,\"index\":2}]}}";
static const char *media="{\"MediaContainer\":{\"Metadata\":[{\"ratingKey\":\"200\",\"duration\":120000,\"Media\":[{\"height\":1080,\"container\":\"mp4\",\"videoCodec\":\"h264\",\"audioCodec\":\"aac\",\"Part\":[{\"key\":\"/library/parts/1/file.mp4\",\"size\":123456}]}]}]}}";
int rede_pedir(const RedePedido *p,RedeResposta *r) {
  assert(p->job && (!!p->parar == !!expectPreparo));
  if(expectPreparo)assert(p->parar_usuario);
  assert(strstr(p->url,"http://server-A/") && !strstr(p->url,"server-B"));
  int oldToken=0,oldDevice=0;
  for(int i=0;p->cabecalhos&&p->cabecalhos[i];i++) {
    oldToken |= !strcmp(p->cabecalhos[i],"X-Plex-Token: test-A");
    oldDevice |= !strcmp(p->cabecalhos[i],"X-Plex-Client-Identifier: device-A");
  }
  assert(oldToken&&oldDevice);
  atomic_fetch_add(&requests,1);atomic_store(&entered,1);
  while(blocked&&!released&&rede_job_estado(p->job)==REDE_OK&&!(p->parar&&p->parar(p->parar_usuario)))usleep(1000);
  memset(r,0,sizeof *r);r->erro=rede_job_estado(p->job);
  if(p->parar&&p->parar(p->parar_usuario))r->erro=REDE_CANCELADO;
  if(answer==ANSWER_NETWORK)r->erro=REDE_TRANSPORTE;
  r->status=answer==ANSWER_AUTH?401:200;
  const char *body=strstr(p->url,"allLeaves")?episodes:media;
  if(answer==ANSWER_NO_EPISODE&&strstr(p->url,"allLeaves"))body="{\"MediaContainer\":{\"Metadata\":[]}}";
  if(answer==ANSWER_EMPTY&&!strstr(p->url,"allLeaves"))body="{\"MediaContainer\":{\"Metadata\":[{\"Media\":[]}]}}";
  if(answer==ANSWER_FORMAT)body="{}";
  r->corpo=strdup(body);assert(r->corpo);r->n_corpo=strlen(body);return 0;
}
static void markInitialized(void) {}
static PxConta accountA(void) {
  PxConta c={0};strcpy(c.base,"http://server-A");strcpy(c.token,"test-A");strcpy(c.tokenConta,"account-A");
  strcpy(c.servidorId,"0123456789abcdef");strcpy(c.dispositivoId,"device-A");return c;
}
static void reset(void) {
  atomic_store(&perfilAtual,1);atomic_fetch_add(&perfilEpoch,1);
  pthread_mutex_lock(&trava);
  if(grupo)rede_grupo_soltar(grupo);
  grupo=rede_grupo_criar();
  memset(&conta,0,sizeof conta);conta=accountA();perfilLido=1;geracao++;
  free(indice);indice=calloc(2,sizeof *indice);assert(indice);nIndice=2;indiceVersao++;
  strcpy(indice[0].imdb,"tt42");indice[0].tmdb=42;indice[0].tipo='s';strcpy(indice[0].rk,"100");
  strcpy(indice[1].imdb,"tt99");indice[1].tmdb=99;indice[1].tipo='m';strcpy(indice[1].rk,"300");
  estado=PX_EST_CONECTADO;parar=0;
  memset(sessoes,0,sizeof sessoes);sessoes[0].ativo=sessoes[0].iniciado=1;sessoes[0].geracao=geracao;
  sessoes[0].ultPos=55.5;strcpy(sessoes[0].s.url,"http://server-A/current.mp4");
  sessoes[1].ativo=1;sessoes[1].geracao=geracao;sessoes[1].criadoMs=123;
  strcpy(sessoes[1].s.url,"http://server-A/library/parts/1/file.mp4?X-Plex-Token=test-A");
  memset(filaRel,0,sizeof filaRel);nRel=1;iniRel=0;filaRel[0].evento=PX_REL_PROGRESSO;filaRel[0].pos=55.5;
  free(fontesLista);fontesLista=calloc(1,sizeof *fontesLista);assert(fontesLista);nFontes=1;fontesEstado=PX_FONTES_PRONTO;
  strcpy(fontesAlvo,"current-source-target");strcpy(fontesLista[0].url,"https://current-source.invalid/video.mp4");
  pthread_mutex_unlock(&trava);
  atomic_store(&requests,0);atomic_store(&blocked,0);atomic_store(&entered,0);atomic_store(&released,0);atomic_store(&callerCancel,0);atomic_store(&answer,ANSWER_OK);atomic_store(&expectPreparo,1);
}
static int canceled(void *u) { (void)u;return callerCancel; }
static int runQuery(PlexPreparo *p,const char *id,const char *type,Stream **out) { return plex_preparo_consultar(p,id,type,canceled,NULL,out); }
static void validQueries(void) {
  reset();PlexPreparo *p=plex_preparo_capturar();assert(p&&plex_preparo_atual(p));
  Sessao active=sessoes[0],waiting=sessoes[1];Relato reports[PX_FILA_REL];memcpy(reports,filaRel,sizeof reports);
  Stream old=*fontesLista,*oldList=fontesLista,*list=NULL;PxConta oldAccount=conta;unsigned oldGeneration=geracao,oldIndex=indiceVersao;
  assert(runQuery(p,"tt42:1:2","series",&list)==2&&list&&requests==2);
  assert(!strcmp(list[0].provedor,"Plex")&&strstr(list[0].url,"/library/parts/1/file.mp4")&&strstr(list[1].url,"start.m3u8"));
  assert(!memcmp(&active,&sessoes[0],sizeof active)&&!memcmp(&waiting,&sessoes[1],sizeof waiting));
  assert(nRel==1&&iniRel==0&&!memcmp(reports,filaRel,sizeof reports));
  assert(fontesLista==oldList&&nFontes==1&&fontesEstado==PX_FONTES_PRONTO&&!strcmp(fontesAlvo,"current-source-target")&&!memcmp(&old,fontesLista,sizeof old));
  assert(!memcmp(&oldAccount,&conta,sizeof conta)&&oldGeneration==geracao&&oldIndex==indiceVersao);
  free(list);list=NULL;assert(runQuery(p,"tmdb:99","movie",&list)==2&&list&&requests==3);free(list);list=NULL;
  assert(runQuery(p,"tt12345:1:2","series",&list)==0&&!list&&requests==3);
  assert(runQuery(p,"px:01234567.200","series",&list)==0&&!list&&requests==3);
  answer=ANSWER_NO_EPISODE;assert(runQuery(p,"tt42:1:9","series",&list)==0&&!list&&requests==4);
  answer=ANSWER_EMPTY;assert(runQuery(p,"tt99","movie",&list)==0&&!list&&requests==5);
  for(int mode=ANSWER_FORMAT;mode<=ANSWER_NETWORK;mode++){answer=mode;assert(runQuery(p,"tt99","movie",&list)==-1&&!list);}
  callerCancel=1;int before=requests;assert(runQuery(p,"tt99","movie",&list)==-1&&!list&&requests==before);
  plex_preparo_liberar(p);
  // A regular protocol call after the snapshot is freed must not inherit
  // preparation's TLS callback or dereference its old credentials/context.
  expectPreparo=0;callerCancel=0;answer=ANSWER_OK;
  RedeJob *normalJob=rede_job_criar(grupo);PxPlayback *normal=malloc(sizeof *normal);assert(normal);
  assert(px_fontes(&conta,normalJob,"300",normal)==PX_OK&&normal->n==2);
  free(normal);rede_job_soltar(normalJob);expectPreparo=1;
  puts("PASS real Plex preparation: public ID/episode matching, captured requests, valid empty vs failure, private IDs cold, active/waiting/source/report state untouched");
}
static void contextChanges(void) {
  for(int change=0;change<5;change++) {
    reset();PlexPreparo *p=plex_preparo_capturar();assert(p);
    if(change==0){pthread_mutex_lock(&trava);strcpy(conta.token,"new-token");pthread_mutex_unlock(&trava);}
    if(change==1){pthread_mutex_lock(&trava);indiceVersao++;strcpy(indice[0].rk,"101");pthread_mutex_unlock(&trava);}
    if(change==2){pthread_mutex_lock(&perfilTrava);perfilAtual=2;perfilEpoch++;perfilAtual=1;perfilEpoch++;pthread_mutex_unlock(&perfilTrava);}
    if(change==3){pthread_mutex_lock(&trava);avancarLocked();pthread_mutex_unlock(&trava);}
    if(change==4){pthread_mutex_lock(&trava);strcpy(conta.base,"http://server-B");strcpy(conta.dispositivoId,"device-B");pthread_mutex_unlock(&trava);}
    Stream *list=NULL;assert(!plex_preparo_atual(p)&&runQuery(p,"tt42:1:2","series",&list)==-1&&!list&&!requests);
    plex_preparo_liberar(p);
  }
  reset();pthread_mutex_lock(&trava);free(indice);indice=NULL;nIndice=0;indiceVersao++;pthread_mutex_unlock(&trava);
  PlexPreparo *p=plex_preparo_capturar();assert(p&&plex_preparo_atual(p));Stream *list=NULL;
  assert(runQuery(p,"tt42:1:2","series",&list)==0&&!list&&!requests);plex_preparo_liberar(p);
  puts("PASS real Plex preparation: token/server/device/index/opening/generation invalidation; index-not-ready is valid empty");
}
static void boundedSessions(void) {
  for(int freeRows=0;freeRows<=1;freeRows++) {
    reset();
    pthread_mutex_lock(&trava);
    // Both new source URLs require rows; one available row must not permit
    // a partial registration, and no foreground playing/waiting row is stolen.
    for(int i=1;i<PX_SESSOES-freeRows;i++) {
      sessoes[i].ativo=1;sessoes[i].geracao=geracao;sessoes[i].iniciado=i%2;
      snprintf(sessoes[i].s.url,sizeof sessoes[i].s.url,"http://server-A/foreground-%d.mp4",i);
    }
    Sessao before[PX_SESSOES];memcpy(before,sessoes,sizeof before);
    Relato reports[PX_FILA_REL];memcpy(reports,filaRel,sizeof reports);
    pthread_mutex_unlock(&trava);
    PlexPreparo *p=plex_preparo_capturar();assert(p);Stream *list=NULL;
    assert(runQuery(p,"tt42:1:2","series",&list)==-1&&!list&&requests==2);
    assert(!memcmp(before,sessoes,sizeof before)&&nRel==1&&!memcmp(reports,filaRel,sizeof reports));
    assert(fontesEstado==PX_FONTES_PRONTO&&nFontes==1&&!strcmp(fontesAlvo,"current-source-target"));
    plex_preparo_liberar(p);
  }
  // Successful unused snapshots remain provisional for a later foreground
  // open. If repeated new HLS sessions fill this bounded registry, preparation
  // falls back rather than handing out a URL without timeline/stop ownership.
  reset();Sessao active=sessoes[0],waiting=sessoes[1];int full=0;
  for(int i=0;i<PX_SESSOES+1;i++) {
    PlexPreparo *p=plex_preparo_capturar();assert(p);Stream *list=NULL;
    int n=runQuery(p,"tt42:1:2","series",&list);
    if(n<0){assert(n==-1&&!list);full=1;plex_preparo_liberar(p);break;}
    assert(n==2&&list&&sessaoDaUrlLocked(list[0].url)&&sessaoDaUrlLocked(list[1].url));
    free(list);plex_preparo_liberar(p);
    assert(!memcmp(&active,&sessoes[0],sizeof active)&&!memcmp(&waiting,&sessoes[1],sizeof waiting));
    assert(atomic_load(&grupo->refs)==1);
  }
  assert(full&&!memcmp(&active,&sessoes[0],sizeof active)&&!memcmp(&waiting,&sessoes[1],sizeof waiting));
  puts("PASS real Plex preparation: full/partial session capacity rejects atomically without eviction; every successful prepared URL is tracked");
}
typedef struct { PlexPreparo *p;Stream *list;int result; } Query;
static void *queryThread(void *u) { Query *q=u;q->result=runQuery(q->p,"tt42:1:2","series",&q->list);return NULL; }
static void waitEntered(void) { for(int i=0;i<2000&&!entered;i++)usleep(1000);assert(entered); }
static void inFlightChanges(void) {
  for(int change=0;change<4;change++) {
    reset();Query q={.p=plex_preparo_capturar()};assert(q.p);blocked=1;
    Sessao active=sessoes[0],waiting=sessoes[1];Relato reports[PX_FILA_REL];memcpy(reports,filaRel,sizeof reports);
    pthread_t th;assert(!pthread_create(&th,NULL,queryThread,&q));waitEntered();
    if(change==0)callerCancel=1;
    if(change==1){pthread_mutex_lock(&perfilTrava);perfilAtual=2;perfilEpoch++;perfilAtual=1;perfilEpoch++;pthread_mutex_unlock(&perfilTrava);}
    if(change==2){pthread_mutex_lock(&trava);strcpy(conta.base,"http://server-B");strcpy(conta.token,"test-B");pthread_mutex_unlock(&trava);}
    if(change==3){pthread_mutex_lock(&trava);indiceVersao++;pthread_mutex_unlock(&trava);}
    pthread_join(th,NULL);assert(q.result==-1&&!q.list&&requests==1);
    assert(!memcmp(&active,&sessoes[0],sizeof active)&&!memcmp(&waiting,&sessoes[1],sizeof waiting));
    assert(nRel==1&&!memcmp(reports,filaRel,sizeof reports));
    assert(fontesEstado==PX_FONTES_PRONTO&&nFontes==1&&!strcmp(fontesAlvo,"current-source-target"));
    plex_preparo_liberar(q.p);
  }
  puts("PASS real Plex preparation: in-flight caller cancel/A-B-A/credential/index change interrupts captured HTTP and publishes no sessions or sources");
}
int main(void) {
  pthread_mutexattr_t a;assert(!pthread_mutexattr_init(&a));assert(!pthread_mutexattr_settype(&a,PTHREAD_MUTEX_RECURSIVE));assert(!pthread_mutex_init(&perfilTrava,&a));pthread_mutexattr_destroy(&a);
  pthread_once(&uma,markInitialized);validQueries();contextChanges();boundedSessions();inFlightChanges();
  free(indice);free(fontesLista);rede_grupo_soltar(grupo);grupo=NULL;return 0;
}
