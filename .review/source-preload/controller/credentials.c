#include "debrid.h"
#include "rede.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static pthread_mutex_t testLock=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t testCond=PTHREAD_COND_INITIALIZER;
static int entered,released,requests,posts,cacheNegative;
static const char *expected="KEY-PROFILE-A";
char *rede_baixar_st(const char *url,int seconds,const char *const *headers,int *status) {
 (void)seconds;assert(strstr(url,"checkcached"));assert(headers&&headers[0]&&strstr(headers[0],expected));
 pthread_mutex_lock(&testLock);requests++;entered=1;pthread_cond_broadcast(&testCond);while(!released)pthread_cond_wait(&testCond,&testLock);pthread_mutex_unlock(&testLock);
 if(cacheNegative){*status=200;return strdup("{\"success\":true,\"data\":[]}");}
 *status=403;return strdup("{\"success\":false,\"error\":\"PLAN_RESTRICTED_FEATURE\"}");
}
char *rede_postar_st(const char *url,int seconds,const char *const *headers,const char *body,int *status) { (void)url;(void)seconds;(void)headers;(void)body;posts++;*status=500;return NULL; }
static DebridContexto job;
static int result;
static void *worker(void *u) { (void)u;char url[700];debrid_contexto_entrar(&job);result=debrid_resolver("abcdef",-1,url,sizeof url);assert(!url[0]);debrid_contexto_sair();return NULL; }
static void key(const char *value) { DebridConta c;debrid_conta_iniciar(&c);debrid_conta_definir(&c,"torbox",value);debrid_aplicar_chaves_locais(&c); }
int main(void) {
 DebridConta c,empty;debrid_conta_iniciar(&c);debrid_conta_iniciar(&empty);debrid_conta_definir(&c,"torbox","CLOUD-A");
 debrid_abrir_sessao(100);assert(debrid_aplicar_chaves_conta(&c,100));assert(debrid_origem("torbox")==1);assert(debrid_aplicar_chaves_conta(&empty,100));assert(!debrid_ativo());
 assert(debrid_aplicar_chaves_conta(&c,100));debrid_abrir_sessao(101);assert(!debrid_ativo());assert(!debrid_aplicar_chaves_conta(&c,100));assert(!debrid_ativo());
 key("KEY-PROFILE-A");debrid_definir_episodio(2,5);debrid_contexto_capturar(&job);pthread_t t;assert(!pthread_create(&t,NULL,worker,NULL));
 pthread_mutex_lock(&testLock);while(!entered)pthread_cond_wait(&testCond,&testLock);pthread_mutex_unlock(&testLock);
 debrid_abrir_sessao(102);key("KEY-PROFILE-B");debrid_definir_episodio(3,7);
 pthread_mutex_lock(&testLock);released=1;pthread_cond_broadcast(&testCond);pthread_mutex_unlock(&testLock);pthread_join(t,NULL);
 assert(!result&&requests==1&&!posts&&!debrid_sem_plano()&&!debrid_fora_de_cache());
 // Dispatch before switch, worker scheduled after it: never reads/sends B's key.
 assert(!pthread_create(&t,NULL,worker,NULL));pthread_join(t,NULL);assert(!result&&requests==1&&!posts);
 // Even old code trying to mutate the target from its worker cannot change B.
 debrid_contexto_entrar(&job);assert(!debrid_ativo());const char *origins[4];assert(!debrid_origens(origins,4));debrid_definir_episodio(9,9);int season,episode;debrid_episodio(&season,&episode);assert(season==2&&episode==5);debrid_contexto_sair();debrid_episodio(&season,&episode);assert(season==3&&episode==7);
#ifdef NUVIO_PROFILE_COMBINED
 // Retained per-provider resolver must install a snapshot even without an
 // app worker context, and stale contexts must not read current availability.
 debrid_contexto_entrar(&job);assert(!debrid_servico_disponivel(1));char link[700];int outside=7;
 assert(!debrid_resolver_servico_cache(1,"abcdef",-1,link,sizeof link,&outside));assert(outside==0&&requests==1&&!link[0]);debrid_contexto_sair();
 assert(debrid_servico_disponivel(1));expected="KEY-PROFILE-B";cacheNegative=1;outside=0;
 assert(!debrid_resolver_servico_cache(1,"abcdef",-1,link,sizeof link,&outside));assert(outside==1&&requests==2&&!posts&&!link[0]&&!debrid_sem_plano());
#endif
 // Cloud replacement does not erase an explicit local bank.
 assert(debrid_aplicar_chaves_conta(&empty,102));DebridContexto current;debrid_contexto_capturar(&current);assert(!strcmp(current.chave[1],"KEY-PROFILE-B"));
 puts("provider ownership: complete cloud replacement, stale publish, delayed worker, in-flight response, target isolation, local bank PASS");return 0;
}
