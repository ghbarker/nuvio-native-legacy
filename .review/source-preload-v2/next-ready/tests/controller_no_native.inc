#include <pthread.h>
#include <errno.h>
static int fixtureCriarFio(pthread_t *,const pthread_attr_t *,void *(*)(void *),void *);
#define pthread_create fixtureCriarFio
#include "../src/proximopreparo.c"
#undef pthread_create
#include "../src/debrid.h"
#include <assert.h>
#include <unistd.h>

struct AddonsConsulta { unsigned versao; char conta[80]; };
struct StreamPreparo {
  Stream *lista; int n, t, e; uint64_t cfg;
  NvPerfilSessao sessaoPerfil;
  FonteFalhaSessao *politica;
};
static pthread_t ui;
static _Atomic unsigned relogio = 1000;
static _Atomic int consultas, resolucoes, entrouMeta, entrouResolver;
static _Atomic int reservas, http;
static _Atomic int falhaFioRenovar;
static _Atomic int bloquearMeta, bloquearResolver;
static _Atomic uint64_t perfilEpoch = 1;
static pthread_mutex_t perfilTrava = PTHREAD_MUTEX_INITIALIZER;
static int mudarPerfilCriar;
static NvPerfilSessao snapshotPerfil;
static int ligado = 1, manual, nFontes = 160, semFonte, semSnapshot, metaResposta, politicaInterna;
static uint64_t cfg = 100;
// The real StreamPreparo fingerprint is separately tested by the resolver
// fixture. This boundary exposes ONLY a transport-epoch change to the actual
// next controller, with account, profile, credentials and settings unchanged.
static DebridContexto debridAtual;
static FontecacheEscopo escopo = {"conta-A", 1, 10, 20, 1};
static int iniciaBuffer, cancelaBuffer, renova, renovaFalha, bufferFalha, estadoBuffer = 2;
static unsigned tokenAtual, sequencia = 10;
static int snapshotT, snapshotE, liberaPolitica;
uint64_t perfis_sessao_geracao(void) { return atomic_load(&perfilEpoch); }
int perfis_sessao_atual(uint64_t epoch) { return epoch == atomic_load(&perfilEpoch); }
int perfis_sessao_travar(uint64_t epoch) {
  pthread_mutex_lock(&perfilTrava);
  if (perfis_sessao_atual(epoch)) return 1;
  pthread_mutex_unlock(&perfilTrava); return 0;
}
void perfis_sessao_destravar(void) { pthread_mutex_unlock(&perfilTrava); }
static void alterarPerfil(void) {
  pthread_mutex_lock(&perfilTrava); atomic_fetch_add(&perfilEpoch, 1); pthread_mutex_unlock(&perfilTrava);
}
static int fixtureCriarFio(pthread_t *t,const pthread_attr_t *a,void *(*f)(void *),void *u) {
  if(f==trabalhar && atomic_load(&((ProximoJob *)u)->estado)==PP_RENOVANDO &&
     atomic_exchange(&falhaFioRenovar,0))return EAGAIN;
  return pthread_create(t,a,f,u);
}

Uint32 SDL_GetTicks(void) { return atomic_load(&relogio); }
int ajustes_fonte_precarregar(void) { return ligado; }
int ajustes_fonte_manual(void) { return manual; }
void addons_capturar_escopo(FontecacheEscopo *out) {
  assert(pthread_equal(ui,pthread_self())); *out = escopo;
  out->sessao=perfis_sessao_geracao();
}
AddonsConsulta *addons_consulta_capturar(void) {
  assert(pthread_equal(ui,pthread_self()));
  if (semSnapshot) return NULL;
  AddonsConsulta *a=calloc(1,sizeof *a); assert(a);
  a->versao=escopo.addons; snprintf(a->conta,sizeof a->conta,"%s",escopo.conta); return a;
}
int addons_consulta_atual(const AddonsConsulta *a) { return a && a->versao==escopo.addons && !strcmp(a->conta,escopo.conta); }
void addons_consulta_liberar(AddonsConsulta *a) { free(a); }
int addons_consulta_consultar(const AddonsConsulta *a,const char *id,const char *tipo,int fios,
                             int (*c)(void*),void *u,void *saida) {
  Stream **out = saida;
  assert(!pthread_equal(ui,pthread_self()));
  assert(a && !strcmp(id,"tt42:1:2") && !strcmp(tipo,"series") && fios==2);
  atomic_fetch_add(&consultas,1); atomic_store(&entrouMeta,1);
  while (atomic_load(&bloquearMeta) && !c(u)) usleep(1000);
  if (c(u)) return -1;
  if(metaResposta) {
    *out=metaResposta==2?calloc(1,sizeof **out):NULL;
    if(*out)strcpy((*out)[0].url,"https://partial.invalid/never-promoted.mp4");
    return metaResposta==1?0:-1;
  }
  *out=calloc((size_t)nFontes,sizeof **out); assert(*out);
  for(int i=0;i<nFontes;i++) {
    snprintf((*out)[i].infoHash,sizeof (*out)[i].infoHash,"hash-%d",i);
    snprintf((*out)[i].addonChave,sizeof (*out)[i].addonChave,"%s",i<2?"addon-A":"addon-B");
    snprintf((*out)[i].provedor,sizeof (*out)[i].provedor,"%s",i<2?"A":"B");
    (*out)[i].addonOrdem=i<2?0:1; (*out)[i].fileIdx=i;
  }
  return nFontes;
}
uint64_t stream_preparo_config_atual(void) { assert(pthread_equal(ui,pthread_self())); return cfg ^ ((uint64_t)debridAtual.busca << 32); }
int stream_preparo_contexto_atual(const StreamPreparo *p) {
  return p && p->cfg==stream_preparo_config_atual() && perfis_sessao_atual(p->sessaoPerfil);
}
StreamPreparo *stream_preparo_criar_alvo(const Stream *s,int n,int preferred,const char *id) {
  assert(pthread_equal(ui,pthread_self())); assert(preferred==-1 && !strcmp(id,"tt42:1:2"));
  if (mudarPerfilCriar) { mudarPerfilCriar=0; alterarPerfil(); }
  StreamPreparo *p=calloc(1,sizeof *p); assert(p);
  p->lista=malloc((size_t)n*sizeof *s); assert(p->lista); memcpy(p->lista,s,(size_t)n*sizeof *s);
  p->n=n; p->cfg=stream_preparo_config_atual();
  p->sessaoPerfil=snapshotPerfil=perfis_sessao_geracao();
  assert(stream_preparo_contexto_atual(p));
  FonteFalhaConfig f={.provedores=1,.addons=1,.ordem={DEBRID_TB,DEBRID_PM,DEBRID_RD,DEBRID_AD},
                      .limites={2,3,2,2},.nAddons=2};
  strcpy(f.addon[0].chave,"addon-A"); f.addon[0].limite=10;
  strcpy(f.addon[1].chave,"addon-B"); f.addon[1].limite=10;
  p->politica=fontefalha_sessao_nova(&f,(1u<<DEBRID_TB)|(1u<<DEBRID_PM),-1); assert(p->politica); return p;
}
void stream_preparo_definir_episodio(StreamPreparo *p,int t,int e) { assert(pthread_equal(ui,pthread_self())); p->t=t;p->e=e; }
int stream_preparo_resolver(StreamPreparo *p,int (*c)(void*),void *u,Stream *out,int *index,FonteFalhaSessao **policy) {
  assert(!pthread_equal(ui,pthread_self())); assert(p->t==1 && p->e==2);
  snapshotT=p->t; snapshotE=p->e;
  atomic_fetch_add(&resolucoes,1); atomic_store(&entrouResolver,1);
  while(atomic_load(&bloquearResolver) && !c(u)) usleep(1000);
  if(c(u)) return -1;
  FonteFalhaCandidata candidates[4];
  for(int i=0;i<4;i++) candidates[i]=(FonteFalhaCandidata){.indice=i,.nativa=1,.ordemAddon=i<2?0:1,
                 .identidade=p->lista[i].infoHash};
  for(int i=0;i<4;i++) strcpy(candidates[i].addon,p->lista[i].addonChave);
  int antes=fontefalha_tentativas(p->politica),nTB=0,nPM=0; FonteFalhaRota route;
  while(fontefalha_proxima(p->politica,candidates,4,1,&route)==FONTEFALHA_PROXIMA) {
    assert(fontefalha_tentar(p->politica,&candidates[route.candidata],route.servico));
    atomic_fetch_add(&reservas,1); atomic_fetch_add(&http,1);
    if(route.servico==DEBRID_TB) { nTB++;continue; }
    assert(route.servico==DEBRID_PM); nPM++;
    if(semFonte) continue;
    assert((antes ? nTB==0 : nTB==3) && nPM==1);
    *index=route.indice; *out=p->lista[*index]; out->debridServico=DEBRID_PM+1;
    snprintf(out->url,sizeof out->url,"https://video.test/next-%d.mkv",*index);
    assert(fontefalha_url_registrar(p->politica,out->url));
    p->lista[*index]=*out;
    if(!politicaInterna) { *policy=p->politica; p->politica=NULL; }
    return 1;
  }
  *index=-1; *policy=p->politica;p->politica=NULL; return 0;
}
int stream_preparo_renovar(StreamPreparo *p,int index,int (*c)(void*),void *u,Stream *out) {
  assert(!pthread_equal(ui,pthread_self())); if(c(u)) return -1;
  assert(index>=0 && index<p->n); renova++;
  if (renovaFalha) return 0;
  *out=p->lista[index]; strcpy(out->url,"https://video.test/renewed.mkv"); p->lista[index]=*out; return 1;
}
Stream *stream_preparo_tomar_lista(StreamPreparo *p,int *n) { *n=p->n;Stream *r=p->lista;p->lista=NULL;p->n=0;return r; }
int stream_preparo_continuar(StreamPreparo *p,FonteFalhaSessao **policy) {
  assert(pthread_equal(ui,pthread_self()));
  if(*policy) { assert(!p->politica);p->politica=*policy;*policy=NULL; }
  return p->politica!=NULL;
}
FonteFalhaSessao *stream_preparo_tomar_politica(StreamPreparo *p) {FonteFalhaSessao *r=p->politica;p->politica=NULL;return r;}
void stream_preparo_liberar(StreamPreparo *p) { if(p) {free(p->lista);fontefalha_sessao_liberar(p->politica);free(p);} }
int fontecache_vod_pegar(const char *id,const char *tipo,const char *origem,const FontecacheEscopo *e,
                        Stream **list,int *n,Uint32 *idade) {
  (void)id;(void)tipo;(void)origem;(void)e;*list=NULL;*n=0;*idade=0;return FC_NADA;
}
void fontecache_vod_guardar(const char *id,const char *tipo,const char *origem,const FontecacheEscopo *e,
                            const Stream *list,int n,Uint32 when) {
  assert(pthread_equal(ui,pthread_self()));(void)id;(void)tipo;(void)origem;(void)e;(void)list;(void)when;
  assert(n==nFontes); // Optional cache must not truncate the independently resolved list.
}
unsigned player_precarregar_novo_token(void) { return ++sequencia; }
int player_precarregar_proxima_fonte(int catalogo,int t,int e,const Stream *s,unsigned token) {
  assert(pthread_equal(ui,pthread_self()));assert(catalogo==0&&t==1&&e==2&&s->debridServico==DEBRID_PM+1);
  iniciaBuffer++; if(bufferFalha)return 0;tokenAtual=token;return 1;
}
int player_precarregar_estado(unsigned token) { return token==tokenAtual ? estadoBuffer :0; }
void player_precarregar_cancelar(unsigned token) { cancelaBuffer++; if(token==tokenAtual)tokenAtual=0; }

static void reset(void) {
  proximopreparo_encerrar();
  atomic_store(&relogio,1000);atomic_store(&consultas,0);atomic_store(&resolucoes,0);
  atomic_store(&reservas,0);atomic_store(&http,0);
  atomic_store(&falhaFioRenovar,0);
  atomic_store(&entrouMeta,0);atomic_store(&entrouResolver,0);
  atomic_store(&bloquearMeta,0);atomic_store(&bloquearResolver,0);
  atomic_store(&perfilEpoch,1);mudarPerfilCriar=0;snapshotPerfil=0;
  ligado=1;manual=semFonte=semSnapshot=bufferFalha=metaResposta=politicaInterna=0;estadoBuffer=2;cfg=100;
  memset(&debridAtual,0,sizeof debridAtual);debridAtual.busca=1;debridAtual.sessao=7;
  strcpy(debridAtual.chave[DEBRID_TB],"captured-test-only-key");
  escopo=(FontecacheEscopo){"conta-A",1,10,20,1};
  iniciaBuffer=cancelaBuffer=renova=renovaFalha=0;tokenAtual=0;
}
static void agendar(void) { assert(proximopreparo_agendar(0,1,2,"tt42:1:1","tt42:1:2",&escopo)); }
static void prontoFonte(void) {
  for(int i=0;i<2000;i++) {
    proximopreparo_passo("tt42:1:1");
    if(!proximopreparo_em_curso("tt42:1:2")) {
      proximopreparo_passo("tt42:1:1");
      if(!proximopreparo_em_curso("tt42:1:2"))break;
    }
    usleep(1000);
  }
  assert(!proximopreparo_em_curso("tt42:1:2"));
}
static void entrou(_Atomic int *flag) { for(int i=0;i<2000&&!atomic_load(flag);i++)usleep(1000);assert(atomic_load(flag)); }

int main(void) {
  ui=pthread_self();reset();
  ligado=0;assert(!proximopreparo_agendar(0,1,2,"tt42:1:1","tt42:1:2",&escopo));assert(!consultas);
  reset();agendar();prontoFonte();
  assert(consultas==1&&resolucoes==1&&iniciaBuffer==1&&snapshotT==1&&snapshotE==2);
  for(int i=0;i<20;i++)proximopreparo_passo("tt42:1:1");assert(iniciaBuffer==1);
  ProximoPreparoResultado r;assert(proximopreparo_consumir("tt42:1:2",&r));
  assert(r.n==160 && (size_t)r.n*sizeof(Stream)>FONTECACHE_VOD_BYTES && r.indiceFonte>=0 && r.token);
  assert(r.sessaoPerfil==perfis_sessao_geracao());
  assert(fontefalha_tentativas(r.politica)==4); // 3 TB across addons A/B, then PM.
  unsigned moved=r.token;assert(!proximopreparo_consumir("tt42:1:2",&(ProximoPreparoResultado){0}));
  assert(!cancelaBuffer && tokenAtual==moved);proximopreparo_liberar_resultado(&r);assert(cancelaBuffer==1);
  // A late source completion/seek-to-credits must preserve already READY
  // bytes, token and the same spent ledger; the caller may still refresh later.
  reset();atomic_store(&bloquearMeta,1);agendar();entrou(&entrouMeta);
  assert(!proximopreparo_renovar("tt42:1:1","tt42:1:2"));
  assert(!proximo->renovou && !resolucoes && !renova && !iniciaBuffer);
  atomic_store(&relogio,56000);atomic_store(&bloquearMeta,0);prontoFonte();
  unsigned freshToken=tokenAtual;unsigned freshEpoch=proximopreparo_geracao();
  for(int i=0;i<25;i++)assert(!proximopreparo_renovar("tt42:1:1","tt42:1:2"));
  assert(tokenAtual==freshToken && !proximo->renovou && iniciaBuffer==1 && !cancelaBuffer && !renova);
  assert(reservas==4 && http==4 && resolucoes==1 && proximopreparo_geracao()==freshEpoch);
  assert(proximopreparo_consumir("tt42:1:2",&r) && r.token==freshToken && fontefalha_tentativas(r.politica)==4);
  proximopreparo_liberar_resultado(&r);
  // Resolver and native loading each retain their existing work without
  // queuing a second worker or native preparation.
  reset();atomic_store(&bloquearResolver,1);agendar();
  for(int i=0;i<2000&&!atomic_load(&entrouResolver);i++){proximopreparo_passo("tt42:1:1");usleep(1000);}
  entrou(&entrouResolver);assert(!proximopreparo_renovar("tt42:1:1","tt42:1:2"));
  assert(!proximo->renovou && !renova && resolucoes==1 && !reservas && !http);
  atomic_store(&bloquearResolver,0);estadoBuffer=1;prontoFonte();freshToken=tokenAtual;
  atomic_fetch_add(&relogio,5000);
  for(int i=0;i<25;i++)assert(!proximopreparo_renovar("tt42:1:1","tt42:1:2"));
  assert(tokenAtual==freshToken && iniciaBuffer==1 && !cancelaBuffer && !renova && reservas==4 && http==4);
  assert(proximopreparo_consumir("tt42:1:2",&r) && r.token==freshToken && fontefalha_tentativas(r.politica)==4);
  proximopreparo_liberar_resultado(&r); // loading is partial reuse, not READY.
  // TTL is elapsed real clock, with exact expiry and unsigned tick wrap.
  reset();agendar();prontoFonte();freshToken=tokenAtual;
  atomic_fetch_add(&relogio,FA_BUFFER_VALE_MS-1);
  assert(!proximopreparo_renovar("tt42:1:1","tt42:1:2"));
  atomic_fetch_add(&relogio,1);
  assert(proximopreparo_renovar("tt42:1:1","tt42:1:2"));prontoFonte();
  assert(tokenAtual!=freshToken && cancelaBuffer==1 && iniciaBuffer==2 && renova==1 && reservas==4 && http==4);
  assert(!proximopreparo_renovar("tt42:1:1","tt42:1:2"));
  assert(proximopreparo_consumir("tt42:1:2",&r) && fontefalha_tentativas(r.politica)==4);proximopreparo_liberar_resultado(&r);
  reset();atomic_store(&relogio,UINT32_MAX-1000u);agendar();prontoFonte();freshToken=tokenAtual;
  atomic_fetch_add(&relogio,5000);
  assert(!proximopreparo_renovar("tt42:1:1","tt42:1:2") && !cancelaBuffer && !renova);
  assert(proximopreparo_consumir("tt42:1:2",&r) && r.token==freshToken);proximopreparo_liberar_resultado(&r);
  // Backend state0 is cancellation/unavailability, not a failed route.
  // Retry the still-checked source once without resolver HTTP or new budget.
  reset();agendar();prontoFonte();freshToken=tokenAtual;estadoBuffer=0;
  assert(proximopreparo_renovar("tt42:1:1","tt42:1:2"));estadoBuffer=2;
  assert(tokenAtual!=freshToken && iniciaBuffer==2 && cancelaBuffer==1 && !renova && resolucoes==1 && reservas==4 && http==4);
  assert(!proximopreparo_renovar("tt42:1:1","tt42:1:2"));
  assert(proximopreparo_consumir("tt42:1:2",&r) && fontefalha_tentativas(r.politica)==4);proximopreparo_liberar_resultado(&r);
  // An unavailable backend may refuse the bounded native-only retry too.
  // Count that operation as accepted, keep the checked source, never spin.
  reset();bufferFalha=1;agendar();prontoFonte();assert(iniciaBuffer==1&&!tokenAtual);
  assert(proximopreparo_renovar("tt42:1:1","tt42:1:2")&&proximo->renovou&&!tokenAtual);
  for(int i=0;i<25;i++)assert(!proximopreparo_renovar("tt42:1:1","tt42:1:2"));
  assert(iniciaBuffer==2&&!renova&&resolucoes==1&&reservas==4&&http==4);
  assert(proximopreparo_consumir("tt42:1:2",&r)&&!r.token&&r.indiceFonte>=0&&fontefalha_tentativas(r.politica)==4);
  proximopreparo_liberar_resultado(&r);
  // A native failure observed by renewal continues the SAME spent policy,
  // never refreshes the failed route or resets the provider budgets.
  reset();agendar();prontoFonte();estadoBuffer=-1;
  assert(!proximopreparo_renovar("tt42:1:1","tt42:1:2"));estadoBuffer=2;prontoFonte();
  assert(!renova && resolucoes==2 && reservas==5 && http==5 && iniciaBuffer==2);
  assert(proximopreparo_consumir("tt42:1:2",&r) && fontefalha_tentativas(r.politica)==5 && r.indiceFonte==1);proximopreparo_liberar_resultado(&r);
  reset();agendar();prontoFonte();atomic_fetch_add(&relogio,FA_BUFFER_VALE_MS);assert(proximopreparo_renovar("tt42:1:1","tt42:1:2"));prontoFonte();
  assert(!proximopreparo_renovar("tt42:1:1","tt42:1:2"));assert(renova==1&&resolucoes==1&&iniciaBuffer==2);
  assert(proximopreparo_consumir("tt42:1:2",&r));assert(fontefalha_tentativas(r.politica)==4);proximopreparo_liberar_resultado(&r);
  for(int retained=0;retained<=1;retained++) {
    reset();politicaInterna=retained;agendar();prontoFonte();unsigned renewalEpoch=proximopreparo_geracao();atomic_fetch_add(&relogio,FA_BUFFER_VALE_MS);falhaFioRenovar=1;
    assert(!proximopreparo_renovar("tt42:1:1","tt42:1:2"));
    assert(proximopreparo_geracao()==renewalEpoch&&!proximopreparo_em_curso("tt42:1:2"));
    assert(proximopreparo_consumir("tt42:1:2",&r)&&r.indiceFonte==-2&&!r.token&&r.politica);
    assert(fontefalha_tentativas(r.politica)==4&&reservas==4&&http==4&&resolucoes==1&&renova==0&&iniciaBuffer==1&&cancelaBuffer==1);
    proximopreparo_liberar_resultado(&r);
  }
  reset();agendar();prontoFonte();atomic_fetch_add(&relogio,FA_BUFFER_VALE_MS);renovaFalha=1;assert(proximopreparo_renovar("tt42:1:1","tt42:1:2"));prontoFonte();
  assert(renova==1&&resolucoes==2&&iniciaBuffer==2);
  assert(proximopreparo_consumir("tt42:1:2",&r));assert(fontefalha_tentativas(r.politica)==5&&r.indiceFonte==1);
  proximopreparo_liberar_resultado(&r);
  reset();bufferFalha=1;agendar();prontoFonte();for(int i=0;i<20;i++)proximopreparo_passo("tt42:1:1");assert(iniciaBuffer==1);
  assert(proximopreparo_consumir("tt42:1:2",&r)&&r.indiceFonte>=0&&!r.token);proximopreparo_liberar_resultado(&r);
  reset();agendar();prontoFonte();estadoBuffer=-1;
  assert(!proximopreparo_consumir("tt42:1:2",&r));assert(proximopreparo_em_curso("tt42:1:2"));
  estadoBuffer=2;prontoFonte();assert(resolucoes==2&&iniciaBuffer==2&&cancelaBuffer==1);
  assert(proximopreparo_consumir("tt42:1:2",&r));assert(r.indiceFonte==1&&fontefalha_tentativas(r.politica)==5);
  proximopreparo_liberar_resultado(&r);
  reset();agendar();prontoFonte();estadoBuffer=0;
  assert(proximopreparo_consumir("tt42:1:2",&r));assert(r.indiceFonte>=0&&!r.token&&resolucoes==1);
  assert(fontefalha_tentativas(r.politica)==4);proximopreparo_liberar_resultado(&r);
  reset();semFonte=1;agendar();prontoFonte();assert(!iniciaBuffer);
  assert(proximopreparo_consumir("tt42:1:2",&r)&&r.indiceFonte==-1&&r.politica);assert(fontefalha_tentativas(r.politica)==5);proximopreparo_liberar_resultado(&r);
  // A pending explicit Next can stop waiting and fall back once when this
  // speculative metadata job fails. Internal cleanup does not revoke intent;
  // no partial list, reservation/HTTP budget, token or payload is promoted.
  for(int response=1;response<=2;response++) {
    reset();metaResposta=response;atomic_store(&bloquearMeta,1);agendar();entrou(&entrouMeta);
    unsigned intentEpoch=proximopreparo_geracao();assert(proximopreparo_em_curso("tt42:1:2"));
    atomic_store(&bloquearMeta,0);prontoFonte();
    assert(!proximo&&!proximopreparo_em_curso("tt42:1:2")&&proximopreparo_geracao()==intentEpoch);
    assert(!proximopreparo_consumir("tt42:1:2",&r)&&!r.fontes&&!r.politica&&!r.token);
    assert(consultas==1&&!resolucoes&&!reservas&&!http&&!iniciaBuffer);
  }
  for(int changed=0;changed<7;changed++) {
    reset();atomic_store(&bloquearMeta,1);agendar();entrou(&entrouMeta);
    if(changed==0)escopo.perfil++;
    if(changed==1)strcpy(escopo.conta,"conta-B");
    if(changed==2)escopo.addons++;
    if(changed==3)escopo.geracao++;
    if(changed==4)cfg++;
    if(changed==5)ligado=0;
    if(changed==6)manual=1;
    proximopreparo_passo("tt42:1:1");proximopreparo_encerrar();assert(!resolucoes&&!iniciaBuffer&&!proximo);
  }
  // Incoming metadata scope was captured in the previous account visit.
  // Same account/profile/addons/settings strings after A->B->A must not
  // rebind that old scope/target to a new original dispatch actor.
  reset();FontecacheEscopo escopoAntigo=escopo;alterarPerfil();alterarPerfil();
  assert(!perfis_sessao_atual(escopoAntigo.sessao));
  assert(!proximopreparo_agendar(0,1,2,"tt42:1:1","tt42:1:2",&escopoAntigo));
  assert(!proximo&&!consultas&&!resolucoes&&!reservas&&!http&&!iniciaBuffer);
  // The old metadata actor must remain bound through the source capture.
  // Deliberately create a CURRENT snapshot after an asynchronous actor boundary:
  // checking only that new snapshot would allow the old episode to spend routes.
  reset();agendar();entrou(&entrouMeta);
  for(int i=0;i<2000 && atomic_load(&proximo->estado)!=PP_META_PRONTA;i++)usleep(1000);
  assert(atomic_load(&proximo->estado)==PP_META_PRONTA);
  NvPerfilSessao originalPerfil=proximo->sessaoPerfil;
  unsigned originalIntento=proximopreparo_geracao();mudarPerfilCriar=1;
  proximopreparo_passo("tt42:1:1");proximopreparo_encerrar();
  assert(snapshotPerfil!=originalPerfil && perfis_sessao_atual(snapshotPerfil));
  assert(!proximo && proximopreparo_geracao()!=originalIntento && !resolucoes && !reservas && !http && !iniciaBuffer);
  assert(!proximopreparo_consumir("tt42:1:2",&r));
  // The original actor also reaches the worker callback without relying on
  // a UI frame to cancel the job. A paused resolver wakes and spends nothing.
  reset();atomic_store(&bloquearResolver,1);agendar();
  for(int i=0;i<2000&&!atomic_load(&entrouResolver);i++){proximopreparo_passo("tt42:1:1");usleep(1000);}
  entrou(&entrouResolver);alterarPerfil();
  for(int i=0;i<2000 && atomic_load(&proximo->estado)!=PP_RESOLVIDO;i++)usleep(1000);
  assert(atomic_load(&proximo->estado)==PP_RESOLVIDO && proximo->resultado==-1);
  assert(!reservas && !http && !iniciaBuffer);
  proximopreparo_passo("tt42:1:1");proximopreparo_encerrar();assert(!proximo && resolucoes==1);
  // A foreground source refresh advances only debrid.busca. Neither the VOD
  // metadata scope nor saved credentials change, so the fingerprint must
  // cancel this job before any stale resolver reservation or native start.
  reset();agendar();entrou(&entrouMeta);
  for(int i=0;i<2000 && atomic_load(&proximo->estado)!=PP_META_PRONTA;i++)usleep(1000);
  assert(atomic_load(&proximo->estado)==PP_META_PRONTA);
  FontecacheEscopo escopoAntes=escopo;DebridContexto contextoAntes=debridAtual;
  unsigned epochAntes=proximopreparo_geracao();debridAtual.busca++;
  assert(!memcmp(&escopoAntes,&escopo,sizeof escopo) && cfg==100);
  contextoAntes.busca=debridAtual.busca;assert(!memcmp(&contextoAntes,&debridAtual,sizeof debridAtual));
  proximopreparo_passo("tt42:1:1");proximopreparo_encerrar();
  assert(!proximo && proximopreparo_geracao()!=epochAntes && !resolucoes && !reservas && !http && !iniciaBuffer);
  // The same change while a resolver is paused must wake its cancellation
  // callback and discard its unspent real FonteFalha ledger.
  reset();atomic_store(&bloquearResolver,1);agendar();
  for(int i=0;i<2000&&!atomic_load(&entrouResolver);i++){proximopreparo_passo("tt42:1:1");usleep(1000);}
  entrou(&entrouResolver);debridAtual.busca++;proximopreparo_passo("tt42:1:1");proximopreparo_encerrar();
  assert(!proximo && resolucoes==1 && !reservas && !http && !iniciaBuffer);
  // Even genuine native failure cannot continue a ready old-epoch snapshot:
  // context cancellation wins before native status dispatch, with no extra
  // provider reservation/HTTP or next prebuffer and no consumable old policy.
  reset();agendar();prontoFonte();assert(reservas==4&&http==4&&iniciaBuffer==1);
  estadoBuffer=-1;debridAtual.busca++;proximopreparo_passo("tt42:1:1");proximopreparo_encerrar();
  assert(!proximo&&resolucoes==1&&reservas==4&&http==4&&iniciaBuffer==1&&cancelaBuffer==1);
  assert(!proximopreparo_consumir("tt42:1:2",&r));
  reset();atomic_store(&bloquearResolver,1);agendar();
  for(int i=0;i<2000&&!atomic_load(&entrouResolver);i++){proximopreparo_passo("tt42:1:1");usleep(1000);}entrou(&entrouResolver);
  assert(proximopreparo_em_curso("tt42:1:2"));proximopreparo_cancelar();proximopreparo_encerrar();assert(!iniciaBuffer);
  reset();atomic_store(&bloquearMeta,1);agendar();entrou(&entrouMeta);unsigned before=proximopreparo_geracao();
  proximopreparo_cancelar();assert(proximopreparo_geracao()!=before);agendar();atomic_store(&bloquearMeta,0);prontoFonte();
  assert(consultas==2&&resolucoes==1&&iniciaBuffer==1);
  assert(proximopreparo_consumir("tt42:1:2",&r));proximopreparo_liberar_resultado(&r);
  reset();agendar();prontoFonte();assert(!proximopreparo_consumir("tt42:9:9",&r)&&cancelaBuffer==1);
  reset();agendar();prontoFonte();proximopreparo_passo("tt99:1:1");assert(!proximo&&cancelaBuffer==1);
  reset();agendar();prontoFonte();atomic_store(&relogio,1000+FA_BUFFER_VALE_MS);
  assert(proximopreparo_consumir("tt42:1:2",&r)&&r.indiceFonte==-2&&!r.token&&r.politica);proximopreparo_liberar_resultado(&r);
  // Inactive failover keeps a ledger inside the snapshot. An expired
  // successful handoff must transfer that same spent ledger, not drop it.
  reset();politicaInterna=1;agendar();prontoFonte();atomic_fetch_add(&relogio,FA_BUFFER_VALE_MS);
  assert(proximopreparo_consumir("tt42:1:2",&r)&&r.indiceFonte==-2&&!r.token&&r.politica);
  assert(fontefalha_tentativas(r.politica)==4&&reservas==4&&http==4);
  proximopreparo_liberar_resultado(&r);
  reset();semSnapshot=1;assert(!proximopreparo_agendar(0,1,2,"tt42:1:1","tt42:1:2",&escopo));assert(!consultas);
  proximopreparo_encerrar();
  puts("Next controller: independent large list, 3 TB then PM budget/one-shot transfer, fresh READY/loading reuse, one expired refresh, native failure continuation, exhaustion, original actor/transport epoch cancellation without extra budget or HTTP, late worker/target/TTL cancellation PASS");
}
