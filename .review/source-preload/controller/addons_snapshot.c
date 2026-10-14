#define _GNU_SOURCE
#define SDL_MAIN_HANDLED
#include SOURCE_ADDONS
#include <assert.h>
static DebridContexto captured;
static pthread_key_t testKey;
static pthread_once_t keyOnce = PTHREAD_ONCE_INIT;
static pthread_mutex_t testLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t testCond = PTHREAD_COND_INITIALIZER;
static int entered, released, requests, queryResult, cancelQuery;
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
  assert(!querySources&&requests==2); addons_consulta_liberar(snapshot);
  puts("addon snapshots: two immutable instances/credentials/episode, stale rejection, full600 sources >512KiB, cancel PASS");
}
