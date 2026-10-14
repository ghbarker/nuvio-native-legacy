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
static int metadataOutcome=-1, metadataCalls, snapshotCreates;
static uint64_t joinedConfig=1;
void addons_capturar_escopo(FontecacheEscopo *s) {
  memset(s,0,sizeof *s); s->perfil=profile; s->addons=1; s->geracao=1;
  snprintf(s->conta,sizeof s->conta,"%s",account);
#ifdef NUVIO_COMBINED
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
  metadataCalls++; return metadataOutcome;
}
int fontecache_vod_pegar(const char *id,const char *type,const char *base,const FontecacheEscopo *s,
                         Stream **l,int *n,Uint32 *age) {
  (void)id;(void)type;(void)base;(void)s; *l=NULL;*n=0;*age=0;return FC_NADA;
}
void fontecache_vod_guardar(const char *id,const char *type,const char *base,const FontecacheEscopo *s,
                           const Stream *l,int n,Uint32 age) {
  (void)id;(void)type;(void)base;(void)s;(void)l;(void)n;(void)age;
  assert(!"failed/empty metadata must not enter source cache");
}
uint64_t stream_preparo_config_atual(void) { return joinedConfig; }
int stream_preparo_contexto_atual(const StreamPreparo *p) { (void)p; return 1; }
StreamPreparo *stream_preparo_criar_alvo(const Stream *l,int n,int idx,const char *id) {
  (void)l;(void)n;(void)idx;(void)id;snapshotCreates++; assert(!"incomplete metadata must not produce a partial source snapshot");return NULL;
}
void stream_preparo_definir_episodio(StreamPreparo *p,int t,int e) { (void)p;(void)t;(void)e; }
void stream_preparo_liberar(StreamPreparo *p) { assert(!p); }
int stream_preparo_resolver(StreamPreparo *p,int(*cancel)(void*),void *ctx,Stream *s,int *idx,FonteFalhaSessao **f) {
  (void)p;(void)cancel;(void)ctx;(void)s;(void)idx;(void)f;assert(!"failed metadata cannot spend source budget");return -1;
}
int stream_preparo_renovar(StreamPreparo *p,int idx,int(*cancel)(void*),void *ctx,Stream *s) {
  (void)p;(void)idx;(void)cancel;(void)ctx;(void)s;assert(!"no source exists to renew");return 0;
}
int stream_preparo_continuar(StreamPreparo *p,FonteFalhaSessao **f) { (void)p;(void)f;assert(0);return 0; }
FonteFalhaSessao *stream_preparo_tomar_politica(StreamPreparo *p) { (void)p;assert(0);return NULL; }
Stream *stream_preparo_tomar_lista(StreamPreparo *p,int *n) { (void)p;(void)n;assert(0);return NULL; }
int player_precarregar_proxima_fonte(int c,int t,int e,const Stream *s,unsigned token) {
  (void)c;(void)t;(void)e;(void)s;(void)token;assert(!"incomplete metadata cannot preload");return 0;
}
static void joinedReset(void) {
  proximopreparo_cancelar(); if(worker) concluirWorker();
  nextReset(); metadataCalls=snapshotCreates=0; joinedConfig=1;
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
#ifdef NUVIO_COMBINED
      atomic_fetch_add(&fixtureSession,1);
#endif
    }
    proximoPreparoPasso();
    assert(!proximoIntento.tem&&!closes&&!opens&&!coldRequests&&playerE==1);
    assert(proximopreparo_geracao()>epoch);
    concluirWorker(); for(int i=0;i<8;i++) proximoPreparoPasso();
    assert(!metadataCalls&&!snapshotCreates&&!requests&&!nativeStarts&&!opens&&!coldRequests);
  }
  proximopreparo_encerrar();
  puts("PASS joined actual app+Next module: pending explicit Next falls back once after metadata failure/empty; Back/rewind/profile cancel never opens; no partial list or source budget spend");
  return 0;
}
