// Actual app intent controller joined to the actual next preparation module.
// Scheduling is deterministic; only external query/source/player boundaries
// are stubbed. No module API is replaced and source policy is the real ledger.
#define JOINED_APP_NEXT
#define main unused_controller_main
#include "controller.c"
#undef main
#define pthread_create fake_create
#include SOURCE_PROXIMO
#undef pthread_create

struct AddonsConsulta { int owner; };
static int metadataOutcome=-1, metadataCalls, snapshotCreates, metadataStores;
struct StreamPreparo { Stream *list; int n; uint64_t actor; };

static uint64_t joinedConfig=1;
void addons_capturar_escopo(FontecacheEscopo *s) {
  memset(s,0,sizeof *s); s->perfil=profile; s->addons=1; s->geracao=1;
  snprintf(s->conta,sizeof s->conta,"%s",account);
#if defined(NUVIO_COMBINED) || defined(NUVIO_SOURCE_EPOCH)
  s->sessao=perfis_sessao_geracao();
#endif
}
AddonsConsulta *addons_consulta_capturar(void) {
  AddonsConsulta *a=malloc(sizeof *a); assert(a); a->owner=profile; return a;
}
int addons_consulta_atual(const AddonsConsulta *a) { return a&&a->owner==profile; }
void addons_consulta_liberar(AddonsConsulta *a) { free(a); }
int addons_consulta_consultar(const AddonsConsulta *a,const char *id,const char *type,
                              int fios,int(*cancel)(void*),void *ctx,void *out) {
  assert(a&&a->owner==profile&&!strcmp(id,nextTarget)&&!strcmp(type,"series")&&fios>0);
  *(Stream**)out=NULL; if(cancel(ctx)) return -1;
  metadataCalls++;
  if(metadataOutcome>0) {
    Stream *list=malloc(4*sizeof *list); assert(list); memcpy(list,fontes,4*sizeof *list);
    *(Stream**)out=list;
  }
  return metadataOutcome;
}
int fontecache_vod_pegar(const char *id,const char *type,const char *base,const FontecacheEscopo *s,
                         Stream **l,int *n,Uint32 *age) {
  (void)id;(void)type;(void)base;(void)s; *l=NULL;*n=0;*age=0;return FC_NADA;
}
void fontecache_vod_guardar(const char *id,const char *type,const char *base,const FontecacheEscopo *s,
                           const Stream *l,int n,Uint32 age) {
  (void)id;(void)type;(void)base;(void)s;(void)l;(void)n;(void)age;
  assert(metadataOutcome==4&&n==4&&l&&s->sessao==perfis_sessao_geracao()); metadataStores++;
}
uint64_t stream_preparo_config_atual(void) { return joinedConfig ^ perfis_sessao_geracao(); }
int stream_preparo_contexto_atual(const StreamPreparo *p) { return p&&perfis_sessao_atual(p->actor); }
StreamPreparo *stream_preparo_criar_alvo(const Stream *l,int n,int idx,const char *id) {
  (void)idx; assert(metadataOutcome==4&&n==4&&!strcmp(id,nextTarget)); snapshotCreates++;
  StreamPreparo *p=calloc(1,sizeof *p); assert(p); p->list=malloc(4*sizeof *l); assert(p->list);
  memcpy(p->list,l,4*sizeof *l); p->n=n; p->actor=perfis_sessao_geracao(); return p;
}
void stream_preparo_definir_episodio(StreamPreparo *p,int t,int e) { (void)p;(void)t;(void)e; }
void stream_preparo_liberar(StreamPreparo *p) {
  if(!p) return; free(p->list); free(p);
  // The actual module has already validated ownership and detached payload.
  // Model a token-worker identity boundary before it returns to app.c.
  if(actorAfterConsume) atomic_fetch_add(&fixtureSession,2);
}
int stream_preparo_resolver(StreamPreparo *p,int(*cancel)(void*),void *ctx,Stream *s,int *idx,FonteFalhaSessao **f) {
  assert(p&&stream_preparo_contexto_atual(p)&&!cancel(ctx)); *idx=2; *s=p->list[2];
  FonteFalhaConfig cfg; fontefalha_config(&cfg); *f=fontefalha_sessao_nova(&cfg,6,-1); assert(*f);
  FonteFalhaCandidata c={.indice=2,.nativa=1,.identidade=s->infoHash,.ordemAddon=1}; strcpy(c.addon,s->addonChave);
  assert(fontefalha_tentar(*f,&c,DEBRID_TB)); snprintf(s->url,sizeof s->url,"https://joined-next.invalid/prepared.mp4");
  assert(fontefalha_url_registrar(*f,s->url)); p->list[2]=*s; requests++; return 1;
}
int stream_preparo_renovar(StreamPreparo *p,int idx,int(*cancel)(void*),void *ctx,Stream *s) {
  (void)p;(void)idx;(void)cancel;(void)ctx;(void)s;assert(!"no source exists to renew");return 0;
}
int stream_preparo_continuar(StreamPreparo *p,FonteFalhaSessao **f) { (void)p;(void)f;assert(0);return 0; }
FonteFalhaSessao *stream_preparo_tomar_politica(StreamPreparo *p) { (void)p;assert(0);return NULL; }
Stream *stream_preparo_tomar_lista(StreamPreparo *p,int *n) { Stream *l=p->list; *n=p->n;p->list=NULL;return l; }
int player_precarregar_proxima_fonte(int c,int t,int e,const Stream *s,unsigned token) {
  assert(c==0&&t==1&&e==2&&s->url[0]&&token); nativeStarts++;lastToken=token;return 1;
}
static void joinedReset(void) {
  proximopreparo_cancelar(); if(worker) concluirWorker();
  nextReset(); metadataCalls=snapshotCreates=metadataStores=0; joinedConfig=1;
}
static void scheduleAndPressNext(void) {
  FontecacheEscopo scope; addons_capturar_escopo(&scope);
  assert(proximopreparo_agendar(0,1,2,stampedTarget,nextTarget,&scope));
  assert(worker&&proximopreparo_em_curso(nextTarget));
  abrirProximoEpisodio(1,2,1);
  assert(proximoIntento.tem&&!closes&&!opens&&!coldRequests&&loadingToasts==1);
}
int main(int argc,char **argv) {
  assert(argc==2); fontefalha_dir(argv[1]);
  for(int outcome=-1;outcome<=0;outcome++) {
    joinedReset(); metadataOutcome=outcome; scheduleAndPressNext();
    unsigned epoch=proximopreparo_geracao(); concluirWorker();
    assert(proximo&&atomic_load(&proximo->estado)==PP_META_PRONTA&&metadataCalls==1);
    proximoPreparoPasso();
    assert(!proximo&&!proximoIntento.tem&&closes==1&&opens==1&&coldRequests==1);
    assert(savedT==1&&savedE==1&&playerT==1&&playerE==2&&aguardandoFonte==1);
    assert(!snapshotCreates&&!requests&&!nativeStarts&&!fonteFalha);
    assert(proximopreparo_geracao()==epoch+1); // only normal player close cancels
    for(int i=0;i<8;i++) proximoPreparoPasso();
    assert(closes==1&&opens==1&&coldRequests==1);
  }
  for(int cancel=0;cancel<3;cancel++) {
    joinedReset(); metadataOutcome=-1; scheduleAndPressNext();
    unsigned epoch=proximopreparo_geracao();
    if(cancel<2) proximopreparo_cancelar(); // Back/rewind share explicit API
    else { profile++;
#if defined(NUVIO_COMBINED) || defined(NUVIO_SOURCE_EPOCH)
      atomic_fetch_add(&fixtureSession,1);
#endif
    }
    proximoPreparoPasso();
    assert(!proximoIntento.tem&&!closes&&!opens&&!coldRequests&&playerE==1);
    assert(proximopreparo_geracao()>epoch);
    concluirWorker(); for(int i=0;i<8;i++) proximoPreparoPasso();
    assert(!metadataCalls&&!snapshotCreates&&!requests&&!nativeStarts&&!opens&&!coldRequests);
  }
  // A failure can finish before the actor boundary, leaving only the intent.
  joinedReset(); metadataOutcome=-1; scheduleAndPressNext(); concluirWorker();
  proximopreparo_passo(stampedTarget); assert(!proximo&&proximoIntento.tem);
  atomic_fetch_add(&fixtureSession,2); proximoPreparoPasso();
  assert(!proximoIntento.tem&&!closes&&!opens&&!coldRequests&&!requests);
  // Actual module consumes/detaches ready bytes and spent ledger, then app
  // must recheck the original actor without resetting or using its ledger.
  for(int race=0;race<4;race++) {
    joinedReset(); metadataOutcome=4; scheduleAndPressNext(); concluirWorker();
    proximopreparo_passo(stampedTarget); assert(worker); concluirWorker();
    proximopreparo_passo(stampedTarget);
    assert(proximo&&proximo->politica&&fontefalha_tentativas(proximo->politica)==1);
    assert(metadataStores==1&&snapshotCreates==1&&requests==1&&nativeStarts==1);
    nativeState=2; actorAfterConsume=race==1; actorDuringClose=race==2; actorDuringOpen=race==3; unsigned unchangedList=lista;
    proximoPreparoPasso();
    assert(!proximo&&!proximoIntento.tem&&!coldRequests&&requests==1&&!worker);
    if(!race) {
      assert(closes==1&&opens==1&&nativeAdopt==1&&!nativeCancel);
      assert(fonteFalha&&fontefalha_tentativas(fonteFalha)==1&&fonteEscolhida==2&&playerE==2);
    } else if(race<3) {
      assert(!opens&&nativeCancel==1&&!fonteFalha&&playerE==1);
      assert(closes==(race==2)&&nativeAdopt==(race==2));
    } else {
      assert(closes==2&&opens==1&&!aberto&&lista==unchangedList);
      assert(nativeCancel==1&&nativeAdopt==1&&!fonteFalha&&fonteEscolhida==-2);
    }
  }
  proximopreparo_encerrar();
  puts("PASS joined actual app+Next module: pending explicit Next falls back once after metadata failure/empty; Back/rewind/profile cancel never opens; detached warm payload actor gates preserve one spent reservation");
  return 0;
}
