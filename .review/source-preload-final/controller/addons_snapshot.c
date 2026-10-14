#define _GNU_SOURCE
#define SDL_MAIN_HANDLED
#ifdef NUVIO_COMBINED
#include "session-stubs.h"
#endif
#include SOURCE_ADDONS
#include <assert.h>
static DebridContexto captured;
static pthread_key_t testKey;
static pthread_once_t keyOnce = PTHREAD_ONCE_INIT;
static pthread_mutex_t testLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t testCond = PTHREAD_COND_INITIALIZER;
static int entered, released, requests, queryResult, cancelQuery, bodyCase;
static size_t largestBody;
static Stream *querySources;
static AddonsConsulta *snapshot;
static void keyInit(void) { assert(!pthread_key_create(&testKey,NULL)); }
void debrid_contexto_capturar(DebridContexto *c) { *c=captured; }
void debrid_contexto_entrar(const DebridContexto *c) { pthread_once(&keyOnce,keyInit); assert(!pthread_getspecific(testKey)); pthread_setspecific(testKey,c); }
void debrid_contexto_sair(void) { pthread_setspecific(testKey,NULL); }
Uint32 SDL_GetTicks(void) { return 1234; }
uint64_t badges_detectar(const char *s) { (void)s; return 0; }
uint64_t badges_bit(const char *s) { (void)s; return 0; }
char *rede_baixar(const char *url,int seconds) {
  assert(seconds == 12);
  const DebridContexto *c = pthread_getspecific(testKey);
  assert(c && c->chave[DEBRID_TB][0] == 1 && c->temporada == 2 && c->episodio == 7);
  assert(strstr(url,"snapshot-a.invalid/old-a/") || strstr(url,"snapshot-b.invalid/old-b/"));
  assert(strstr(url,"/stream/series/tt-capture:2:8.json"));
  pthread_mutex_lock(&testLock); requests++; entered++; pthread_cond_broadcast(&testCond);
  while(!released) pthread_cond_wait(&testCond,&testLock);
  pthread_mutex_unlock(&testLock);
  if(bodyCase) {
    static const char *const bodies[]={"", "{\"streams\":[]}", "<html>upstream error</html>",
      "{\"error\":\"not authorized\"}", "{\"streams\":[{\"url\":\"https://truncated.invalid\"}",
      "{\"nested\":{\"streams\":[]}}", "{\"streams\":[4]}", "{\"streams\":null}"};
    return strdup(bodies[bodyCase]);
  }
  char description[1901]; memset(description,'x',sizeof description-1); description[1900]=0;
  char *json = malloc(1000000); assert(json); size_t off=0;
  off+=(size_t)sprintf(json+off,"{\"streams\":[");
  for(int i=0;i<300;i++) off+=(size_t)sprintf(json+off,"%s{\"name\":\"1080p\",\"description\":\"%s\",\"url\":\"https://media.invalid/%d.mp4\"}",i?",":"",description,i);
  off+=(size_t)sprintf(json+off,"]}");
  pthread_mutex_lock(&testLock); if(off>largestBody) largestBody=off; pthread_mutex_unlock(&testLock);
  assert(off>512*1024); return json;
}
static int cancelled(void *ctx) { (void)ctx; return cancelQuery; }
static void *query(void *ctx) { (void)ctx; queryResult=addons_consulta_consultar(snapshot,"tt-capture:2:8","series",2,cancelled,NULL,&querySources); return NULL; }
typedef struct { int active, config, result, failCallback, key; } MockOrigin;
static MockOrigin liveOrigin[2];
static int originEntered, originQueries, liveOriginQueries, originFreed;
static int activePlugin(void) { return liveOrigin[0].active; }
static int activePlex(void) { return liveOrigin[1].active; }
static int livePlugin(const char *id,const char *tipo,int(*cancel)(void*),void *ctx,OrigemAviso aviso,void *u,void *out) {
  (void)id;(void)tipo;(void)cancel;(void)ctx;(void)aviso;(void)u;(void)out; liveOriginQueries++; return -1;
}
static int livePlex(const char *id,const char *tipo,int(*cancel)(void*),void *ctx,OrigemAviso aviso,void *u,void *out) {
  return livePlugin(id,tipo,cancel,ctx,aviso,u,out);
}
static void *captureOrigin(int idx) { MockOrigin *o=malloc(sizeof *o); assert(o); *o=liveOrigin[idx]; return o; }
static void *capturePlugin(void) { return captureOrigin(0); }
static void *capturePlex(void) { return captureOrigin(1); }
static int currentPlugin(const void *p) { return !memcmp(p,&liveOrigin[0],sizeof liveOrigin[0]); }
static int currentPlex(const void *p) { return !memcmp(p,&liveOrigin[1],sizeof liveOrigin[1]); }
static void freeOrigin(void *p) { free(p); originFreed++; }
static int queryOrigin(const void *data,const char *id,const char *type,int(*cancel)(void*),void *ctx,
                       OrigemAviso aviso,void *u,Stream **out,int plugin) {
  const MockOrigin *o=data; *out=NULL;
  assert(!strcmp(id,"tt-capture:2:8")&&!strcmp(type,"series"));
  const DebridContexto *dc=pthread_getspecific(testKey);
  assert(dc&&dc->chave[DEBRID_TB][0]==1&&o->key==1);
  pthread_mutex_lock(&testLock); originEntered++; originQueries++; pthread_cond_broadcast(&testCond);
  while(!released) pthread_cond_wait(&testCond,&testLock);
  pthread_mutex_unlock(&testLock);
  if(cancel(ctx)||o->result<=0) return o->result<0?-1:0;
  Stream *l=calloc((size_t)o->result,sizeof *l); assert(l);
  for(int i=0;i<o->result;i++) {
    snprintf(l[i].provedor,sizeof l[i].provedor,"%s %d",plugin?"Captured plugin":"Captured Plex",i);
    snprintf(l[i].url,sizeof l[i].url,"https://captured-extra.invalid/%s/%d.mp4",plugin?"plugin":"plex",i);
    if(plugin) {
      char key[80]; snprintf(key,sizeof key,"plugin:captured-%d",i);
      fontefalha_addon_chave(key,l[i].addonChave,sizeof l[i].addonChave);
      snprintf(l[i].infoHash,sizeof l[i].infoHash,"captured-native-%d",i); l[i].fileIdx=i+4;
      strcpy(l[i].cabecalhos,"X-Captured: original");
      aviso(u,i?3:0,l[i].provedor,1,NULL,0);
      aviso(u,i?3:0,l[i].provedor,o->failCallback?3:2,&l[i],1);
    }
  }
  *out=l; return o->result;
}
static int queryPlugin(const void *d,const char *id,const char *t,int(*c)(void*),void *ctx,OrigemAviso a,void *u,Stream **out) {
  return queryOrigin(d,id,t,c,ctx,a,u,out,1);
}
static int queryPlex(const void *d,const char *id,const char *t,int(*c)(void*),void *ctx,OrigemAviso a,void *u,Stream **out) {
  return queryOrigin(d,id,t,c,ctx,a,u,out,0);
}
static void allOriginsSnapshots(void) {
  const OrigemPreparoOps pluginOps={capturePlugin,currentPlugin,queryPlugin,freeOrigin};
  const OrigemPreparoOps plexOps={capturePlex,currentPlex,queryPlex,freeOrigin};
  addons_definir_origem_extra(livePlugin,activePlugin); addons_definir_origem_extra_preparo(livePlugin,&pluginOps);
  addons_definir_origem_extra(livePlex,activePlex); addons_definir_origem_extra_preparo(livePlex,&plexOps);
  nAddon=2; strcpy(addon[0].base,"https://snapshot-a.invalid/old-a"); strcpy(addon[1].nome,"Second instance");
  captured.chave[DEBRID_TB][0]=1; captured.temporada=2; captured.episodio=7;
  liveOrigin[0]=(MockOrigin){1,10,2,0,1}; liveOrigin[1]=(MockOrigin){1,20,1,0,1};
  cancelQuery=0; entered=originEntered=released=0;
  unsigned version=versaoLista; int oldProg=progN, oldPub=progPubN; AddEstado oldState=atomic_load(&estado);
  int oldMudo=addon[0].mudoSeg;
  snapshot=addons_consulta_capturar(); assert(snapshot&&addons_consulta_atual(snapshot));
  pthread_t th; assert(!pthread_create(&th,NULL,query,NULL));
  pthread_mutex_lock(&testLock);
  while(entered<2||originEntered<2) pthread_cond_wait(&testCond,&testLock);
  released=1; pthread_cond_broadcast(&testCond); pthread_mutex_unlock(&testLock);
  pthread_join(th,NULL);
  assert(queryResult==603&&querySources&&originQueries==2&&!liveOriginQueries);
  assert(querySources[600].addonOrdem==ADD_MAX&&querySources[601].addonOrdem==ADD_MAX+3&&querySources[602].addonOrdem==ADD_MAX+1);
  assert(!strcmp(querySources[600].infoHash,"captured-native-0")&&querySources[601].fileIdx==5);
  assert(!strcmp(querySources[600].cabecalhos,"X-Captured: original")&&querySources[600].addonChave[0]&&!querySources[602].addonChave[0]);
  assert(version==versaoLista&&oldProg==progN&&oldPub==progPubN&&oldState==atomic_load(&estado)&&oldMudo==addon[0].mudoSeg);
  free(querySources); querySources=NULL;
  liveOrigin[0].config++; assert(!addons_consulta_atual(snapshot)); liveOrigin[0].config--;
  liveOrigin[1].key++; assert(!addons_consulta_atual(snapshot)); liveOrigin[1].key--;
  liveOrigin[1].active=0; assert(!addons_consulta_atual(snapshot)); liveOrigin[1].active=1;
  captured.busca++; assert(!addons_consulta_atual(snapshot)); captured.busca--;
  captured.chave[DEBRID_TB][0]++; assert(!addons_consulta_atual(snapshot)); captured.chave[DEBRID_TB][0]--;
  addons_consulta_liberar(snapshot);
  // Extras-only is queried even with no installed addon; valid empty Plex
  // and plugins also leave a complete AIO response usable.
  nAddon=0; int oldRequests=requests;
  snapshot=addons_consulta_capturar(); assert(snapshot);
  query(NULL); assert(queryResult==3&&querySources&&requests==oldRequests);
  free(querySources); querySources=NULL; addons_consulta_liberar(snapshot);
  nAddon=2; liveOrigin[0].result=liveOrigin[1].result=0;
  snapshot=addons_consulta_capturar(); assert(snapshot);
  query(NULL); assert(queryResult==600&&querySources);
  free(querySources); querySources=NULL; addons_consulta_liberar(snapshot);
  liveOrigin[0].result=2; liveOrigin[1].result=1;
  for(bodyCase=1;bodyCase<=7;bodyCase++) {
    snapshot=addons_consulta_capturar(); assert(snapshot); query(NULL);
    assert(queryResult==(bodyCase==1?3:-1));
    if(bodyCase==1) { assert(querySources); free(querySources); querySources=NULL; }
    else assert(!querySources);
    addons_consulta_liberar(snapshot);
  }
  bodyCase=0;
  for(int failure=0;failure<2;failure++) {
    liveOrigin[0].result=failure?-1:2; liveOrigin[0].failCallback=!failure;
    snapshot=addons_consulta_capturar(); assert(snapshot); query(NULL);
    assert(queryResult==-1&&!querySources); addons_consulta_liberar(snapshot);
  }
  // Unknown enabled origins still take the regular foreground path.
  addons_definir_origem_extra_preparo(livePlex,NULL);
  int freedBefore=originFreed;
  assert(!addons_consulta_capturar()&&originFreed==freedBefore+1);
  addons_definir_origem_extra_preparo(livePlex,&plexOps);
  addons_definir_origem_extra_preparo(livePlugin,NULL);
  assert(!addons_consulta_capturar());
  liveOrigin[0].active=0; snapshot=addons_consulta_capturar(); assert(snapshot);
  addons_consulta_liberar(snapshot); assert(originFreed>=12&&!liveOriginQueries);
  puts("PASS all-origin snapshot: parallel AIO/plugins/Plex, extras-only, exact provenance/native metadata, valid-empty vs incomplete, config/key/off rejection, no foreground writes");
}
int main(void) {
  nAddon=2; versaoLista=77; memset(addon,0,sizeof addon);
  strcpy(addon[0].nome,"First instance"); strcpy(addon[1].nome,"Second instance");
  strcpy(addon[0].base,"https://snapshot-a.invalid/old-a"); strcpy(addon[1].base,"https://snapshot-b.invalid/old-b");
  addon[0].ativo=addon[0].fonte=addon[1].ativo=addon[1].fonte=1;
  captured.chave[DEBRID_TB][0]=1; captured.temporada=2; captured.episodio=7;
  snapshot=addons_consulta_capturar(); assert(snapshot&&addons_consulta_atual(snapshot));
  pthread_t thread; assert(!pthread_create(&thread,NULL,query,NULL));
  pthread_mutex_lock(&testLock); while(entered<2) pthread_cond_wait(&testCond,&testLock); pthread_mutex_unlock(&testLock);
  strcpy(addon[0].base,"https://new-profile.invalid/new-a"); strcpy(addon[1].nome,"Changed profile"); versaoLista++;
  captured.chave[DEBRID_TB][0]=2; captured.temporada=3; captured.episodio=2;
  assert(!addons_consulta_atual(snapshot));
  pthread_mutex_lock(&testLock); released=1; pthread_cond_broadcast(&testCond); pthread_mutex_unlock(&testLock);
  pthread_join(thread,NULL); assert(queryResult==600&&querySources&&requests==2&&largestBody>512*1024);
  char firstKey[FONTEFALHA_CHAVE], secondKey[FONTEFALHA_CHAVE];
  fontefalha_addon_chave("https://snapshot-a.invalid/old-a",firstKey,sizeof firstKey);
  fontefalha_addon_chave("https://snapshot-b.invalid/old-b",secondKey,sizeof secondKey);
  for(int i=0;i<600;i++) {
    assert(querySources[i].addonOrdem==i/300);
    assert(!strcmp(querySources[i].provedor,i<300?"First instance":"Second instance"));
    assert(!strcmp(querySources[i].addonChave,i<300?firstKey:secondKey));
  }
  free(querySources); querySources=NULL;
  cancelQuery=1; assert(addons_consulta_consultar(snapshot,"tt-capture:2:8","series",2,cancelled,NULL,&querySources)==-1);
  assert(!querySources&&requests==2);
#ifdef NUVIO_COMBINED
  cancelQuery=0; atomic_fetch_add(&fixtureSession,1);
  assert(!addons_consulta_atual(snapshot));
  assert(addons_consulta_consultar(snapshot,"tt-capture:2:8","series",2,NULL,NULL,&querySources)==-1);
  assert(!querySources&&requests==2);
  addons_consulta_liberar(snapshot);
  strcpy(addon[0].base,"https://snapshot-a.invalid/old-a"); strcpy(addon[1].nome,"Second instance");
  captured.chave[DEBRID_TB][0]=1; captured.temporada=2; captured.episodio=7;
  snapshot=addons_consulta_capturar(); assert(snapshot);
  entered=released=0; assert(!pthread_create(&thread,NULL,query,NULL));
  pthread_mutex_lock(&testLock); while(entered<2) pthread_cond_wait(&testCond,&testLock); pthread_mutex_unlock(&testLock);
  atomic_fetch_add(&fixtureSession,1);
  pthread_mutex_lock(&testLock); released=1; pthread_cond_broadcast(&testCond); pthread_mutex_unlock(&testLock);
  pthread_join(thread,NULL); assert(queryResult==-1&&!querySources&&requests==4);
  puts("PASS combined addon snapshot rejects old same-profile visit and frees full in-flight result");
#endif
  addons_consulta_liberar(snapshot);
  puts("addon snapshots: two immutable instances/credentials/episode, stale rejection, full600 sources >512KiB, cancel PASS");
  allOriginsSnapshots();
}
