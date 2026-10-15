/* Typed metadata-transport boundary. Actual vertudo scheduling, state publication,
   pointer handling and real perfis.c epochs remain production code. */
#include <pthread.h>
#include "perfisessao.h"
#include <time.h>
#include <unistd.h>
#ifdef NV_COLLECTION_PARALLEL_BASELINE
typedef struct { int n, extraN, erro; } DescColecaoResultado;
#endif
#define PARALLEL_SOURCE_MAX 128
static pthread_mutex_t parallelMutex=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t parallelChanged=PTHREAD_COND_INITIALIZER;
static int parallelGate,parallelReleased[PARALLEL_SOURCE_MAX],parallelStarted[PARALLEL_SOURCE_MAX];
static int parallelActive,parallelMaxActive,parallelCalls,parallelCompleted;
static int parallelLegacyGate,parallelLegacyStarted[PARALLEL_SOURCE_MAX];
static int parallelSource(const ColSource *s){const char *p=s->catId;while(*p&&(*p<'0'||*p>'9'))p++;return *p?atoi(p):0;}
static unsigned parallelMs(void){struct timespec ts;clock_gettime(CLOCK_MONOTONIC,&ts);return (unsigned)((uint64_t)ts.tv_sec*1000+ts.tv_nsec/1000000);}
static void parallelBlockFrom(int first){pthread_mutex_lock(&parallelMutex);parallelGate=1;for(int r=0;r<PARALLEL_SOURCE_MAX;r++)parallelReleased[r]=r<first;pthread_mutex_unlock(&parallelMutex);}
static void parallelReleaseOne(int r){assert(r>=0&&r<PARALLEL_SOURCE_MAX);pthread_mutex_lock(&parallelMutex);parallelReleased[r]=1;pthread_cond_broadcast(&parallelChanged);pthread_mutex_unlock(&parallelMutex);}
static void parallelReleaseAll(void){pthread_mutex_lock(&parallelMutex);parallelGate=0;pthread_cond_broadcast(&parallelChanged);pthread_mutex_unlock(&parallelMutex);}
static int parallelStartedAt(int r){int n;pthread_mutex_lock(&parallelMutex);n=parallelStarted[r]+parallelLegacyStarted[r];pthread_mutex_unlock(&parallelMutex);return n;}
int desc_colecao_inicial(const ColSource *source,const char *base,NvPerfilSessao epoch,
                         CatItem *out,int limit,DescColecaoResultado *result){
 int r=parallelSource(source);assert(r>=0&&r<PARALLEL_SOURCE_MAX);assert(base&&*base&&out&&limit>0&&result);(void)epoch;
 pthread_mutex_lock(&parallelMutex);parallelStarted[r]++;parallelCalls++;loads++;parallelActive++;if(parallelActive>parallelMaxActive)parallelMaxActive=parallelActive;pthread_cond_broadcast(&parallelChanged);
 while(parallelGate&&!parallelReleased[r])pthread_cond_wait(&parallelChanged,&parallelMutex);
 int count=itemCount<limit?itemCount:limit;
 for(int i=0;i<count;i++){out[i]=items[i];snprintf(out[i].imdb,sizeof out[i].imdb,"tt%03d",r*100+i+1);snprintf(out[i].poster,sizeof out[i].poster,"https://art.invalid/source%d/title%d.jpg",r,i+1);}
 *result=(DescColecaoResultado){.n=count,.extraN=0,.erro=0};parallelActive--;parallelCompleted++;pthread_cond_broadcast(&parallelChanged);pthread_mutex_unlock(&parallelMutex);
 /* Even a stale epoch is returned deliberately: these UI tests verify that
    the UI scheduler also rejects stale ownership after transport completes. */
 return 1;
}
static void parallelPumpRows(unsigned mask){unsigned end=parallelMs()+3000;for(;;){vitCarregar(clockNow+=16);vitProjetar();unsigned ready=0;for(int r=0;r<vitN;r++)if(vitLin[r].estado==VIT_PRONTA||vitLin[r].estado==VIT_VAZIA)ready|=1u<<r;if((ready&mask)==mask)return;assert((int)(end-parallelMs())>0);usleep(1000);}}
static void parallelWaitStarted(unsigned mask){unsigned end=parallelMs()+3000;for(;;){vitCarregar(clockNow+=16);unsigned started=0;for(int r=0;r<16;r++)if(parallelStartedAt(r))started|=1u<<r;if((started&mask)==mask)return;if((int)(end-parallelMs())<=0){fprintf(stderr,"PARALLEL_WAIT_TIMEOUT expected_mask=%u observed_mask=%u legacy_loads=%u backend_calls=%d source0=%d source1=%d source2=%d\n",mask,started,loads,parallelCalls,parallelStartedAt(0),parallelStartedAt(1),parallelStartedAt(2));assert(0);}usleep(1000);}}
static void parallelWaitReturned(void){unsigned end=parallelMs()+3000;for(;;){pthread_mutex_lock(&parallelMutex);int active=parallelActive;pthread_mutex_unlock(&parallelMutex);if(!active)return;assert((int)(end-parallelMs())>0);usleep(1000);}}
